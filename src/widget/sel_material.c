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
 * SelMaterial
 * 素材画像の選択ウィジェット
 *****************************************/

#include <string.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_pixbuf.h"
#include "mlk_guicol.h"
#include "mlk_font.h"
#include "mlk_event.h"
#include "mlk_str.h"
#include "mlk_menu.h"

#include "sel_material.h"

#include "trid.h"


/*
 * - 選択が変更された時、通知イベントが送られる。
 *   param1 = パス名 (char *)。なしの場合 NULL
 */

//----------------------

struct _SelMaterial
{
	mWidget wg;

	int type;
	mStr strtext,	//表示用テキスト
		strpath;	//パス名
};

//----------------------

#define SELMATERIAL(p)  ((SelMaterial *)(p))

#define _PAD_X     3
#define _PAD_Y     3
#define _MENUBTT_W 13

enum
{
	TRID_NONE,
	TRID_NONE_FORCE,
	TRID_OPT_TEXTURE,
	TRID_SHAPE_NORMAL,
	TRID_SELECT,
	TRID_SELECT_TEXTURE
};

//メニュー
static const uint16_t g_menudat[3][4] = {
	{TRID_NONE, TRID_SELECT_TEXTURE, 0xffff, 0}, //オプション/レイヤテクスチャ
	{TRID_OPT_TEXTURE, TRID_NONE_FORCE, TRID_SELECT, 0xffff}, //ブラシテクスチャ
	{TRID_SHAPE_NORMAL, TRID_SELECT, 0xffff, 0} //ブラシ形状
};

//----------------------


//************************************
// SelMaterial
//************************************


/* 通知 */

static void _notify(SelMaterial *p)
{
	mWidgetEventAdd_notify(MLK_WIDGET(p), NULL,
		0, (intptr_t)(mStrIsEmpty(&p->strpath)? 0: p->strpath.buf), 0);
}

/* ダイアログ実行 */

static void _run_dialog(SelMaterial *p)
{
	mStr str = MSTR_INIT;

	mStrCopy(&str, &p->strpath);

	if(SelMaterialDlg_run(MLK_WINDOW(p->wg.toplevel), &str,
		(p->type != SELMATERIAL_TYPE_BRUSH_SHAPE)))
	{
		mStrCopy(&p->strtext, &str);
		mStrCopy(&p->strpath, &str);

		mWidgetRedraw(MLK_WIDGET(p));

		_notify(p);
	}

	mStrFree(&str);
}

/* メニュー実行 */

static void _run_menu(SelMaterial *p)
{
	mMenu *menu;
	mMenuItem *mi;
	mBox box;
	int id;

	//メニュー

	MLK_TRGROUP(TRGROUP_SELMATERIAL);

	menu = mMenuNew();

	mMenuAppendTrText_array16(menu, g_menudat[p->type]);

	mWidgetGetBox_rel(MLK_WIDGET(p), &box);

	mi = mMenuPopup(menu, MLK_WIDGET(p), 0, 0, &box,
		MPOPUP_F_RIGHT | MPOPUP_F_BOTTOM | MPOPUP_F_GRAVITY_LEFT | MPOPUP_F_FLIP_Y, NULL);

	id = (mi)? mMenuItemGetID(mi): -1;

	mMenuDestroy(menu);

	//セット

	if(id == -1) return;

	if(id == TRID_SELECT || id == TRID_SELECT_TEXTURE)
		_run_dialog(p);
	else
	{
		//"画像選択" 以外
		// なし => NULL
		// オプションテクスチャを使う => "?"
		
		SelMaterial_setPath(p, (id == TRID_OPT_TEXTURE)? "?": NULL);

		_notify(p);
	}
}

/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_POINTER
		&& ev->pt.act == MEVENT_POINTER_ACT_PRESS)
	{
		SelMaterial *p = SELMATERIAL(wg);
	
		//------ 押し時
	
		if(ev->pt.btt == MLK_BTT_RIGHT
			|| (ev->pt.btt == MLK_BTT_LEFT && (ev->pt.state & MLK_STATE_CTRL)))
		{
			//右ボタン or Ctrl+左でリセット

			if(mStrIsnotEmpty(&p->strpath))
			{
				SelMaterial_setPath(p, NULL);

				_notify(p);
			}
		}
		else if(ev->pt.btt == MLK_BTT_LEFT)
		{
			//左ボタン

			if(ev->pt.x >= wg->w - 1 - _MENUBTT_W)
				_run_menu(p);
			else
				_run_dialog(p);
		}
	}
	
	return 1;
}

/* 描画 */

static void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	int w,h;
	mlkbool enable;

	enable = mWidgetIsEnable(wg);

	w = wg->w;
	h = wg->h;

	//ボタン背景

	mPixbufDrawButton(pixbuf, 0, 0, w, h,
		(enable)? 0: MPIXBUF_DRAWBTT_DISABLED);

	//メニューボタン

	mPixbufLineV(pixbuf, w - 2 - _MENUBTT_W, 2, h - 4,
		mGuiCol_getPix(enable? MGUICOL_FRAME: MGUICOL_FRAME_DISABLE));

	mPixbufDrawArrowDown(pixbuf, w - 1 - _MENUBTT_W + (_MENUBTT_W - 7) / 2, (h - 4) / 2,
		4, mGuiCol_getPix(enable? MGUICOL_TEXT: MGUICOL_TEXT_DISABLE));

	//テキスト

	if(enable
		&& mPixbufClip_setBox_d(pixbuf, 3, 0, w - 2 - _PAD_X * 2 - _MENUBTT_W, h))
	{
		mFontDrawText_pixbuf(mWidgetGetFont(wg), pixbuf,
			1 + _PAD_X, _PAD_Y, SELMATERIAL(wg)->strtext.buf, -1, MGUICOL_RGB(TEXT));
	}
}


//========================
// main
//========================


/* 推奨サイズ計算 */

static void _calchint_handle(mWidget *wg)
{
	wg->hintH = mWidgetGetFontHeight(wg) + _PAD_Y * 2;
	wg->hintW = wg->hintH + _MENUBTT_W + 5;
}

/* 破棄ハンドラ */

static void _destroy_handle(mWidget *wg)
{
	SelMaterial *p = SELMATERIAL(wg);

	mStrFree(&p->strtext);
	mStrFree(&p->strpath);
}

/** 作成 */

SelMaterial *SelMaterial_new(mWidget *parent,int id,int type)
{
	SelMaterial *p;

	p = (SelMaterial *)mWidgetNew(parent, sizeof(SelMaterial));
	if(!p) return NULL;

	p->wg.id = id;
	p->wg.flayout = MLF_EXPAND_W;
	p->wg.fevent |= MWIDGET_EVENT_POINTER;

	p->wg.destroy = _destroy_handle;
	p->wg.draw = _draw_handle;
	p->wg.event = _event_handle;
	p->wg.calc_hint = _calchint_handle;
	
	p->type = type;

	return p;
}

/** パスを取得 */

void SelMaterial_getPath(SelMaterial *p,mStr *str)
{
	mStrCopy(str, &p->strpath);
}

/** パスをセット
 *
 * name: NULL または空文字列でなし */

void SelMaterial_setPath(SelMaterial *p,const char *path)
{
	int id = -1;
	mlkbool empty;

	mStrSetText(&p->strpath, path);

	//表示名

	empty = (!path || !path[0]);

	switch(p->type)
	{
		//テクスチャ
		case SELMATERIAL_TYPE_TEXTURE:
			if(empty) id = TRID_NONE;
			break;
		//ブラシテクスチャ
		case SELMATERIAL_TYPE_BRUSH_TEXTURE:
			if(empty)
				id = TRID_NONE_FORCE;
			else if(strcmp(path, "?") == 0)
				id = TRID_OPT_TEXTURE;
			break;
		//ブラシ形状
		case SELMATERIAL_TYPE_BRUSH_SHAPE:
			if(empty)
				id = TRID_SHAPE_NORMAL;
			break;
	}

	if(id == -1)
		mStrSetText(&p->strtext, path);
	else
		mStrSetText(&p->strtext, MLK_TR2(TRGROUP_SELMATERIAL, id));

	//

	mWidgetRedraw(MLK_WIDGET(p));
}


//************************************
// レイヤパネル上のテクスチャ選択
//************************************

/* mWidget::param1 = 画像があるかどうか
 *
 * <通知>
 *  type: [0] なし [1] 画像選択
 */

#define _TEX_IMGW  15
#define _TEX_IMGH  8
#define _TEX_PADX  4

//15x8
static const unsigned char g_img_tex[]={
0xef,0x6d,0xef,0x6d,0x66,0x38,0xe6,0x39,0xe6,0x39,0x66,0x38,0xe6,0x6d,0xe6,0x6d };


/* メニュー */

static void _tex_run_menu(mWidget *p)
{
	mMenu *menu;
	mMenuItem *mi;
	mBox box;
	int id;

	MLK_TRGROUP(TRGROUP_SELMATERIAL);

	menu = mMenuNew();

	mMenuAppendTrText_array16(menu, g_menudat[0]);

	mMenuSetItemEnable(menu, TRID_NONE, (p->param1 != 0));

	mWidgetGetBox_rel(p, &box);

	mi = mMenuPopup(menu, p, 0, 0, &box,
		MPOPUP_F_RIGHT | MPOPUP_F_BOTTOM | MPOPUP_F_GRAVITY_LEFT | MPOPUP_F_FLIP_XY, NULL);

	id = (mi)? mMenuItemGetID(mi): -1;

	mMenuDestroy(menu);

	//通知

	if(id != -1)
		mWidgetEventAdd_notify(p, NULL, (id != TRID_NONE), 0, 0);
}

/* イベント */

static int _tex_event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_POINTER
		&& ev->pt.act == MEVENT_POINTER_ACT_PRESS)
	{
		if(ev->pt.btt == MLK_BTT_RIGHT
			|| (ev->pt.btt == MLK_BTT_LEFT && (ev->pt.state & MLK_STATE_CTRL)))
		{
			//右ボタン or Ctrl+左: 画像選択

			mWidgetEventAdd_notify(wg, NULL, 1, 0, 0);
		}
		else if(ev->pt.btt == MLK_BTT_LEFT)
		{
			//左ボタン

			if(wg->param1)
				//画像があればメニュー
				_tex_run_menu(wg);
			else
				//画像がなければ、選択
				mWidgetEventAdd_notify(wg, NULL, 1, 0, 0);
		}
	}

	return 1;
}

/* 描画 */

static void _tex_draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	mlkbool enable;
	uint32_t col;

	enable = mWidgetIsEnable(wg);

	//ボタン背景

	mPixbufDrawButton(pixbuf, 0, 0, wg->w, wg->h,
		(enable)? 0: MPIXBUF_DRAWBTT_DISABLED);

	//tex

	if(!enable)
		col = MGUICOL_PIX(TEXT_DISABLE);
	else if(!wg->param1)
		col = MGUICOL_PIX(TEXT);
	else
	{
		col = MGUICOL_PIX(TEXT_SELECT);

		mPixbufFillBox(pixbuf, _TEX_PADX - 1, (wg->h - _TEX_IMGH) / 2 - 1,
			_TEX_IMGW + 2, _TEX_IMGH + 2, MGUICOL_PIX(FACE_SELECT));
	}

	mPixbufDraw1bitPattern(pixbuf, _TEX_PADX, (wg->h - _TEX_IMGH) / 2,
		g_img_tex, _TEX_IMGW, _TEX_IMGH, MPIXBUF_TPCOL, col);
}

/** 作成 */

mWidget *SelMaterialTex_new(mWidget *parent,int id)
{
	mWidget *p;

	p = mWidgetNew(parent, 0);

	p->id = id;
	p->w = _TEX_IMGW + _TEX_PADX * 2;
	p->hintH = _TEX_IMGH + 4;
	p->flayout = MLF_MIDDLE | MLF_FIX_W | MLF_EXPAND_H;
	p->fevent = MWIDGET_EVENT_POINTER;
	p->draw = _tex_draw_handle;
	p->event = _tex_event_handle;

	return p;
}

/** テクスチャがあるかどうかのフラグをセット */

void SelMaterialTex_setFlag(mWidget *p,int flag)
{
	p->param1 = flag;

	mWidgetRedraw(p);
}


