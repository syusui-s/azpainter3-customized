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
 * [Panel] ImageViewer
 * イメージビューア
 *****************************************/

#include <stdio.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_panel.h"
#include "mlk_iconbar.h"
#include "mlk_sysdlg.h"
#include "mlk_menu.h"
#include "mlk_event.h"
#include "mlk_str.h"
#include "mlk_filelist.h"
#include "mlk_list.h"
#include "mlk_filestat.h"

#include "def_widget.h"
#include "def_config.h"

#include "appresource.h"
#include "image32.h"
#include "panel.h"
#include "filedialog.h"
#include "trid.h"

#include "pv_panel_imgviewer.h"


/* [!] 画像中心位置は、表示倍率変更時などの前にセットすること。 */

//------------------

#define _IS_ZOOM_FIT  (APPCONF->imgviewer.flags & CONFIG_IMAGEVIEWER_F_FIT)
#define _FILELIST_EXTS "png:jpg:jpeg:bmp:gif:tif:tiff:webp:psd"

enum
{
	CMDID_OPEN,
	CMDID_PREV,
	CMDID_NEXT,
	CMDID_CLEAR,
	CMDID_FIT,
	CMDID_FLIP_HORZ,
	CMDID_OPTION,

	CMDID_MENU,
	CMDID_ZOOM,
	
	CMDID_ZOOM_TOP = 1000
};

enum
{
	TRID_OPEN,
	TRID_PREV,
	TRID_NEXT,
	TRID_CLEAR,
	TRID_FIT,
	TRID_FLIP_HORZ,
	TRID_OPTION,

	TRID_TB_MENU = 100,
	TRID_TB_OPEN,
	TRID_TB_PREV,
	TRID_TB_NEXT,
	TRID_TB_ZOOM,
	TRID_TB_FIT,
	TRID_TB_FLIP_HORZ
};

//メニューデータ

static const uint16_t g_menudat[] = {
	TRID_OPEN, TRID_PREV, TRID_NEXT, TRID_CLEAR, MMENU_ARRAY16_SEP,
	TRID_FIT, TRID_FLIP_HORZ, MMENU_ARRAY16_SEP,
	TRID_OPTION, MMENU_ARRAY16_SEP,
	MMENU_ARRAY16_END
};

//------------------

ImgViewerPage *ImgViewerPage_new(mWidget *parent,ImgViewerEx *dat);

int PopupZoomSlider_run(mWidget *parent,mBox *box,
	int max,int current,void (*handle)(int,void *),void *param);

void PanelViewer_optionDlg_run(mWindow *parent,uint8_t *dstbuf,mlkbool is_imageviewer);

//------------------


//=========================
// sub
//=========================


/* アイコンバーの有効/無効セット */

static void _iconbar_enable(ImgViewer *p)
{
	//表示倍率 (全体表示 ON 時は無効)
	
	mIconBarSetEnable(p->iconbar, CMDID_ZOOM, !_IS_ZOOM_FIT);
}

/* 更新 */

static void _update_page(ImgViewer *p)
{
	ImgViewer_setZoom_fit(p);
	
	mWidgetRedraw(MLK_WIDGET(p->page));
}


//==============================
// コマンド
//==============================


/* 開く */

static void _cmd_open(ImgViewer *p)
{
	mStr str = MSTR_INIT;

	if(FileDialog_openImage(
		Panel_getDialogParent(PANEL_IMAGE_VIEWER),
		AppResource_getOpenFileFilter_normal(), 0,
		APPCONF->strImageViewerDir.buf, &str))
	{
		ImgViewer_loadImage(p, str.buf);

		mStrFree(&str);
	}
}

/* 次/前のファイル、リスト追加関数 */

static int _func_filelist_add(const char *fname,const mFileStat *st,void *param)
{
	mStr *pstr = (mStr *)param;

	if(st->flags & MFILESTAT_F_DIRECTORY)
		return 0;

	mStrSetText(pstr, fname);

	return mStrPathCompareExts(pstr, _FILELIST_EXTS); 
}

/* 次/前のファイル */

static void _cmd_prevnext_file(ImgViewer *p,mlkbool fnext)
{
	mList list = MLIST_INIT;
	mStr str = MSTR_INIT;
	mFileListItem *pi;

	if(!p->ex->img) return;

	//リスト作成

	if(!mFileList_create(&list, APPCONF->strImageViewerDir.buf, _func_filelist_add, &str))
		return;

	mFileList_sort_name(&list);

	//

	mStrPathGetBasename(&str, p->ex->strFilename.buf);

	pi = mFileList_find_name(&list, str.buf);

	if(pi)
	{
		pi = (mFileListItem *)((fnext)? pi->i.next: pi->i.prev);
		if(pi)
		{
			mStrPathGetDir(&str, p->ex->strFilename.buf);
			mStrPathJoin(&str, pi->name);

			ImgViewer_loadImage(p, str.buf);
		}
	}

	//

	mListDeleteAll(&list);
	mStrFree(&str);
}

/* 画像をクリア */

static void _clear_image(ImgViewer *p)
{
	Image32_free(p->ex->img);
	p->ex->img = NULL;

	mStrEmpty(&p->ex->strFilename);

	_update_page(p);
}

/* 全体表示切り替え */

static void _cmd_toggle_fit(ImgViewer *p)
{
	APPCONF->imgviewer.flags ^= CONFIG_IMAGEVIEWER_F_FIT;

	ImgViewer_setImageCenter(p, TRUE);

	_iconbar_enable(p);
	_update_page(p);
}

/* 左右反転切り替え */

static void _cmd_toggle_mirror(ImgViewer *p)
{
	ImgViewer_setImageCenter(p, FALSE);

	APPCONF->imgviewer.flags ^= CONFIG_IMAGEVIEWER_F_MIRROR;
	
	_update_page(p);
}

/* 表示倍率スライダー:倍率変更時 */

static void _popupslider_handle(int zoom,void *param)
{
	ImgViewer_setZoom((ImgViewer *)param, zoom);
}

/* 表示倍率スライダー実行 */

static void _popupslider_run(ImgViewer *p)
{
	mBox box;

	//全体表示時は無効

	if(_IS_ZOOM_FIT) return;

	ImgViewer_setImageCenter(p, FALSE);

	//

	mIconBarGetItemBox(p->iconbar, CMDID_ZOOM, &box);
	
	PopupZoomSlider_run(MLK_WIDGET(p->iconbar), &box,
		IMGVIEWER_ZOOM_MAX, p->ex->zoom, _popupslider_handle, p);
}


//==============================
// イベント
//==============================


/* メニュー表示 */

static void _run_menu(ImgViewer *p)
{
	mMenu *menu;
	mBox box;
	int i,zoom_fit;
	char m[16];
	mStr str = MSTR_INIT;

	zoom_fit = _IS_ZOOM_FIT;

	MLK_TRGROUP(TRGROUP_PANEL_IMAGEVIEWER);

	//---- メニュー作成

	menu = mMenuNew();

	//ファイル名

	if(p->ex->img)
	{
		mStrPathGetBasename(&str, p->ex->strFilename.buf);
		
		mMenuAppend(menu, -1, str.buf, 0, MMENUITEM_F_COPYTEXT | MMENUITEM_F_DISABLE);
		mMenuAppendSep(menu);

		mStrFree(&str);
	}

	//共通

	mMenuAppendTrText_array16(menu, g_menudat);

	//チェック

	mMenuSetItemCheck(menu, TRID_FIT, zoom_fit);
	mMenuSetItemCheck(menu, TRID_FLIP_HORZ, APPCONF->imgviewer.flags & CONFIG_IMAGEVIEWER_F_MIRROR);

	//倍率

	for(i = 100; i <= 800; )
	{
		snprintf(m, 16, "%d%%", i);

		mMenuAppend(menu, CMDID_ZOOM_TOP + i, m, 0,
			MMENUITEM_F_COPYTEXT | (zoom_fit ? MMENUITEM_F_DISABLE: 0));

		i += (i == 100)? 100: 200;
	}

	//---- 実行

	mIconBarGetItemBox(p->iconbar, CMDID_MENU, &box);

	mMenuPopup(menu, MLK_WIDGET(p->iconbar), 0, 0, &box,
		MPOPUP_F_RIGHT | MPOPUP_F_TOP | MPOPUP_F_FLIP_XY | MPOPUP_F_MENU_SEND_COMMAND,
		MLK_WIDGET(p));

	mMenuDestroy(menu);
}

/* COMMAND イベント */

static void _event_command(ImgViewer *p,mEventCommand *ev)
{
	int id = ev->id;

	//表示倍率
	
	if(id >= CMDID_ZOOM_TOP)
	{
		ImgViewer_setImageCenter(p, FALSE);
		ImgViewer_setZoom(p, (id - CMDID_ZOOM_TOP) * 10);
		return;
	}

	//

	switch(id)
	{
		//メニュー
		case CMDID_MENU:
			_run_menu(p);
			break;
		//倍率スライダー
		case CMDID_ZOOM:
			_popupslider_run(p);
			break;

		//開く
		case CMDID_OPEN:
			_cmd_open(p);
			break;
		//前のファイル
		case CMDID_PREV:
			_cmd_prevnext_file(p, FALSE);
			break;
		//次のファイル
		case CMDID_NEXT:
			_cmd_prevnext_file(p, TRUE);
			break;
		//クリア
		case CMDID_CLEAR:
			_clear_image(p);
			break;
		//全体表示
		case CMDID_FIT:
			_cmd_toggle_fit(p);

			if(ev->from != MEVENT_COMMAND_FROM_ICONBAR_BUTTON)
				mIconBarSetCheck(p->iconbar, CMDID_FIT, -1);
			break;
		//左右反転
		case CMDID_FLIP_HORZ:
			_cmd_toggle_mirror(p);

			if(ev->from != MEVENT_COMMAND_FROM_ICONBAR_BUTTON)
				mIconBarSetCheck(p->iconbar, CMDID_FLIP_HORZ, -1);
			break;
		//操作設定
		case CMDID_OPTION:
			PanelViewer_optionDlg_run(
				Panel_getDialogParent(PANEL_IMAGE_VIEWER),
				APPCONF->imgviewer.bttcmd, TRUE);
			break;
	}
}

/* イベントハンドラ */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	switch(ev->type)
	{
		case MEVENT_COMMAND:
			_event_command((ImgViewer *)wg, (mEventCommand *)ev);
			break;
	}

	return 1;
}


//=========================
// main
//=========================


/* mPanel 内容作成ハンドラ */

static mWidget *_panel_create_handle(mPanel *panel,int id,mWidget *parent)
{
	ImgViewer *p;
	ImgViewerEx *exdat;
	mIconBar *ib;

	exdat = (ImgViewerEx *)mPanelGetExPtr(panel);

	//メインコンテナ

	p = (ImgViewer *)mContainerNew(parent, sizeof(ImgViewer));

	mContainerSetType_horz(MLK_CONTAINER(p), 0);

	p->wg.flayout = MLF_EXPAND_WH;
	p->wg.event = _event_handle;
	p->wg.notify_to = MWIDGET_NOTIFYTO_SELF;
	p->wg.draw = NULL;

	p->ex = exdat;

	//アイコンバー

	p->iconbar = ib = mIconBarCreate(MLK_WIDGET(p), 0, MLF_EXPAND_H, 0, MICONBAR_S_TOOLTIP | MICONBAR_S_VERT);

	mIconBarSetImageList(ib, APPRES->imglist_icon_other);
	mIconBarSetTooltipTrGroup(ib, TRGROUP_PANEL_IMAGEVIEWER);

	mIconBarAdd(ib, CMDID_MENU, APPRES_OTHERICON_HOME, TRID_TB_MENU, 0);
	mIconBarAdd(ib, CMDID_OPEN, APPRES_OTHERICON_OPEN, TRID_TB_OPEN, 0);
	mIconBarAdd(ib, CMDID_PREV, APPRES_OTHERICON_PREV, TRID_TB_PREV, 0);
	mIconBarAdd(ib, CMDID_NEXT, APPRES_OTHERICON_NEXT, TRID_TB_NEXT, 0);
	mIconBarAdd(ib, CMDID_ZOOM, APPRES_OTHERICON_ZOOM, TRID_TB_ZOOM, 0);
	mIconBarAdd(ib, CMDID_FIT, APPRES_OTHERICON_FIT, TRID_TB_FIT, MICONBAR_ITEMF_CHECKBUTTON);
	mIconBarAdd(ib, CMDID_FLIP_HORZ, APPRES_OTHERICON_FLIP_HORZ, TRID_TB_FLIP_HORZ, MICONBAR_ITEMF_CHECKBUTTON);

	mIconBarSetCheck(ib, CMDID_FIT, APPCONF->imgviewer.flags & CONFIG_IMAGEVIEWER_F_FIT);
	mIconBarSetCheck(ib, CMDID_FLIP_HORZ, APPCONF->imgviewer.flags & CONFIG_IMAGEVIEWER_F_MIRROR);

	_iconbar_enable(p);

	//表示エリア

	p->page = ImgViewerPage_new(MLK_WIDGET(p), exdat);

	return (mWidget *)p;
}

/* mPanel 破棄ハンドラ */

static void _panel_destroy_handle(mPanel *p,void *exdat)
{
	ImgViewerEx *ex = (ImgViewerEx *)exdat;

	Image32_free(ex->img);

	mStrFree(&ex->strFilename);
}

/** 作成 */

void PanelImageViewer_new(mPanelState *state)
{
	mPanel *p;
	ImgViewerEx *ex;

	//パネル作成

	p = Panel_new(PANEL_IMAGE_VIEWER, sizeof(ImgViewerEx), 0, 0,
		state, _panel_create_handle);

	mPanelSetFunc_destroy(p, _panel_destroy_handle);

	//拡張データ

	ex = (ImgViewerEx *)mPanelGetExPtr(p);

	ex->zoom = 1000;
	ex->dscale = ex->dscalediv = 1.0;

	//

	mPanelCreateWidget(p);
}

/** 画像を開く */

void ImgViewer_loadImage(ImgViewer *p,const char *filename)
{
	ImgViewerEx *ex = p->ex;

	Image32_free(ex->img);

	//読み込み

	ex->img = Image32_loadFile(filename, IMAGE32_LOAD_F_BLEND_WHITE);

	if(!ex->img)
		_clear_image(p);
	else
	{
		//ファイル名セット
		
		mStrSetText(&ex->strFilename, filename);

		//ディレクトリ記録

		mStrPathGetDir(&APPCONF->strImageViewerDir, filename);

		//更新

		ImgViewer_setImageCenter(p, TRUE);

		_update_page(p);
	}
}

/** イメージ -> キャンバス位置 */

void ImgViewer_image_to_canvas(ImgViewer *p,mPoint *dst,double x,double y)
{
	ImgViewerEx *ex = p->ex;

	x = (x - ex->ptimgct.x) * ex->dscale;
	y = (y - ex->ptimgct.y) * ex->dscale;

	if(APPCONF->imgviewer.flags & CONFIG_IMAGEVIEWER_F_MIRROR) x = -x;

	dst->x = x + MLK_WIDGET(p->page)->w / 2 - ex->scrx;
	dst->y = y + MLK_WIDGET(p->page)->h / 2 - ex->scry;
}

/** キャンバス -> イメージ位置 */

void ImgViewer_canvas_to_image(ImgViewer *p,mDoublePoint *dst,int x,int y)
{
	ImgViewerEx *ex = p->ex;

	x = x - MLK_WIDGET(p->page)->w / 2 + ex->scrx;
	y = y - MLK_WIDGET(p->page)->h / 2 + ex->scry;

	if(APPCONF->imgviewer.flags & CONFIG_IMAGEVIEWER_F_MIRROR) x = -x;

	dst->x = x * ex->dscalediv + ex->ptimgct.x;
	dst->y = y * ex->dscalediv + ex->ptimgct.y;
}

/** 画像の中心位置をセット
 *
 * reset: TRUE でリセット、FALSE で現在の表示中央 */

void ImgViewer_setImageCenter(ImgViewer *p,mlkbool reset)
{
	ImgViewerEx *ex = p->ex;

	if(!ex->img) return;

	if(reset)
	{
		ex->ptimgct.x = ex->img->w * 0.5;
		ex->ptimgct.y = ex->img->h * 0.5;
	}
	else
	{
		ImgViewer_canvas_to_image(p, &ex->ptimgct,
			MLK_WIDGET(p->page)->w / 2, MLK_WIDGET(p->page)->h / 2);
	}

	ex->scrx = ex->scry = 0;
}

/** 全体表示時、倍率をウィンドウに合わせる (更新なし) */

void ImgViewer_setZoom_fit(ImgViewer *p)
{
	ImgViewerEx *ex = p->ex;
	double h,v;

	if(ex->img && _IS_ZOOM_FIT)
	{
		//倍率が低い方 (等倍以上にはしない)

		h = (double)MLK_WIDGET(p->page)->w / ex->img->w;
		v = (double)MLK_WIDGET(p->page)->h / ex->img->h;

		if(v < h) h = v;
		
		if(h > 1.0) h = 1.0;
		else if(h < 0.001) h = 0.001;

		ex->zoom = (int)(h * 1000 + 0.5);
		if(ex->zoom == 0) ex->zoom = 1;

		ex->dscale = h;
		ex->dscalediv = 1.0 / h;

		ex->scrx = ex->scry = 0;
	}
}

/** 表示倍率セット (更新あり)
 *
 * [!] 画像がない時にも実行される場合あり。
 *
 * zoom: 1000 = 100% */

void ImgViewer_setZoom(ImgViewer *p,int zoom)
{
	ImgViewerEx *ex = p->ex;

	//同じ、または全体表示時

	if(zoom == ex->zoom || _IS_ZOOM_FIT) return;

	//

	ex->zoom = zoom;
	ex->dscale = zoom / 1000.0;
	ex->dscalediv = 1.0 / ex->dscale;

	if(ex->img)
	{
		ImgViewer_adjustScroll(p);

		mWidgetRedraw(MLK_WIDGET(p->page));
	}
}

/** スクロール位置調整 */

void ImgViewer_adjustScroll(ImgViewer *p)
{
	ImgViewerEx *ex = p->ex;
	mDoublePoint dpt;
	mPoint pt1,pt2;
	int imgw,imgh,cw,ch;

	if(!ex->img) return;

	imgw = ex->img->w;
	imgh = ex->img->h;
	cw = MLK_WIDGET(p->page)->w / 2;
	ch = MLK_WIDGET(p->page)->h / 2;

	//キャンバス中央のイメージ位置

	ImgViewer_canvas_to_image(p, &dpt, cw, ch);

	//イメージ端のキャンバス位置

	ImgViewer_image_to_canvas(p, &pt1, 0, 0);
	ImgViewer_image_to_canvas(p, &pt2, imgw, imgh);

	//調整

	if(dpt.x < 0)
		ex->scrx = pt1.x + ex->scrx - cw;
	else if(dpt.x > imgw)
		ex->scrx = pt2.x + ex->scrx - cw;

	if(dpt.y < 0)
		ex->scry = pt1.y + ex->scry - ch;
	else if(dpt.y > imgh)
		ex->scry = pt2.y + ex->scry - ch;
}

