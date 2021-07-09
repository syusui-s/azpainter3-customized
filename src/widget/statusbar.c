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
 * StatusBar
 * ステータスバー
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_label.h"
#include "mlk_pixbuf.h"
#include "mlk_guicol.h"
#include "mlk_str.h"

#include "def_config.h"
#include "def_widget.h"
#include "def_draw.h"

#include "draw_calc.h"

#include "statusbar.h"

#include "trid.h"


/*
  [!] ステータスバーが非表示の場合、ウィジェットは削除され、
      APPWIDGET->statusbar = NULL となるので、注意。
*/

//----------------------

struct _StatusBar
{
	MLK_CONTAINER_DEF

	mLabel *label_info,
		*label_help;
	mWidget *wg_curpos;

	mStr strtmp; //作業用
};

//----------------------

enum
{
	TRID_TOOLLIST_FREE,
	TRID_DOTPEN_FREE,
	TRID_FINGER_FREE,
	TRID_MOVE,
	TRID_SEL_SELECT,
	TRID_SEL_MOVECOPY,
	TRID_STAMP,
	TRID_SPOIT,

	TRID_LINE,
	TRID_DRAWTYPE_BOX,
	TRID_DRAWTYPE_CIRCLE,
	TRID_DRAWTYPE_SUCCLINE,
	TRID_DRAWTYPE_CONCLINE,
	TRID_DRAWTYPE_BEZIER,
	TRID_POLYGON
};

//描画タイプ (自由線以外)

static const uint8_t g_trid_drawtype[] = {
	TRID_LINE, TRID_DRAWTYPE_BOX, TRID_DRAWTYPE_CIRCLE,
	TRID_DRAWTYPE_SUCCLINE, TRID_DRAWTYPE_CONCLINE, TRID_DRAWTYPE_BEZIER
};

//図形塗りつぶし

static const uint8_t g_trid_fillpoly[] = {
	TRID_DRAWTYPE_BOX, TRID_DRAWTYPE_CIRCLE, TRID_POLYGON, 255
};

//カーソル位置の数字 1bit, (6x9)x12
static const unsigned char g_img_curpos[]={
0x1c,0xc2,0x71,0x90,0xcf,0xf9,0x1c,0x07,0x00,0x22,0x23,0x8a,0x98,0x20,0x82,0xa2,
0x08,0x30,0xa2,0x22,0x8a,0x98,0x20,0x80,0xa2,0x08,0x00,0x22,0x02,0x82,0x94,0x27,
0x40,0xa2,0x08,0x00,0x22,0x02,0x61,0x92,0xe8,0x41,0x1c,0xef,0x31,0x22,0x82,0x80,
0x12,0x28,0x22,0x22,0x08,0x00,0x22,0x42,0x88,0x3e,0x28,0x22,0x22,0x08,0x00,0x22,
0x22,0x88,0x90,0x28,0x22,0xa2,0x08,0x30,0x9c,0xef,0x73,0x10,0xc7,0x21,0x1c,0x07,
0x00 };

//----------------------


//************************************
// カーソル位置
//************************************
/* param1 = x, param2 = y */


#define _CURPOS_PADX 3
#define _CURPOS_PADY 3


/* 数値から画像インデックスをセット */

static uint8_t *_curpos_setbuf(uint8_t *pd,int v)
{
	if(v < 0)
	{
		*(pd++) = 10;
		v = -v;
	}

	if(v >= 10000)
		*(pd++) = (v / 10000) % 10;
	if(v >= 1000)
		*(pd++) = (v / 1000) % 10;
	if(v >= 100)
		*(pd++) = (v / 100) % 10;
	if(v >= 10)
		*(pd++) = (v / 10) % 10;

	*(pd++) = v % 10;

	return pd;
}

/* 描画 */

static void _curpos_draw_handle(mWidget *p,mPixbuf *pixbuf)
{
	uint8_t buf[16],*pd;

	mWidgetDrawBkgnd(p, NULL);

	mPixbufBox(pixbuf, 0, 0, p->w, p->h, MGUICOL_PIX(FRAME_BOX));

	//

	pd = _curpos_setbuf(buf, p->param1);

	*(pd++) = 11;

	pd = _curpos_setbuf(pd, p->param2);

	*(pd++) = 255;

	//

	mPixbufDraw1bitPattern_list(pixbuf, _CURPOS_PADX, _CURPOS_PADY,
		g_img_curpos, 6 * 12, 9, 6, buf, MGUICOL_PIX(TEXT));
}

/* カーソル位置ウィジェット作成 */

static mWidget *_create_curpos(mWidget *parent)
{
	mWidget *p;

	p = mWidgetNew(parent, 0);
	if(!p) return NULL;

	p->flayout = MLF_FIX_WH | MLF_MIDDLE;
	p->w = 6 * (12 + 1) + _CURPOS_PADX * 2;
	p->h = 9 + _CURPOS_PADY * 2;
	p->draw = _curpos_draw_handle;

	if(!(APPCONF->fview & CONFIG_VIEW_F_CURSOR_POS))
		p->fstate &= ~MWIDGET_STATE_VISIBLE;

	return p;
}


//************************************
// StatusBar
//************************************


/* 破棄ハンドラ */

static void _destroy_handle(mWidget *wg)
{
	mStrFree(&((StatusBar *)wg)->strtmp);
}

/** ステータスバー作成 */

void StatusBar_new(void)
{
	StatusBar *p;

	p = (StatusBar *)mContainerNew(
		MLK_WIDGET(APPWIDGET->mainwin), sizeof(StatusBar));

	APPWIDGET->statusbar = p;

	//

	p->wg.flayout = MLF_EXPAND_W;
	p->wg.destroy = _destroy_handle;
	p->wg.draw = mWidgetDrawHandle_bkgnd;
	p->wg.draw_bkgnd = mWidgetDrawBkgndHandle_fillFace;

	mContainerSetType_horz(MLK_CONTAINER(p), 6);
	mContainerSetPadding_same(MLK_CONTAINER(p), 1);

	//ウィジェット

	p->label_info = mLabelCreate(MLK_WIDGET(p), 0, 0, MLABEL_S_BORDER | MLABEL_S_COPYTEXT, NULL);

	p->wg_curpos = _create_curpos(MLK_WIDGET(p));

	p->label_help = mLabelCreate(MLK_WIDGET(p), MLF_BOTTOM | MLF_EXPAND_W, 0, 0, NULL);

	//セット (初期時 or 表示が切り替わった時は再設定)

	StatusBar_setImageInfo();
	StatusBar_setHelp_tool();
}

/** 表示状態切り替え */

void StatusBar_toggleVisible(void)
{
	if(APPWIDGET->statusbar)
	{
		//非表示 -> 削除
		
		mWidgetDestroy(MLK_WIDGET(APPWIDGET->statusbar));
		APPWIDGET->statusbar = NULL;
	}
	else
	{
		//表示 -> 作成
		
		StatusBar_new();
	}

	APPCONF->fview ^= CONFIG_VIEW_F_STATUSBAR;

	mWidgetReLayout(MLK_WIDGET(APPWIDGET->mainwin));
}

/** ステータスバー内のウィジェットの状態が変化した時 */

void StatusBar_layout(void)
{
	StatusBar *p = APPWIDGET->statusbar;

	if(p)
	{
		mWidgetShow(p->wg_curpos, APPCONF->fview & CONFIG_VIEW_F_CURSOR_POS);

		mWidgetReLayout(MLK_WIDGET(p));
	}
}

/** プログレスバーを表示する範囲を取得
 *
 * return: NULL で非表示状態 */

mWidget *StatusBar_getProgressBarPos(mBox *box)
{
	StatusBar *p = APPWIDGET->statusbar;

	if(!p)
		return NULL;
	else
	{
		mWidgetGetBox_rel(MLK_WIDGET(p->label_help), box);
		
		return (mWidget *)p->label_help;
	}
}


//=========================


/** 画像情報セット */

void StatusBar_setImageInfo(void)
{
	StatusBar *p = APPWIDGET->statusbar;

	if(p)
	{
		mStrSetFormat(&p->strtmp, "%d x %d | %d dpi | %dbit",
			APPDRAW->imgw, APPDRAW->imgh, APPDRAW->imgdpi, APPDRAW->imgbits);

		mLabelSetText(p->label_info, p->strtmp.buf);

		mWidgetReLayout(MLK_WIDGET(p));
	}
}

/** カーソル位置セット */

void StatusBar_setCursorPos(int x,int y)
{
	StatusBar *p = APPWIDGET->statusbar;

	if(p
		&& (x != p->wg_curpos->param1 || y != p->wg_curpos->param2))
	{
		p->wg_curpos->param1 = x;
		p->wg_curpos->param2 = y;

		if(APPCONF->fview & CONFIG_VIEW_F_CURSOR_POS)
			mWidgetRedraw(p->wg_curpos);
	}
}

/** ヘルプ欄に矩形選択時の範囲をセット
 *
 * reset: TRUE で元に戻す */

void StatusBar_setHelp_selbox(mlkbool reset)
{
	StatusBar *p = APPWIDGET->statusbar;
	mRect rc;

	if(!p || !(APPCONF->fview & CONFIG_VIEW_F_BOXSEL_POS))
		return;

	if(reset)
		StatusBar_setHelp_tool();
	else
	{
		rc.x1 = APPDRAW->w.pttmp[0].x;
		rc.y1 = APPDRAW->w.pttmp[0].y;
		rc.x2 = APPDRAW->w.pttmp[1].x;
		rc.y2 = APPDRAW->w.pttmp[1].y;

		//イメージ範囲内に

		if(!drawCalc_clipImageRect(APPDRAW, &rc))
			mStrSetText(&p->strtmp, "-");
		else
		{
			mStrSetFormat(&p->strtmp, "(%d, %d) - (%d, %d) [%d x %d]",
				rc.x1, rc.y1, rc.x2, rc.y2,
				rc.x2 - rc.x1 + 1, rc.y2 - rc.y1 + 1);
		}

		mLabelSetText(p->label_help, p->strtmp.buf);
	}
}

/** ツールのヘルプセット */

void StatusBar_setHelp_tool(void)
{
	StatusBar *p = APPWIDGET->statusbar;
	int tool,sub,id;

	if(!p) return;

	tool = APPDRAW->tool.no;
	sub = APPDRAW->tool.subno[tool];

	//id = 文字列ID

	id = -1;

	switch(tool)
	{
		//描画タイプ
		case TOOL_TOOLLIST:
		case TOOL_DOTPEN:
		case TOOL_DOTPEN_ERASE:
		case TOOL_FINGER:
			if(sub != TOOLSUB_DRAW_FREE)
				id = g_trid_drawtype[sub - 1];
			else if(tool == TOOL_TOOLLIST)
				id = TRID_TOOLLIST_FREE;
			else if(tool == TOOL_FINGER)
				id = TRID_FINGER_FREE;
			else
				id = TRID_DOTPEN_FREE;
			break;
			
		//図形塗り
		case TOOL_FILL_POLYGON:
		case TOOL_FILL_POLYGON_ERASE:
			id = g_trid_fillpoly[sub];
			if(id == 255) id = -1;
			break;
		//移動
		case TOOL_MOVE:
			id = TRID_MOVE;
			break;
		//選択範囲
		case TOOL_SELECT:
			if(sub == TOOLSUB_SEL_POSMOVE)
				id = TRID_MOVE;
			else if(sub == TOOLSUB_SEL_IMGMOVE || sub == TOOLSUB_SEL_IMGCOPY)
				id = TRID_SEL_MOVECOPY;
			else
				id = TRID_SEL_SELECT;
			break;
		//矩形選択
		case TOOL_BOXEDIT:
			id = TRID_DRAWTYPE_BOX;
			break;
		//スタンプ
		case TOOL_STAMP:
			id = TRID_STAMP;
			break;
		//グラデーション/キャンバス回転
		case TOOL_GRADATION:
		case TOOL_CANVAS_ROTATE:
			id = TRID_LINE;
			break;
		//スポイト
		case TOOL_SPOIT:
			id = TRID_SPOIT;
			break;
	}

	//セット

	if(id == -1)
		mLabelSetText(p->label_help, NULL);
	else
		mLabelSetText(p->label_help, MLK_TR2(TRGROUP_STATUSBAR_HELP, id));
}

