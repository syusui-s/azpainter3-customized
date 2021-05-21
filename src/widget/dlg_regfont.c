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

/*******************************
 * 登録フォントダイアログ
 *******************************/

#include <string.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_label.h"
#include "mlk_groupbox.h"
#include "mlk_checkbutton.h"
#include "mlk_lineedit.h"
#include "mlk_listview.h"
#include "mlk_button.h"
#include "mlk_combobox.h"
#include "mlk_sysdlg.h"
#include "mlk_event.h"
#include "mlk_columnitem.h"
#include "mlk_str.h"
#include "mlk_buf.h"
#include "mlk_unicode.h"

#include "def_config.h"

#include "regfont.h"
#include "appresource.h"
#include "apphelp.h"

#include "widget_func.h"

#include "trid.h"

//------------------

enum
{
	TRID_TITLE_LIST,
	TRID_TITLE_FONT,
	TRID_TITLE_CHAR,

	TRID_REGNAME,
	TRID_BASEFONT,
	TRID_REPFONT1,
	TRID_REPFONT2,
	TRID_CHAREDIT,
	TRID_CHARTYPE,
	TRID_CODE_SPEC,
	TRID_VIEW_UNICODE,

	TRID_CHARLIST_TOP = 100,

	TRID_MES_NAME = 1000,
	TRID_MES_BASEFONT,
	TRID_MES_CHAR_ERROR,
	TRID_MES_CODE_DUP
};

//------------------


//************************************
// 置き換え文字の編集ダイアログ
//************************************

#define _CHARLIST_NUM  6

typedef struct
{
	MLK_DIALOG_DEF

	mBuf buf;  //OK 時に作成される

	mLineEdit *edit_code,
		*edit_unicode,
		*edit_unicode_view;
	mCheckButton *ck_list[_CHARLIST_NUM],
		*ck_cid;
}_dlg_char;

enum
{
	WID_CHAR_BTT_HELP = 100,
	WID_CHAR_EDIT_UNICODE
};


/* コード値の取得
 *
 * return: 0 で成功 (終了する) */

static int _char_get_codebuf(_dlg_char *p)
{
	mStr str = MSTR_INIT;
	mBuf buf;
	int ret;

	//空

	if(mLineEditIsEmpty(p->edit_code)) return 0;

	//バッファ

	mBufInit(&buf);

	if(!mBufAlloc(&buf, 1024, 1024)) return 0;

	//buf に値をセット

	mLineEditGetTextStr(p->edit_code, &str);

	ret = RegFontDat_text_to_charbuf(&buf, str.buf, !mCheckButtonIsChecked(p->ck_cid));

	mStrFree(&str);

	//

	if(!ret)
		p->buf = buf;
	else
	{
		mBufFree(&buf);

		if(ret == 1)
			ret = TRID_MES_CODE_DUP;
		else
			ret = TRID_MES_CHAR_ERROR;
		
		mMessageBoxErr(MLK_WINDOW(p), MLK_TR(ret));
	}

	return ret;
}

/* Unicode 表示 */

static void _char_view_unicode(_dlg_char *p)
{
	mStr str = MSTR_INIT,strdst = MSTR_INIT;
	mlkuchar c;
	const char *pc;

	mLineEditGetTextStr(p->edit_unicode, &str);

	if(mStrIsEmpty(&str))
	{
		mLineEditSetText(p->edit_unicode_view, NULL);
		mStrFree(&str);
		return;
	}

	//Unicode 値

	pc = str.buf;

	while(*pc && mUTF8GetChar(&c, pc, -1, &pc) == 0)
	{
		mStrAppendFormat(&strdst, "%X", c);

		if(*pc)
			mStrAppendChar(&strdst, ',');
	}

	mLineEditSetText(p->edit_unicode_view, strdst.buf);

	//

	mStrFree(&str);
	mStrFree(&strdst);
}

/* イベント */

static int _char_event_handle(mWidget *wg,mEvent *ev)
{
	switch(ev->type)
	{
		case MEVENT_NOTIFY:
			switch(ev->notify.id)
			{
				//Unicode表示
				case WID_CHAR_EDIT_UNICODE:
					if(ev->notify.notify_type == MLINEEDIT_N_CHANGE)
						_char_view_unicode((_dlg_char *)wg);
					break;
				
				//OK
				case MLK_WID_OK:
					if(_char_get_codebuf((_dlg_char *)wg))
						return 1;
					break;

				//ヘルプ
				case WID_CHAR_BTT_HELP:
					AppHelp_message(MLK_WINDOW(wg), HELP_TRGROUP_SINGLE, HELP_TRID_REGFONT_CHAREDIT);
					break;
			}
			break;
	}

	return mDialogEventDefault_okcancel(wg, ev);
}

/* コード値から mLineEdit にテキストセット */

static void _char_set_codetext(mLineEdit *edit,RegFontDat *dat,int no)
{
	mStr str = MSTR_INIT;

	if(RegFontDat_getCodeText(dat, no, &str))
		mLineEditSetText(edit, str.buf);

	mStrFree(&str);
}

/* ダイアログ作成 */

static _dlg_char *_create_dlg_char(mWindow *parent,RegFontDat *dat,int fontno)
{
	_dlg_char *p;
	mWidget *ct,*ct2;
	int i;
	uint32_t charflags;

	MLK_TRGROUP(TRGROUP_DLG_REGFONT);

	p = (_dlg_char *)widget_createDialog(parent, sizeof(_dlg_char),
		MLK_TR(TRID_TITLE_CHAR), _char_event_handle);

	if(!p) return NULL;

	charflags = dat->charflags[fontno];

	//---- 文字種

	ct = (mWidget *)mGroupBoxCreate(MLK_WIDGET(p), 0, 0, 0, MLK_TR(TRID_CHARTYPE));

	mContainerSetType_vert(MLK_CONTAINER(ct), 3);

	for(i = 0; i < _CHARLIST_NUM; i++)
	{
		p->ck_list[i] = mCheckButtonCreate(ct, 0, 0, 0, 0, MLK_TR(TRID_CHARLIST_TOP + i),
			charflags & (1 << i));
	}

	//---- コード指定

	ct = (mWidget *)mGroupBoxCreate(MLK_WIDGET(p), MLF_EXPAND_W,
		MLK_MAKE32_4(0,10,0,0), 0, MLK_TR(TRID_CODE_SPEC));

	mContainerSetType_vert(MLK_CONTAINER(ct), 6);

	//type

	ct2 = mContainerCreateHorz(ct, 5, 0, 0);

	mCheckButtonCreate(ct2, 0, 0, 0, MCHECKBUTTON_S_RADIO, "Unicode", !(charflags & REGFONT_CHARF_CODE_CID));
	
	p->ck_cid = mCheckButtonCreate(ct2, 0, 0, 0, MCHECKBUTTON_S_RADIO, "CID", ((charflags & REGFONT_CHARF_CODE_CID) != 0));

	//edit

	ct2 = mContainerCreateHorz(ct, 3, MLF_EXPAND_W, 0);

	p->edit_code = mLineEditCreate(ct2, 0, MLF_EXPAND_W | MLF_MIDDLE, 0, 0);

	mWidgetSetInitSize_fontHeightUnit(MLK_WIDGET(p->edit_code), 20, 0);

	widget_createHelpButton(ct2, WID_CHAR_BTT_HELP, MLF_MIDDLE, 0);

	_char_set_codetext(p->edit_code, dat, fontno);

	//----- Unicode 表示

	mLabelCreate(MLK_WIDGET(p), 0, MLK_MAKE32_4(0,10,0,0), 0, MLK_TR(TRID_VIEW_UNICODE));

	//

	ct = mContainerCreateHorz(MLK_WIDGET(p), 4, MLF_EXPAND_W, MLK_MAKE32_4(0,5,0,0));

	p->edit_unicode = mLineEditCreate(ct, WID_CHAR_EDIT_UNICODE, MLF_EXPAND_W, 0, MLINEEDIT_S_NOTIFY_CHANGE);

	p->edit_unicode_view = mLineEditCreate(ct, 0, MLF_EXPAND_W, 0, MLINEEDIT_S_READ_ONLY);

	//----- OK/Cancel

	mContainerCreateButtons_okcancel(MLK_WIDGET(p), MLK_MAKE32_4(0,15,0,0));

	return p;
}

/* OK 時 */

static void _char_end_ok(_dlg_char *p,RegFontDat *dat,int no)
{
	int i;
	uint16_t f = 0;

	//CID

	if(mCheckButtonIsChecked(p->ck_cid))
		f |= REGFONT_CHARF_CODE_CID;

	//フラグ

	for(i = 0; i < _CHARLIST_NUM; i++)
	{
		if(mCheckButtonIsChecked(p->ck_list[i]))
			f |= 1 << i;
	}

	dat->charflags[no] = f;

	//文字データ

	RegFontDat_setCharBuf(dat, no, &p->buf);

	//

	mBufFree(&p->buf);
}

/* 置換文字編集ダイアログ
 *
 * no: 置き換えフォントの番号 (0,1) */

static mlkbool _chareditdlg_run(mWindow *parent,RegFontDat *dat,int no)
{
	_dlg_char *p;
	int ret;

	p = _create_dlg_char(parent, dat, no);
	if(!p) return FALSE;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	ret = mDialogRun(MLK_DIALOG(p), FALSE);

	if(ret)
		_char_end_ok(p, dat, no);

	mWidgetDestroy(MLK_WIDGET(p));

	return ret;
}



//************************************
// フォント編集ダイアログ
//************************************

typedef struct
{
	MLK_DIALOG_DEF

	RegFontDat *dat;

	mLineEdit *edit_name,
		*edit_base,
		*edit_rep[2];
}_dlg_font;

enum
{
	WID_FONT_BTT_BASE = 100,
	WID_FONT_BTT_REP1,
	WID_FONT_BTT_REP2,
	WID_FONT_BTT_REP1_CHAR,
	WID_FONT_BTT_REP2_CHAR,
	WID_FONT_BTT_BASE_CLEAR,
	WID_FONT_BTT_REP1_CLEAR,
	WID_FONT_BTT_REP2_CLEAR
};

/* フォント選択 */

static void _font_selectfont(_dlg_font *p,mLineEdit *edit,mStr *pstr,uint32_t *pindex)
{
	int index;

	if(mSysDlg_openFontFile(MLK_WINDOW(p), "All files\t*", 0,
		APPCONF->strFontFileDir.buf, pstr, &index))
	{
		//ディレクトリ記録

		mStrPathGetDir(&APPCONF->strFontFileDir, pstr->buf);

		//

		*pindex = index;

		mLineEditSetText(edit, pstr->buf);
	}
}

/* イベント */

static int _font_event_handle(mWidget *wg,mEvent *ev)
{
	_dlg_font *p = (_dlg_font *)wg;
	int n;

	switch(ev->type)
	{
		case MEVENT_NOTIFY:
			switch(ev->notify.id)
			{
				//ベースフォント選択
				case WID_FONT_BTT_BASE:
					_font_selectfont(p, p->edit_base, &p->dat->str_basefont, &p->dat->index_base);
					break;
				//置換フォント選択
				case WID_FONT_BTT_REP1:
				case WID_FONT_BTT_REP2:
					n = ev->notify.id - WID_FONT_BTT_REP1;
					
					_font_selectfont(p, p->edit_rep[n], &p->dat->str_repfont[n], &p->dat->index_rep[n]);
					break;
				//ベースフォントクリア
				case WID_FONT_BTT_BASE_CLEAR:
					mStrEmpty(&p->dat->str_basefont);
					
					mLineEditSetText(p->edit_base, NULL);
					break;
				//置換フォントクリア
				case WID_FONT_BTT_REP1_CLEAR:
				case WID_FONT_BTT_REP2_CLEAR:
					n = ev->notify.id - WID_FONT_BTT_REP1_CLEAR;

					mStrEmpty(&p->dat->str_repfont[n]);
					
					mLineEditSetText(p->edit_rep[n], NULL);
					break;
				//文字編集
				case WID_FONT_BTT_REP1_CHAR:
				case WID_FONT_BTT_REP2_CHAR:
					_chareditdlg_run(MLK_WINDOW(p), p->dat, ev->notify.id - WID_FONT_BTT_REP1_CHAR);
					break;

				//OK
				case MLK_WID_OK:
					if(mLineEditIsEmpty(p->edit_name))
					{
						//名前が空

						mMessageBoxErrTr(MLK_WINDOW(p), TRGROUP_DLG_REGFONT, TRID_MES_NAME);
						return 1;
					}
					else if(mStrIsEmpty(&p->dat->str_basefont))
					{
						//ベースフォントが空
						
						mMessageBoxErrTr(MLK_WINDOW(p), TRGROUP_DLG_REGFONT, TRID_MES_BASEFONT);
						return 1;
					}
					break;
			}
			break;
	}

	return mDialogEventDefault_okcancel(wg, ev);
}

/* フォント選択のウィジェットを作成 */

static mLineEdit *_font_create_fontselect(mWidget *parent,int id_sel,int id_clear,mStr *str)
{
	mWidget *ct;
	mLineEdit *edit;

	ct = mContainerCreateHorz(parent, 4, MLF_EXPAND_W, 0);

	//edit

	edit = mLineEditCreate(ct, 0, MLF_EXPAND_W | MLF_MIDDLE, 0, MLINEEDIT_S_READ_ONLY);

	mLineEditSetText(edit, str->buf);

	//選択ボタン

	mButtonCreate(ct, id_sel, MLF_MIDDLE, 0, MBUTTON_S_REAL_W, "...");

	//クリアボタン

	mButtonCreate(ct, id_clear, MLF_MIDDLE, 0, MBUTTON_S_REAL_W, "x");

	return edit;
}

/* ダイアログ作成 */

static _dlg_font *_create_dlg_font(mWindow *parent,RegFontDat *dat)
{
	_dlg_font *p;
	mWidget *ct;
	int i;

	MLK_TRGROUP(TRGROUP_DLG_REGFONT);

	p = (_dlg_font *)widget_createDialog(parent, sizeof(_dlg_font),
		MLK_TR(TRID_TITLE_FONT), _font_event_handle);

	if(!p) return NULL;

	p->dat = dat;

	//----- 登録名

	ct = mContainerCreateHorz(MLK_WIDGET(p), 6, MLF_EXPAND_W, 0);

	mLabelCreate(ct, MLF_MIDDLE, 0, 0, MLK_TR(TRID_REGNAME));

	//edit

	p->edit_name = mLineEditCreate(ct, 0, MLF_EXPAND_W | MLF_MIDDLE, 0, 0);

	mLineEditSetText(p->edit_name, dat->strname.buf);

	mWidgetSetInitSize_fontHeightUnit(MLK_WIDGET(p->edit_name), 20, 0);

	//----- ベースフォント

	mLabelCreate(MLK_WIDGET(p), 0, MLK_MAKE32_4(0,10,0,3), 0, MLK_TR(TRID_BASEFONT));

	p->edit_base = _font_create_fontselect(MLK_WIDGET(p),
		WID_FONT_BTT_BASE, WID_FONT_BTT_BASE_CLEAR, &dat->str_basefont);

	//---- 置換フォント

	for(i = 0; i < 2; i++)
	{
		ct = (mWidget *)mGroupBoxCreate(MLK_WIDGET(p), MLF_EXPAND_W, MLK_MAKE32_4(0,10,0,0), 0,
			MLK_TR(TRID_REPFONT1 + i));

		mContainerSetType_vert(MLK_CONTAINER(ct), 7);

		//ファイル選択

		p->edit_rep[i] = _font_create_fontselect(ct,
			WID_FONT_BTT_REP1 + i, WID_FONT_BTT_REP1_CLEAR + i, &dat->str_repfont[i]);

		//文字編集

		mButtonCreate(ct, WID_FONT_BTT_REP1_CHAR + i, 0, 0, 0, MLK_TR(TRID_CHAREDIT));
	}

	//OK/Cancel

	mContainerCreateButtons_okcancel(MLK_WIDGET(p), MLK_MAKE32_4(0,15,0,0));

	return p;
}

/* フォント編集ダイアログ
 *
 * dat に直接編集される。 */

static mlkbool _fonteditdlg_run(mWindow *parent,RegFontDat *dat)
{
	_dlg_font *p;
	int ret;

	p = _create_dlg_font(parent, dat);
	if(!p) return FALSE;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	ret = mDialogRun(MLK_DIALOG(p), FALSE);

	if(ret)
	{
		//名前
		mLineEditGetTextStr(p->edit_name, &dat->strname);
	}

	mWidgetDestroy(MLK_WIDGET(p));

	return ret;
}



//************************************
// リスト一覧ダイアログ
//************************************


typedef struct
{
	MLK_DIALOG_DEF

	int selid,		//選択されているID (-1 でなし)
		fselmodify;	//選択フォントが変更されたか

	mListView *list;
}_dlg_list;

enum
{
	WID_LIST_LIST = 100
};


/* 未使用のIDを取得
 *
 * return: -1 でエラー */

static int _get_unused_id(mListView *list)
{
	mColumnItem *pi;
	uint8_t *buf,*ps,f;
	int id;

	//使用済みIDをフラグにセット

	buf = (uint8_t *)mMalloc0((REGFONT_MAX_NUM + 7) / 8);
	if(!buf) return -1;

	for(pi = mListViewGetTopItem(list); pi; pi = (mColumnItem *)pi->i.next)
	{
		id = ((RegFontDat *)pi->param)->id;

		buf[id >> 3] |= 1 << (id & 7);
	}

	//未使用のIDを検索

	ps = buf;
	f = 1;

	for(id = 0; id < REGFONT_MAX_NUM; id++)
	{
		if(!(*ps & f)) break;

		f <<= 1;
		if(!f) f = 1, ps++;
	}

	mFree(buf);

	return id;
}

/* 追加 */

static void _list_cmd_add(_dlg_list *p)
{
	RegFontDat *dat;
	mColumnItem *item;

	if(mListViewGetItemNum(p->list) >= REGFONT_MAX_NUM)
		return;

	dat = RegFontDat_new();
	if(!dat) return;

	if(!_fonteditdlg_run(MLK_WINDOW(p), dat))
		RegFontDat_free(dat);
	else
	{
		//未使用IDをセット
		
		dat->id = _get_unused_id(p->list);

		item = mListViewAddItem_text_static_param(p->list, dat->strname.buf, (intptr_t)dat);

		mListViewSetFocusItem(p->list, item);
	}
}

/* 編集 */

static void _list_cmd_edit(_dlg_list *p,mColumnItem *item)
{
	RegFontDat *dat,*src;

	src = (RegFontDat *)item->param;

	//複製

	dat = RegFontDat_dup(src);
	if(!dat) return;

	//ダイアログ

	if(!_fonteditdlg_run(MLK_WINDOW(p), dat))
		RegFontDat_free(dat);
	else
	{
		//入れ替え

		RegFontDat_free(src);

		item->param = (intptr_t)dat;

		//名前

		mListViewSetItemText_static(p->list, item, dat->strname.buf);

		//選択フォント更新

		if(dat->id == p->selid)
			p->fselmodify = TRUE;
	}
}

/* 削除 */

static void _list_cmd_delete(_dlg_list *p,mColumnItem *item)
{
	//選択フォントの削除時
	
	if(p->selid == ((RegFontDat *)item->param)->id)
		p->fselmodify = TRUE;

	//

	mListViewDeleteItem_focus(p->list);
}

/* COMMAND イベント */

static void _list_event_command(_dlg_list *p,int id)
{
	mColumnItem *item;

	item = mListViewGetFocusItem(p->list);

	switch(id)
	{
		//追加
		case APPRES_LISTEDIT_ADD:
			_list_cmd_add(p);
			break;
		//編集
		case APPRES_LISTEDIT_EDIT:
			if(item)
				_list_cmd_edit(p, item);
			break;
		//削除
		case APPRES_LISTEDIT_DEL:
			if(item)
				_list_cmd_delete(p, item);
			break;
		//上へ/下へ
		case APPRES_LISTEDIT_UP:
		case APPRES_LISTEDIT_DOWN:
			if(item)
				mListViewMoveItem_updown(p->list, item, (id == APPRES_LISTEDIT_DOWN));
			break;
	}
}

/* イベント */

static int _list_event_handle(mWidget *wg,mEvent *ev)
{
	switch(ev->type)
	{
		case MEVENT_COMMAND:
			_list_event_command((_dlg_list *)wg, ev->cmd.id);
			break;

		case MEVENT_NOTIFY:
			//リストをダブルクリックで編集
			if(ev->notify.id == WID_LIST_LIST
				&& ev->notify.notify_type == MLISTVIEW_N_ITEM_L_DBLCLK)
			{
				_list_cmd_edit((_dlg_list *)wg, (mColumnItem *)ev->notify.param1);
			}
			break;
	}

	return mDialogEventDefault_okcancel(wg, ev);
}

/* mListView アイテム破棄ハンドラ */

static void _list_listview_item_destroy(mList *list,mListItem *item)
{
	RegFontDat_free((RegFontDat *)MLK_COLUMNITEM(item)->param);

	mColumnItemHandle_destroyItem(list, item);
}

/* ダイアログ作成 */

static _dlg_list *_create_dlg_list(mWindow *parent,mComboBox *cblist)
{
	_dlg_list *p;
	mListView *list;
	mListItem *pi;
	mColumnItem *ci;
	RegFontDat *dat;
	const uint8_t bttno[] = {
		APPRES_LISTEDIT_ADD, APPRES_LISTEDIT_EDIT, APPRES_LISTEDIT_DEL,
		APPRES_LISTEDIT_UP, APPRES_LISTEDIT_DOWN, 255
	};

	MLK_TRGROUP(TRGROUP_DLG_REGFONT);

	p = (_dlg_list *)widget_createDialog(parent, sizeof(_dlg_list),
		MLK_TR(TRID_TITLE_LIST), _list_event_handle);

	if(!p) return NULL;

	//現在の選択IDを取得

	ci = mComboBoxGetSelectItem(cblist);

	if(ci)
		p->selid = RegFont_getItemID((mListItem *)ci->param);
	else
		p->selid = -1;

	//リスト

	p->list = list = mListViewCreate(MLK_WIDGET(p), WID_LIST_LIST, MLF_EXPAND_WH, 0,
		MLISTVIEW_S_AUTO_WIDTH, MSCROLLVIEW_S_HORZVERT_FRAME);

	mWidgetSetInitSize_fontHeightUnit(MLK_WIDGET(list), 22, 15);

	mListViewSetItemDestroyHandle(list, _list_listview_item_destroy);

	for(pi = RegFont_getTopItem(); pi; pi = pi->next)
	{
		dat = RegFontDat_make(pi);
		if(dat)
			mListViewAddItem_text_static_param(list, dat->strname.buf, (intptr_t)dat);
	}

	//ボタン

	widget_createListEdit_iconbar_btt(MLK_WIDGET(p), bttno, TRUE);

	return p;
}

/* OK 時、データをセット */

static void _list_end_ok(_dlg_list *p,mComboBox *cblist)
{
	mColumnItem *pi,*cpi;
	RegFontDat *dat;
	mListItem *item;
	int selid;

	mComboBoxDeleteAllItem(cblist);

	RegFont_clear();

	//

	selid = p->selid;

	for(pi = mListViewGetTopItem(p->list); pi; pi = (mColumnItem *)pi->i.next)
	{
		dat = (RegFontDat *)pi->param;

		//リストアイテム追加
	
		item = RegFont_addDat(dat);

		//mComboBox

		cpi = mComboBoxAddItem_static(cblist, RegFont_getNamePtr(item), (intptr_t)item);

		if(selid != -1 && dat->id == selid)
			mComboBoxSetSelItem(cblist, cpi);
	}
}

/** 登録フォント:リストダイアログ
 *
 * cblist: OK 時、名前のリストをセットする
 * return: TRUE で選択フォントが編集/削除された */

mlkbool RegFontListDlg_run(mWindow *parent,mComboBox *cblist)
{
	_dlg_list *p;
	int ret;

	p = _create_dlg_list(parent, cblist);
	if(!p) return FALSE;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	ret = mDialogRun(MLK_DIALOG(p), FALSE);

	if(ret)
	{
		_list_end_ok(p, cblist);

		ret = p->fselmodify;
	}

	mWidgetDestroy(MLK_WIDGET(p));

	return ret;
}

/** 登録フォント:フォントの編集ダイアログ */

mlkbool RegFontEditDlg_run(mWindow *parent,mComboBox *cblist)
{
	RegFontDat *dat;
	mColumnItem *ci;
	mListItem *item;
	int ret;

	//選択アイテム

	ci = mComboBoxGetSelectItem(cblist);
	if(!ci) return FALSE;

	item = (mListItem *)ci->param;

	//編集

	dat = RegFontDat_make(item);
	if(!dat) return FALSE;

	ret = _fonteditdlg_run(parent, dat);

	//

	if(ret)
	{
		item = RegFont_replaceDat(item, dat);

		ci->param = (intptr_t)item;
	
		mComboBoxSetItemText_static(cblist, ci, RegFont_getNamePtr(item));
	}

	RegFontDat_free(dat);

	return ret;
}


