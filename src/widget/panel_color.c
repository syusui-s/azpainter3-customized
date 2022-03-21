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
 * [Panel] カラー
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_panel.h"
#include "mlk_iconbar.h"
#include "mlk_pager.h"
#include "mlk_event.h"

#include "def_widget.h"
#include "def_config.h"

#include "appresource.h"
#include "panel.h"
#include "panel_func.h"
#include "changecol.h"


//-----------------

/* トップコンテナ */

typedef struct
{
	MLK_CONTAINER_DEF

	mWidget *wg_drawcol,	//描画色
		*wg_colmask;		//色マスク
	mPager *pager;

	uint32_t changecol;		//色変更時用の値
}ColorTop;

#define CMDID_COLTYPE_TOP  100  //mIconBar ボタンID

enum
{
	WID_DRAWCOL = 100,
	WID_COLMASK,
	WID_PAGER
};

//_update() 時のフラグ
enum
{
	_UPDATE_F_DRAWCOL = 1<<0,	//描画色ウィジェットを更新
	_UPDATE_F_PAGE = 1<<1,		//各ページの値を更新
	_UPDATE_F_WHEEL = 1<<2,		//カラーホイール更新

	_UPDATE_F_ALL = 0xff
};

//-----------------

mWidget *DrawColorWidget_new(mWidget *parent,int id);
mWidget *ColorMaskWidget_new(mWidget *parent,int id);
void ColorMaskWidget_update(mWidget *wg,int type);

mlkbool PanelColor_createPage_RGB(mPager *p,mPagerInfo *info);
mlkbool PanelColor_createPage_HSV(mPager *p,mPagerInfo *info);
mlkbool PanelColor_createPage_HSL(mPager *p,mPagerInfo *info);
mlkbool PanelColor_createPage_HSVMap(mPager *p,mPagerInfo *info);


static const mFuncPagerCreate g_pagefunc[] = {
	PanelColor_createPage_RGB, PanelColor_createPage_HSV,
	PanelColor_createPage_HSL, PanelColor_createPage_HSVMap
};

//-----------------

/*-----------------

- changecol に、現在色の、元のカラータイプでの値が保持されている。
  HSV -> HSVmap にページが切り替わった時は、changecol が HSV なら、その値がセットされる。

-------------------*/


//===========================
//
//===========================


/* 色変更時の更新 */

static void _update(ColorTop *p,uint8_t flags,uint32_t col)
{
	if(p)
	{
		p->changecol = col;

		//描画色

		if(flags & _UPDATE_F_DRAWCOL)
			mWidgetRedraw(p->wg_drawcol);

		//ページの値

		if(flags & _UPDATE_F_PAGE)
			mPagerSetPageData(p->pager);
	}

	//カラーホイール

	if(flags & _UPDATE_F_WHEEL)
		PanelColorWheel_setColor(col);
}

/* 通知イベント */

static void _event_notify(ColorTop *p,mEvent *ev)
{
	switch(ev->notify.id)
	{
		//各ページからの通知 (色変更時)
		case WID_PAGER:
			_update(p, _UPDATE_F_DRAWCOL | _UPDATE_F_WHEEL, ev->notify.param1);
			break;
		
		//描画色/背景色 入れ替え時
		case WID_DRAWCOL:
			_update(p, _UPDATE_F_PAGE | _UPDATE_F_WHEEL, CHANGECOL_TYPE_RGB);
			break;

		//マスク色からのスポイト時
		case WID_COLMASK:
			_update(p, _UPDATE_F_ALL, CHANGECOL_TYPE_RGB);
			break;
	}
}

/* イベントハンドラ */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	ColorTop *p = (ColorTop *)wg;

	switch(ev->type)
	{
		case MEVENT_NOTIFY:
			_event_notify(p, ev);
			break;

		//カラータイプ変更 -> ページ切り替え
		case MEVENT_COMMAND:
			APPCONF->panel.color_type = ev->cmd.id - CMDID_COLTYPE_TOP;

			mPagerSetPage(p->pager, g_pagefunc[APPCONF->panel.color_type]);

			mWidgetReLayout(p->pager->wg.parent);
			break;
	}

	return 1;
}


//===========================
// main
//===========================


/* ColorTop 取得 */

static ColorTop *_get_colortop(void)
{
	return (ColorTop *)Panel_getContents(PANEL_COLOR);
}

/* mPanel 内容作成ハンドラ */

static mWidget *_panel_create_handle(mPanel *panel,int id,mWidget *parent)
{
	ColorTop *p;
	mWidget *ct;
	mIconBar *ib;
	int i;

	//コンテナ (トップ)

	p = (ColorTop *)mContainerNew(parent, sizeof(ColorTop));

	p->wg.flayout = MLF_EXPAND_WH;
	p->wg.event = _event_handle;
	p->wg.notify_to = MWIDGET_NOTIFYTO_SELF;

	p->changecol = CHANGECOL_RGB;

	mContainerSetPadding_pack4(MLK_CONTAINER(p), MLK_MAKE32_4(3,4,3,4));

	//------ 上部 (描画色・色マスク)

	ct = mContainerCreateHorz(MLK_WIDGET(p), 15, MLF_EXPAND_W, 0);

	mWidgetSetMargin_pack4(ct, MLK_MAKE32_4(0,0,0,9));

	p->wg_drawcol = DrawColorWidget_new(ct, WID_DRAWCOL);
	p->wg_colmask = ColorMaskWidget_new(ct, WID_COLMASK);

	//------ 内容/選択アイコン

	ct = mContainerCreateHorz(MLK_WIDGET(p), 7, MLF_EXPAND_WH, 0);

	//mPager

	p->pager = mPagerCreate(ct, MLF_EXPAND_WH, 0);

	p->pager->wg.id = WID_PAGER;

	mPagerSetDataPtr(p->pager, &p->changecol);
	mPagerSetPage(p->pager, g_pagefunc[APPCONF->panel.color_type]);

	//ボタン

	ib = mIconBarCreate(ct, 0, MLF_EXPAND_H, 0, MICONBAR_S_VERT);

	mIconBarSetImageList(ib, APPRES->imglist_panelcolor_type);

	for(i = 0; i < 4; i++)
		mIconBarAdd(ib, CMDID_COLTYPE_TOP + i, i, -1, MICONBAR_ITEMF_CHECKGROUP);

	mIconBarSetCheck(ib, CMDID_COLTYPE_TOP + APPCONF->panel.color_type, 1);

	return MLK_WIDGET(p);
}

/** 作成 */

void PanelColor_new(mPanelState *state)
{
	mPanel *p;

	p = Panel_new(PANEL_COLOR, 0, 0, 0, state, _panel_create_handle);

	mPanelCreateWidget(p);
}

/** 描画色/背景色 変更時 (RGB)
 *
 * 直接描画色の RGB 値を変更した時。
 * カラーホイールも同時に更新される。 */

void PanelColor_changeDrawColor(void)
{
	_update(_get_colortop(), _UPDATE_F_ALL, CHANGECOL_RGB);
}

/** カラーホイールからの色変更時
 *
 * col: CHANGECOL */

void PanelColor_setColor_fromWheel(uint32_t col)
{
	_update(_get_colortop(), _UPDATE_F_DRAWCOL | _UPDATE_F_PAGE, col);
}

/** ホイール以外(パレット等)からの色変更時
 *
 * col: CHANGECOL */

void PanelColor_setColor_fromOther(uint32_t col)
{
	_update(_get_colortop(), _UPDATE_F_ALL, col);
}

/** マスク色の1番目の色が変更された時 */

void PanelColor_changeColorMask_color1(void)
{
	ColorTop *p = _get_colortop();

	if(p)
		ColorMaskWidget_update(p->wg_colmask, 0);
}

