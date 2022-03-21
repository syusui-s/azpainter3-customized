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

/*******************************************
 * 素材画像選択ダイアログ
 *******************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_button.h"
#include "mlk_listview.h"
#include "mlk_event.h"
#include "mlk_columnitem.h"
#include "mlk_file.h"
#include "mlk_dir.h"
#include "mlk_str.h"
#include "mlk_string.h"

#include "def_macro.h"
#include "def_config.h"

#include "apphelp.h"
#include "appresource.h"
#include "imagematerial.h"

#include "imgmatprev.h"
#include "widget_func.h"

#include "trid.h"


//-------------------------

typedef struct
{
	MLK_DIALOG_DEF

	mStr *strdst,
		str_curdir_rel,   //カレントディレクトリ:相対パス (sysdir/userdir からの相対位置)
		str_curdir_full;  //カレントディレクトリ:フルパス (空文字列でトップ)
	mlkbool is_texture,
		is_sysdir;

	ImageMaterial *img;

	mListView *list;
	ImgMatPrev *prev;
}_dialog;

//-------------------------

enum
{
	WID_LIST = 100,
	WID_HELP
};

//mColumnItem::param
enum
{
	_ITEMPARAM_PARENT,		//親へ移動
	_ITEMPARAM_DIR,			//ディレクトリ
	_ITEMPARAM_FILE,		//画像ファイル
	_ITEMPARAM_TOP_SYSTEM,	//[top] システムディレクトリへ
	_ITEMPARAM_TOP_USER		//[top] ユーザーディレクトリへ
};

//TRGROUP_SELMATERIAL
enum
{
	TRID_SELECT = 4,
	TRID_SELECT_TEXTURE
};

//-------------------------



//===============================
// リスト
//===============================


/* ソート関数 */

static int _sortfunc_list(mListItem *item1,mListItem *item2,void *param)
{
	mColumnItem *p1,*p2;

	p1 = MLK_COLUMNITEM(item1);
	p2 = MLK_COLUMNITEM(item2);

	if(p1->param < p2->param)
		return -1;
	else if(p1->param > p2->param)
		return 1;
	else
		return mStringCompare_number(p1->text, p2->text);
}

/* ファイルリストセット */

static void _set_list_file(_dialog *p)
{
	mListView *list = p->list;
	mDir *dir;
	mStr str = MSTR_INIT;

	//親に戻る

	mListViewAddItem(list, "..", APPRES_FILEICON_PARENT, 0, _ITEMPARAM_PARENT);

	//ファイル

	dir = mDirOpen(p->str_curdir_full.buf);
	if(!dir) return;

	while(mDirNext(dir))
	{
		if(mDirIsSpecName(dir)) continue;

		mDirGetFilename_str(dir, &str, FALSE);

		if(mDirIsDirectory(dir))
		{
			//ディレクトリ
			mListViewAddItem(list, str.buf, APPRES_FILEICON_DIR, MCOLUMNITEM_F_COPYTEXT, _ITEMPARAM_DIR);
		}
		else if(mStrPathCompareExts(&str, "png:jpg:jpeg:gif:bmp"))
		{
			//画像ファイル
			mListViewAddItem(list, str.buf, APPRES_FILEICON_FILE, MCOLUMNITEM_F_COPYTEXT, _ITEMPARAM_FILE);
		}
	}

	mDirClose(dir);

	mStrFree(&str);

	//ソート

	mListViewSortItem(list, _sortfunc_list, 0);
}

/* リストセット */

static void _set_list(_dialog *p)
{
	mListViewDeleteAllItem(p->list);

	if(mStrIsnotEmpty(&p->str_curdir_full))
		//パスあり
		_set_list_file(p);
	else
	{
		//トップ

		mListViewAddItem(p->list, "<system dir>", APPRES_FILEICON_DIR, 0, _ITEMPARAM_TOP_SYSTEM);
		mListViewAddItem(p->list, "<user dir>", APPRES_FILEICON_DIR, 0, _ITEMPARAM_TOP_USER);
	}
}


//===============================
// sub
//===============================


/* ルートディレクトリ取得 */

static void _get_root_dir(_dialog *p,mStr *str)
{
	if(p->is_texture)
	{
		if(p->is_sysdir)
			mGuiGetPath_data(str, APP_DIRNAME_TEXTURE);
		else
			mStrCopy(str, &APPCONF->strUserTextureDir);
	}
	else
	{
		if(p->is_sysdir)
			mGuiGetPath_data(str, APP_DIRNAME_BRUSH);
		else
			mStrCopy(str, &APPCONF->strUserBrushDir);
	}
}

/* 初期パスセット */

static void _set_init_path(_dialog *p)
{
	mStr str = MSTR_INIT;

	mStrCopy(&str, p->strdst);

	if(mStrIsnotEmpty(&str) && !mStrCompareEq(&str, "?"))
	{
		//先頭が '/' でシステムディレクトリ

		if(str.buf[0] == '/')
		{
			mStrSetText(&str, p->strdst->buf + 1);
			p->is_sysdir = TRUE;
		}
		else
			p->is_sysdir = FALSE;

		//カレントディレクトリ

		mStrPathGetDir(&p->str_curdir_rel, str.buf);

		_get_root_dir(p, &p->str_curdir_full);
		mStrPathJoin(&p->str_curdir_full, p->str_curdir_rel.buf);

		//ディレクトリが存在しなければトップ

		if(!mIsExistDir(p->str_curdir_full.buf))
		{
			mStrEmpty(&p->str_curdir_rel);
			mStrEmpty(&p->str_curdir_full);
		}
	}

	mStrFree(&str);
}

/* プレビュー画像読み込み */

static ImageMaterial *_load_image(_dialog *p,const char *filename)
{
	ImageMaterial *img;
	mStr str = MSTR_INIT;

	mStrCopy(&str, &p->str_curdir_full);
	mStrPathJoin(&str, filename);

	if(p->is_texture)
		img = ImageMaterial_loadTexture(&str);
	else
		img = ImageMaterial_loadBrush(&str);

	mStrFree(&str);

	return img;
}


//===============================
// イベント
//===============================


/* OK 終了 */

static void _end_ok(_dialog *p)
{
	mColumnItem *pi;

	pi = mListViewGetFocusItem(p->list);
	if(!pi || pi->param != _ITEMPARAM_FILE) return;

	//ファイル名

	if(p->is_sysdir)
		mStrSetChar(p->strdst, '/');
	else
		mStrEmpty(p->strdst);

	mStrAppendStr(p->strdst, &p->str_curdir_rel);
	mStrPathJoin(p->strdst, pi->text);

	//

	mDialogEnd(MLK_DIALOG(p), TRUE);
}

/* ディレクトリ移動 */

static void _move_dir(_dialog *p,const char *name)
{
	mStrPathJoin(&p->str_curdir_rel, name);
	mStrPathJoin(&p->str_curdir_full, name);

	_set_list(p);
}

/* リスト選択変更 */

static void _list_change_sel(_dialog *p,mColumnItem *pi)
{
	if(pi->param != _ITEMPARAM_FILE)
		//クリア
		ImgMatPrev_setImage(p->prev, NULL);
	else
	{
		//イメージ読み込み

		ImageMaterial_free(p->img);

		p->img = _load_image(p, pi->text);

		//セット

		ImgMatPrev_setImage(p->prev, p->img);
	}
}

/* リストアイテムダブルクリック */

static void _list_dblclk(_dialog *p,mColumnItem *pi)
{
	switch(pi->param)
	{
		//ファイル
		case _ITEMPARAM_FILE:
			_end_ok(p);
			break;

		//ディレクトリ
		case _ITEMPARAM_DIR:
			_move_dir(p, pi->text);
			break;

		//親へ
		case _ITEMPARAM_PARENT:
			if(mStrIsEmpty(&p->str_curdir_rel))
				//システム/ユーザーのトップなら、最初に戻る
				mStrEmpty(&p->str_curdir_full);
			else
			{
				mStrPathRemoveBasename(&p->str_curdir_rel);
				mStrPathRemoveBasename(&p->str_curdir_full);
			}

			_set_list(p);
			break;

		//[top] system/user へ
		case _ITEMPARAM_TOP_USER:
		case _ITEMPARAM_TOP_SYSTEM:
			p->is_sysdir = (pi->param == _ITEMPARAM_TOP_SYSTEM);

			mStrEmpty(&p->str_curdir_rel);
			_get_root_dir(p, &p->str_curdir_full);

			_set_list(p);
			break;
	}
}

/* イベントハンドラ */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	_dialog *p = (_dialog *)wg;

	if(ev->type == MEVENT_NOTIFY)
	{
		switch(ev->notify.id)
		{
			//リスト
			case WID_LIST:
				if(ev->notify.notify_type == MLISTVIEW_N_CHANGE_FOCUS)
					//フォーカス変更
					_list_change_sel(p, (mColumnItem *)ev->notify.param1);
				else if(ev->notify.notify_type == MLISTVIEW_N_ITEM_L_DBLCLK)
					//ダブルクリック
					_list_dblclk(p, (mColumnItem *)ev->notify.param1);
				break;

			//ヘルプ
			case WID_HELP:
				AppHelp_message(MLK_WINDOW(wg), HELP_TRGROUP_SINGLE, HELP_TRID_SEL_MATERIAL);
				break;
		
			//OK
			case MLK_WID_OK:
				_end_ok(p);
				break;
			//キャンセル
			case MLK_WID_CANCEL:
				mDialogEnd(MLK_DIALOG(wg), FALSE);
				break;
		}
	}

	return mDialogEventDefault(wg, ev);
}


//===============================
//
//===============================


/* 破棄ハンドラ */

static void _destroy_handle(mWidget *wg)
{
	_dialog *p = (_dialog *)wg;

	mStrFree(&p->str_curdir_full);
	mStrFree(&p->str_curdir_rel);

	ImageMaterial_free(p->img);
}

/* ダイアログ作成 */

static _dialog *_create_dlg(mWindow *parent,mStr *strdst,mlkbool is_texture)
{
	_dialog *p;
	mWidget *ct,*wg;

	p = (_dialog *)mDialogNew(parent, sizeof(_dialog), MTOPLEVEL_S_DIALOG_NORMAL);
	if(!p) return NULL;

	p->strdst = strdst;
	p->is_texture = is_texture;

	p->wg.destroy = _destroy_handle;
	p->wg.event = _event_handle;

	//

	mContainerSetType_horz(MLK_CONTAINER(p), 8);
	mContainerSetPadding_same(MLK_CONTAINER(p), 6);

	mToplevelSetTitle(MLK_TOPLEVEL(p),
		MLK_TR2(TRGROUP_SELMATERIAL, is_texture? TRID_SELECT_TEXTURE: TRID_SELECT));

	//---- リスト

	p->list = mListViewCreate(MLK_WIDGET(p), WID_LIST, MLF_EXPAND_WH, 0,
		MLISTVIEW_S_AUTO_WIDTH | MLISTVIEW_S_DESTROY_IMAGELIST,
		MSCROLLVIEW_S_HORZVERT_FRAME);

	mListViewSetImageList(p->list, AppResource_createImageList_fileicon());

	mWidgetSetInitSize_fontHeightUnit(MLK_WIDGET(p->list), 18, 22);

	//---- 右側

	ct = mContainerCreateVert(MLK_WIDGET(p), 0, MLF_EXPAND_H, 0);

	//プレビュー

	p->prev = ImgMatPrev_new(ct, 150, 150, 0);

	//ヘルプ

	widget_createHelpButton(ct, WID_HELP, MLF_EXPAND_Y | MLF_RIGHT, MLK_MAKE32_4(0,6,0,0));

	//OK

	wg = (mWidget *)mButtonCreate(ct, MLK_WID_OK, MLF_EXPAND_W, MLK_MAKE32_4(0,6,0,3), 0,
		MLK_TR_SYS(MLK_TRSYS_OK));

	wg->fstate |= MWIDGET_STATE_ENTER_SEND;

	//キャンセル

	mButtonCreate(ct, MLK_WID_CANCEL, MLF_EXPAND_W, 0, 0, MLK_TR_SYS(MLK_TRSYS_CANCEL));

	//--------

	_set_init_path(p);
	_set_list(p);

	return p;
}

/** ダイアログ実行
 *
 * 画像が選択されていなければ、OK は押せない。 */

mlkbool SelMaterialDlg_run(mWindow *parent,mStr *strdst,mlkbool is_texture)
{
	_dialog *p;

	p = _create_dlg(parent, strdst, is_texture);
	if(!p) return FALSE;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	return mDialogRun(MLK_DIALOG(p), TRUE);
}

