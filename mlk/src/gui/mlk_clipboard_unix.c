/*$
 Copyright (C) 2013-2022 Azel.

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
 * mClipboardUnix
 * 
 * X11/Wayland 共通、クリップボードデータ管理
 *****************************************/

#include <string.h>

#include "mlk_gui.h"
#include "mlk_list.h"
#include "mlk_str.h"
#include "mlk_string.h"
#include "mlk_buf.h"
#include "mlk_clipboard.h"

#include "mlk_clipboard_unix.h"
#include "mlk_pv_mimelist.h"


//===========================
// sub
//===========================


/** データ取得時: 自身が持っているデータを指定タイプで取得 */

static int _get_data_self(mClipboardUnix *p,
	const char *mimetype,mFuncClipboardRecv handle,void *param)
{
	mClipboardSendData send;
	mBuf buf;
	int ret;

	mBufInit(&buf);

	send.mimetype = mimetype;
	send.type = p->dat_type;
	send.buf = p->dat_buf;
	send.size = p->dat_size;

	p->send_data = &buf;
	p->send_is_self = TRUE;

	//データを送信させる (mBuf に)
	
	ret = ((mFuncClipboardSend)p->sendfunc)(&send);

	if(ret == MCLIPBOARD_RET_OK)
	{
		//成功

		if(!buf.buf)
			//確保に失敗
			ret = MCLIPBOARD_RET_ERR;
		else
			ret = (handle)(buf.buf, buf.cursize, param);
	}
	else if(ret == MCLIPBOARD_RET_SEND_RAW)
		//生データをそのまま送る
		ret = (handle)(p->dat_buf, p->dat_size, param);
	else
		ret = MCLIPBOARD_RET_ERR;

	mBufFree(&buf);

	return ret;
}

/** 他クライアントのタイプのリストの中から、実際に取得可能な MIME タイプを一つ取得 */

static char *_find_mimetype(mClipboardUnix *p,const char *mime_types)
{
	const char *pc;
	char **buf,**ps;

	//取得可能なタイプのリストを取得

	buf = (p->getmimetypelist)();
	if(!buf) return NULL;

	//比較

	for(pc = mime_types; *pc; pc += strlen(pc) + 1)
	{
		for(ps = buf; *ps; ps++)
		{
			if(strcmp(*ps, pc) == 0) goto END;
		}
	}

END:
	mStringFreeArray_tonull(buf);

	return (char *)pc;
}


//===========================
// main
//===========================


/** mClipboardUnix 解放 */

void mClipboardUnix_destroy(mClipboardUnix *p)
{
	mFree(p->dat_buf);
	mListDeleteAll(&p->list_mime);
}

/** セットされているクリップボードデータを解放 */

void mClipboardUnix_freeData(mClipboardUnix *p)
{
	mFree(p->dat_buf);
	mListDeleteAll(&p->list_mime);

	p->dat_buf = NULL;
	p->dat_size = 0;
}

/** データをセット */

mlkbool mClipboardUnix_setData(mClipboardUnix *p,
	int type,const void *buf,uint32_t size,
	const char *mimetypes,mFuncEmpty handle)
{
	mList *list;

	//データ解放

	mClipboardUnix_freeData(p);

	//データをコピー

	p->dat_buf = mMalloc(size);
	if(!p->dat_buf) return FALSE;

	memcpy(p->dat_buf, buf, size);

	//セット

	p->dat_type = type;
	p->dat_size = size;
	p->sendfunc = handle;

	//MIME タイプリスト

	list = &p->list_mime;

	if(!mimetypes)
		//タイプごとのデフォルト
		(p->setmimetype_default)(list, type);
	else
	{
		//指定文字列を登録
		const char *pc = mimetypes;

		for(; *pc; pc += strlen(pc) + 1)
			(p->addmimetype)(list, pc);
	}

	//所有権セット

	if(!(p->setselection)())
	{
		mClipboardUnix_freeData(p);
		return FALSE;
	}

	return TRUE;
}

/** データを送る
 * 
 * (mClipboardSend() -> mAppBackend::clipboard_send -> this) */

mlkbool mClipboardUnix_send(mClipboardUnix *p,const void *buf,int size)
{
	if(!p->send_is_self)
		//他クライアントへ送る
		return (p->sendto)(buf, size);
	else
	{
		//自身に送る時
		// - mClipboardUnix::send_data には (mBuf *) がセットされている

		mBuf *mbuf = (mBuf *)p->send_data;

		if(!mbuf->buf)
		{
			if(!mBufAlloc(mbuf, 1024, 4096))
				return FALSE;
		}

		return mBufAppend(mbuf, buf, size);
	}
}

/** テキストデータを取得 */

int mClipboardUnix_getText(mClipboardUnix *p,mStr *str)
{
	if(!p->dat_buf)
		//他クライアントから取得
		return (p->gettext)(str);
	else
	{
		//自身がデータを持っている時

		if(p->dat_type == MCLIPBOARD_TYPE_TEXT)
		{
			mStrSetText_len(str, (char *)p->dat_buf, p->dat_size);
			return MCLIPBOARD_RET_OK;
		}
		else
			return MCLIPBOARD_RET_ERR_TYPE;
	}
}

/** 指定タイプのデータを取得 */

int mClipboardUnix_getData(mClipboardUnix *p,
	const char *mimetype,mFuncEmpty handle,void *param)
{
	if(!p->dat_buf)
		//他クライアントから
		return (p->getdata)(mimetype, handle, param);
	else
	{
		//--- 自身がデータを持っている時

		if(!mMimeListFind_eq(&p->list_mime, mimetype))
			return MCLIPBOARD_RET_ERR_TYPE;

		if(p->sendfunc)
			//送信関数がある場合: 同一プログラム内での送受信
			return _get_data_self(p, mimetype, (mFuncClipboardRecv)handle, param);
		else
		{
			//デフォルト処理
			
			if(p->dat_type == MCLIPBOARD_TYPE_TEXT)
				return ((mFuncClipboardRecv)handle)(p->dat_buf, p->dat_size, param);
			else
				return MCLIPBOARD_RET_ERR_TYPE;
		}
	}
}

/** 現在取得可能なクリップボードデータが対応する MIME タイプのリスト取得 */

char **mClipboardUnix_getMimeTypeList(mClipboardUnix *p)
{
	if(!p->dat_buf)
		//他クライアントから取得
		return (p->getmimetypelist)();
	else
		//自身がデータを持っている時、MIME リストから作成
		return mMimeListCreateNameList(&p->list_mime);
}

/** 複数の MIME タイプの中から、現時点で実際に取得可能な最初の MIME タイプを取得 */

char *mClipboardUnix_findMimeType(mClipboardUnix *p,const char *mime_types)
{
	if(!p->dat_buf)
	{
		//他クライアントから取得
		return _find_mimetype(p, mime_types);
	}
	else
	{
		//自身がデータを持っている時

		const char *pc;

		for(pc = mime_types; *pc; pc += strlen(pc) + 1)
		{
			if(mMimeListFind_eq(&p->list_mime, pc))
				return (char *)pc;
		}
			
		return NULL;
	}
}
