/*$
 Copyright (C) 2013-2021 Azel.

 This file is part of AzPainter.

 AzPainter is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 AzPainter is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
$*/

/*****************************************
 * <X11> クリップボード
 *****************************************/

#include <string.h>

#include "mlk_x11.h"

#include "mlk_str.h"
#include "mlk_clipboard.h"

#include "mlk_pv_gui.h"
#include "mlk_pv_mimelist.h"


//-------------------

//データ送信時の情報

typedef struct
{
	Window win;
	Atom prop,type;
	uint8_t is_add;	//プロパティにデータを追加するか
}_send_pv_data;

//-------------------



//================================
// mClipboard から呼ばれる関数
// (mAppBkend に登録)
//================================


/** クリップボード所有権解放 */

static mlkbool _bkend_clipboard_release(void)
{
	mAppX11 *p = MLKAPPX11;

	mClipboardUnix_freeData(&p->clipb);

	//セレクション解放

	if(p->flags & MLKX11_FLAGS_HAVE_SELECTION)
	{
		XSetSelectionOwner(p->display, p->atoms[MLKX11_ATOM_CLIPBOARD], None, CurrentTime);

		p->flags &= ~MLKX11_FLAGS_HAVE_SELECTION;

		return TRUE;
	}

	return FALSE;
}

/** データセット */

static mlkbool _bkend_clipboard_setData(int type,const void *buf,uint32_t size,
	const char *mimetypes,mFuncEmpty handle)
{
	return mClipboardUnix_setData(&MLKAPPX11->clipb,
		type, buf, size, mimetypes, handle);
}

/** データを送る */

static mlkbool _bkend_clipboard_send(const void *buf,int size)
{
	return mClipboardUnix_send(&MLKAPPX11->clipb, buf, size);
}

/** テキストを取得 */

static int _bkend_clipboard_getText(mStr *str)
{
	return mClipboardUnix_getText(&MLKAPPX11->clipb, str);
}

/** データ取得 */

static int _bkend_clipboard_getData(const char *mimetype,mFuncEmpty handle,void *param)
{
	return mClipboardUnix_getData(&MLKAPPX11->clipb, mimetype, handle, param);
}

/** MIME タイプリスト取得 */

static char **_bkend_clipboard_getMimeTypeList(void)
{
	return mClipboardUnix_getMimeTypeList(&MLKAPPX11->clipb);
}

/** 取得可能な MIME タイプ取得 */

static char *_bkend_clipboard_findMimeType(const char *mime_types)
{
	return mClipboardUnix_findMimeType(&MLKAPPX11->clipb, mime_types);
}


//===================================
// sub
//===================================


/** 他クライアントから取得可能なタイプのリスト内から、指定 Atom 配列の値を検索
 *
 * atoms: 値が 0 で終了。先頭にあるほど優先
 * return: 0 でなし */

static Atom _find_target_atom(mAppX11 *p,Atom *atoms)
{
	Atom *buf,*patom;
	int i,num;

	//取得可能なタイプの Atom リスト取得

	num = mX11GetSelectionTargetAtoms(p->leader_window,
		p->atoms[MLKX11_ATOM_CLIPBOARD], &buf);

	if(num == 0) return 0;

#if MLK_DEBUG_PUT_EVENT

	mDebug(">>> TARGETS >>>\n");
	for(i = 0; i < num; i++)
		mX11PutAtomName(buf[i]);
	mDebug("<<< TARGETS <<<\n");
	
#endif

	//検索

	for(patom = atoms; *patom; patom++)
	{
		for(i = 0; i < num; i++)
		{
			if(*patom == buf[i]) goto END;
		}
	}

END:
	mFree(buf);

	return *patom;
}


//===================================
// mClipboardUnix 用関数
// (mClipboardUnix にセットされる)
//===================================


/** 指定タイプのデフォルトの MIME タイプセット */

static void _bkend_setMimeType_default(mList *list,int type)
{
	mAppX11 *p = MLKAPPX11;

	if(type == MCLIPBOARD_TYPE_TEXT)
	{
		//テキスト
		mMimeListAdd(list, "text/plain;charset=utf-8", (void *)(intptr_t)p->atoms[MLKX11_ATOM_text_utf8]);
		mMimeListAdd(list, "UTF8_STRING", (void *)(intptr_t)p->atoms[MLKX11_ATOM_UTF8_STRING]);
		mMimeListAdd(list, "COMPOUND_TEXT", (void *)(intptr_t)p->atoms[MLKX11_ATOM_COMPOUND_TEXT]);
	}
}

/** MIME タイプリストに追加 */

static void _bkend_addMimeType(mList *list,const char *name)
{
	mMimeListAdd(list, name, (void *)(intptr_t)mX11GetAtom(name));
}

/** 所有権をセットする */

static mlkbool _bkend_setSelection(void)
{
	mAppX11 *p = MLKAPPX11;
	Atom selection;

	if(!(p->flags & MLKX11_FLAGS_HAVE_SELECTION))
	{
		selection = p->atoms[MLKX11_ATOM_CLIPBOARD];

		XSetSelectionOwner(p->display, selection, p->leader_window, CurrentTime);

		if(XGetSelectionOwner(p->display, selection) != p->leader_window)
			return FALSE;

		p->flags |= MLKX11_FLAGS_HAVE_SELECTION;
	}

	return TRUE;
}

/** 他クライアントにデータ送信 (mClipboardSend) */

static mlkbool _bkend_send_data(const void *buf,int size)
{
	_send_pv_data *p = (_send_pv_data *)MLKAPPX11->clipb.send_data;

	mX11SetProperty8(p->win, p->prop, p->type, buf, size, p->is_add);

	//次回以降はプロパティに追加
	p->is_add = 1;

	return TRUE;
}

/** 所有権を持っているクライアントからテキスト取得 */

static int _bkend_getText(mStr *str)
{
	mAppX11 *p = MLKAPPX11;
	Window win;
	Atom selection,target,atoms[4];
	void *buf;
	uint32_t size;
	int ret,len;

	win = p->leader_window;
	selection = p->atoms[MLKX11_ATOM_CLIPBOARD];

	//所有者がいるか

	if(!XGetSelectionOwner(p->display, selection))
		return MCLIPBOARD_RET_NONE;

	//タイプ検索

	atoms[0] = p->atoms[MLKX11_ATOM_text_utf8];
	atoms[1] = p->atoms[MLKX11_ATOM_UTF8_STRING];
	atoms[2] = p->atoms[MLKX11_ATOM_COMPOUND_TEXT];
	atoms[3] = 0;

	target = _find_target_atom(p, atoms);
	if(!target) return MCLIPBOARD_RET_ERR_TYPE;

	//データ取得

	if(target == p->atoms[MLKX11_ATOM_COMPOUND_TEXT])
	{
		ret = mX11GetSelectionCompoundText(win, selection, (char **)&buf, &len);
		if(ret == 0) size = len;
	}
	else
		//UTF-8
		ret = mX11GetSelectionData(win, selection, target, CurrentTime, FALSE, &buf, &size);

	if(ret)
		return MCLIPBOARD_RET_ERR;
	else
	{
		//成功
		
		mStrSetText_len(str, (char *)buf, size);
		mFree(buf);
		return MCLIPBOARD_RET_OK;
	}
}

/** 所有権を持っているクライアントから指定データ取得 */

static int _bkend_getData(const char *mimetype,mFuncEmpty handle,void *param)
{
	void *buf;
	uint32_t size;
	int ret;

	ret = mX11GetSelectionData(MLKAPPX11->leader_window,
		MLKAPPX11->atoms[MLKX11_ATOM_CLIPBOARD],
		mX11GetAtom(mimetype), CurrentTime, FALSE, &buf, &size);

	if(ret == 0)
	{
		ret = ((mFuncClipboardRecv)handle)(buf, size, param);

		mFree(buf);

		return ret;
	}
	else if(ret == -1)
		//所有者なし
		return MCLIPBOARD_RET_NONE;
	else
		return MCLIPBOARD_RET_ERR;
}

/** 他クライアントが所持しているデータの MIME タイプリスト取得
 *
 * return: NULL でなし */

static char **_bkend_getMimeTypeList(void)
{
	Atom *atom;
	char **buf,**pd,*name;
	Display *disp;
	int i,num;

	//Atom リスト

	num = mX11GetSelectionTargetAtoms(MLKAPPX11->leader_window,
		MLKAPPX11->atoms[MLKX11_ATOM_CLIPBOARD], &atom);

	if(num == 0) return NULL;

	//確保

	buf = (char **)mMalloc(sizeof(char *) * (num + 1));
	if(!buf)
	{
		mFree(atom);
		return NULL;
	}

	//セット

	disp = MLKX11_DISPLAY;

	for(pd = buf, i = 0; i < num; i++)
	{
		name = XGetAtomName(disp, atom[i]);
		*(pd++) = mStrdup(name);
		XFree(name);
	}

	*pd = 0;

	mFree(atom);

	return buf;
}


//=========================
//
//=========================


/** 初期化 */

void __mX11ClipboardInit(void)
{
	mAppBackend *bkend = &MLKAPP->bkend;
	mClipboardUnix *clip = &MLKAPPX11->clipb;

	//mAppBackend

	bkend->clipboard_release = _bkend_clipboard_release;
	bkend->clipboard_setdata = _bkend_clipboard_setData;
	bkend->clipboard_send = _bkend_clipboard_send;
	bkend->clipboard_gettext = _bkend_clipboard_getText;
	bkend->clipboard_getdata = _bkend_clipboard_getData;
	bkend->clipboard_getmimetypelist = _bkend_clipboard_getMimeTypeList;
	bkend->clipboard_findmimetype = _bkend_clipboard_findMimeType;

	//mClipboardUnix

	clip->setmimetype_default = _bkend_setMimeType_default;
	clip->addmimetype = _bkend_addMimeType;
	clip->setselection = _bkend_setSelection;
	clip->sendto = _bkend_send_data;
	clip->gettext = _bkend_getText;
	clip->getdata = _bkend_getData;
	clip->getmimetypelist = _bkend_getMimeTypeList;
}


//=========================
// イベント
//=========================


/** MIME タイプリスト内に指定タイプがあるか
 *
 * return: MIME タイプ文字列。NULL でなし */

static char *_check_type(mClipboardUnix *p,Atom type)
{
	mMimeListItem *pi;

	for(pi = (mMimeListItem *)p->list_mime.top; pi; pi = (mMimeListItem *)pi->i.next)
	{
		if((intptr_t)pi->param == type)
			return pi->name;
	}

	return NULL;
}

/** テキストのデータを送信する
 * 
 * 終端のヌル文字は送らない。 */

static void _send_text(mClipboardUnix *p,Window win,Atom prop,Atom type)
{
	if(type == MLKAPPX11->atoms[MLKX11_ATOM_COMPOUND_TEXT])
		mX11SetPropertyCompoundText(win, prop, (char *)p->dat_buf, p->dat_size);
	else
		//UTF-8
		mX11SetProperty8(win, prop, type, p->dat_buf, p->dat_size, FALSE);
}

/** 指定タイプのデータを送信する
 *
 * type: 要求されたタイプ
 * return: 送信されたか */

static mlkbool _send_data(mClipboardUnix *p,Window win,Atom prop,Atom type)
{
	char *mime;
	mClipboardSendData send;
	_send_pv_data dat;
	int ret;

	//MIME リスト内の MIME タイプ文字列取得

	mime = _check_type(p, type);
	if(!mime) return FALSE;

	//データ送信

	if(!p->sendfunc)
	{
		//デフォルト処理

		if(p->dat_type == MCLIPBOARD_TYPE_TEXT)
			_send_text(p, win, prop, type);
		else
			return FALSE;
	}
	else
	{
		//mFuncClipboardSend ハンドラを使って送信

		send.mimetype = mime;
		send.type = p->dat_type;
		send.buf = p->dat_buf;
		send.size = p->dat_size;
		
		dat.win = win;
		dat.prop = prop;
		dat.type = type;
		dat.is_add = 0;

		p->send_data = &dat;
		p->send_is_self = FALSE;

		ret = ((mFuncClipboardSend)p->sendfunc)(&send);

		if(ret == MCLIPBOARD_RET_OK)
			//関数内で送信された
			return TRUE;
		else if(ret == MCLIPBOARD_RET_SEND_RAW)
		{
			//関数内で送信していない。生データをそのまま送信させる
			mX11SetProperty8(win, prop, type, p->dat_buf, p->dat_size, FALSE);
			return TRUE;
		}
		else
			return FALSE;
	}
	
	return TRUE;
}

/** 送信可能なタイプの Atom リストを送る */

static mlkbool _send_targets(mClipboardUnix *p,Window win,Atom prop)
{
	Atom *buf,*pd;
	mMimeListItem *pi;
	int num;

	num = p->list_mime.num + 1;

	buf = (Atom *)mMalloc(sizeof(Atom) * num);
	if(!buf) return FALSE;

	//バッファにセット
	
	pd = buf;

	*(pd++) = MLKAPPX11->atoms[MLKX11_ATOM_TARGETS];

	for(pi = (mMimeListItem *)p->list_mime.top; pi; pi = (mMimeListItem *)pi->i.next)
		*(pd++) = (intptr_t)pi->param;

	//

	mX11SetPropertyAtom(win, prop, buf, num);

	mFree(buf);

	return TRUE;
}

/** SelectionRequest イベント時の処理 (selection="CLIPBOARD")
 *
 * 自身に所有権がある状態で、他のクライアントからデータの送信が要求された時。 */

void mX11Clipboard_selection_request(XSelectionRequestEvent *ev)
{
	mAppX11 *app = MLKAPPX11;
	mClipboardUnix *p = &app->clipb;
	XSelectionEvent xev;
	Atom target;

#ifdef MLK_DEBUG_PUT_EVENT
	_X11_DEBUG("SelectionRequest: ");
	mX11PutAtomName(ev->target);
#endif

	//[!] 送信に対応しない場合は、target を 0 にして送る

	target = ev->target;

	if(!p->dat_buf)
		//セットされているデータがない
		target = 0;
	else if(target == app->atoms[MLKX11_ATOM_TARGETS])
	{
		//"TARGETS" : 送信可能なフォーマットタイプの要求 

		if(!_send_targets(p, ev->requestor, ev->property))
			target = 0;
	}
	else
	{
		//データを送る

		if(!_send_data(p, ev->requestor, ev->property, target))
			target = 0;
	}

	//SelectionNotify イベント送信

	xev.type       = SelectionNotify;
	xev.send_event = 1;
	xev.display    = app->display;
	xev.requestor  = ev->requestor;
	xev.selection  = ev->selection;
	xev.target     = target;
	xev.property   = ev->property;
	xev.time       = ev->time;

	XSendEvent(xev.display, xev.requestor, False, 0, (XEvent *)&xev);
}
