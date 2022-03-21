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
 * パレット編集ダイアログ
 *****************************************/

#include <string.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_label.h"
#include "mlk_button.h"
#include "mlk_lineedit.h"
#include "mlk_scrollbar.h"
#include "mlk_iconbar.h"
#include "mlk_event.h"
#include "mlk_pixbuf.h"
#include "mlk_key.h"
#include "mlk_sysdlg.h"
#include "mlk_str.h"
#include "mlk_imagelist.h"

#include "colorvalue.h"
#include "palettelist.h"
#include "trid.h"
#include "trid_colorpalette.h"


//----------------------

//作業用データ
typedef struct
{
	uint8_t *palbuf;	//作業用パレットバッファ (最大数分確保)
	int curno,		//現在のカーソル位置
		seltop,		//選択先頭位置。-1 で選択なし (カーソル位置一つのみ)
		selend,
		palnum;
}_data;

//パレットリスト
typedef struct
{
	mWidget wg;

	_data *pdat;
	mScrollBar *scr;

	int xnum,	//横に並べる数
		fdrag,
		dragno;	//D&D挿入先。-1 で移動先無効。
}_pallist;

typedef struct
{
	MLK_DIALOG_DEF

	_pallist *list;
	mScrollBar *scr;
	mLineEdit *edit_rgb;

	_data dat;
}_dialog;

//----------------------

enum
{
	WID_SCROLL = 100,
	WID_EDIT_RGB
};

enum
{
	_DRAGF_NONE,
	_DRAGF_PRESS,
	_DRAGF_DRAG
};

#define _CELLW   17
#define _CELLH   17
#define _COL_BKGND     0xaaaaaa	//背景色
#define _COL_SEL       0x845DFF	//選択色
#define _COL_CURFRAME  0xff0000	//現在位置の枠色
#define _COL_INSERT    0xff0000	//挿入位置

//----------------------


/***********************************
 * データ処理
 ***********************************/


/* 処理する範囲を取得 */

static void _data_get_sel(_data *p,int *top,int *end)
{
	if(p->seltop == -1)
		*top = *end = p->curno;
	else
		*top = p->seltop, *end = p->selend;
}

/* 指定位置がカーソル位置または選択範囲内か */

static mlkbool _data_is_in_cursel(_data *p,int no)
{
	return (no == p->curno
		|| (p->seltop != -1 && p->seltop <= no && no <= p->selend));
}

/* カーソル位置から指定位置までを選択範囲にする */

static void _data_select(_data *p,int no)
{
	int top,end;

	if(no == p->curno)
		//カーソル位置なら選択解除
		p->seltop = -1;
	else
	{
		if(no <= p->curno)
			top = no, end = p->curno;
		else
			top = p->curno, end = no;

		p->seltop = top;
		p->selend = end;
	}
}

/* 挿入先位置を調整して返す
 *
 * return: -1 でなし (移動無効) */

static int _data_adjust_insert(_data *p,int no)
{
	//範囲内、または範囲終端の次の位置の場合はなし
	// :移動先が範囲終端の次の位置の場合、移動後の状態は同じになるため無効にする。

	if(_data_is_in_cursel(p, no)
		|| (p->seltop != -1 && p->selend + 1 == no)
		|| (p->seltop == -1 && p->curno + 1 == no))
		return -1;
	else
		return no;
}

/* 終端に色を追加 */

static mlkbool _data_append(_data *p,int num)
{
	int last;

	last = p->palnum;

	//追加数調整

	if(last + num > PALETTE_MAX_COLNUM)
		num = PALETTE_MAX_COLNUM - last;

	if(num == 0) return FALSE;

	//追加&クリア

	p->palnum += num;

	memset(p->palbuf + last * 3, 255, num * 3);

	return TRUE;
}

/* 色を挿入追加 (カーソル位置からN個) */

static mlkbool _data_insert(_data *p,int num)
{
	int last,i;
	uint8_t *p1,*p2;

	last = p->palnum;

	//追加数調整

	if(last + num > PALETTE_MAX_COLNUM)
		num = PALETTE_MAX_COLNUM - last;

	if(num == 0) return FALSE;

	//追加

	p->palnum += num;

	//挿入位置を空ける

	p1 = p->palbuf + (p->palnum - 1) * 3;
	p2 = p->palbuf + (last - 1) * 3;

	for(i = last - p->curno; i >= 0; i--)
	{
		p1[0] = p2[0];
		p1[1] = p2[1];
		p1[2] = p2[2];
	
		p1 -= 3;
		p2 -= 3;
	}

	//新規分をクリア

	memset(p->palbuf + p->curno * 3, 255, num * 3);

	//

	p->curno += num;
	p->seltop = -1;

	return TRUE;
}

/* 選択範囲の色を削除 */

static mlkbool _data_delete(_data *p)
{
	uint8_t *p1,*p2;
	int top,end,num,delnum;

	num = p->palnum;
	if(num == 1) return FALSE;

	_data_get_sel(p, &top, &end);

	//すべて削除の場合は、一つ残す

	if(top == 0 && end == num - 1)
		top = 1;

	//詰める

	end++;
	delnum = end - top;

	p1 = p->palbuf + top * 3;
	p2 = p->palbuf + end * 3;

	for(; end < num; end++)
	{
		p1[0] = p2[0];
		p1[1] = p2[1];
		p1[2] = p2[2];
	
		p1 += 3;
		p2 += 3;
	}

	p->palnum -= delnum;

	//

	p->curno = (top >= p->palnum)? p->palnum - 1: top;
	p->seltop = -1;

	return TRUE;
}

/* 選択間をグラデーション */

static mlkbool _data_gradation(_data *p)
{
	int top,end,i,j,len,diff[3];
	uint8_t *p1,*p2;
	double d;

	_data_get_sel(p, &top, &end);

	//長さ

	len = end - top;
	if(len < 2) return FALSE;

	//RGB値の差

	p1 = p->palbuf + top * 3;
	p2 = p->palbuf + end * 3;

	for(i = 0; i < 3; i++)
		diff[i] = p2[i] - p1[i];

	//グラデーション

	p2 = p1 + 3;

	for(i = 1; i < len; i++, p2 += 3)
	{
		d = (double)i / len;
	
		for(j = 0; j < 3; j++)
			p2[j] = (int)(diff[j] * d + p1[j] + 0.5);
	}

	return TRUE;
}

/* 選択範囲の色を指定位置へ移動 */

static void _data_dnd_move(_data *p,int dstno)
{
	uint8_t *buf,*p1,*p2;
	int top,end,num;

	_data_get_sel(p, &top, &end);

	num = end - top + 1;

	//コピー

	buf = (uint8_t *)mMalloc(num * 3);
	if(!buf) return;

	memcpy(buf, p->palbuf + top * 3, num * 3);

	//削除

	_data_delete(p);

	//挿入

	if(dstno > top)
		dstno -= num;

	p->curno = dstno;

	_data_insert(p, num);

	//挿入位置に貼り付け

	p1 = p->palbuf + dstno * 3;
	p2 = buf;

	for(; num > 0; num--)
	{
		p1[0] = p2[0];
		p1[1] = p2[1];
		p1[2] = p2[2];
	
		p1 += 3;
		p2 += 3;
	}

	//

	mFree(buf);

	p->curno = dstno;
}


/***********************************
 * _pallist
 ***********************************/


/* スクロール情報セット */

static void _list_set_scroll(_pallist *p)
{
	int page;

	page = p->wg.h / _CELLH;
	if(page <= 0) page = 1;

	mScrollBarSetStatus(p->scr, 0, (p->pdat->palnum + p->xnum - 1) / p->xnum, page);
}

/* カーソル位置からパレット番号取得
 *
 * return: -1 で範囲外 */

static int _list_getpalno(_pallist *p,int x,int y)
{
	int no;

	if(x < 0) x = 0;
	if(y < 0) y = 0;

	x = x / _CELLW;
	y = y / _CELLH + mScrollBarGetPos(p->scr);

	if(x >= p->xnum) x = p->xnum - 1;

	no = x + y * p->xnum;

	if(no >= p->pdat->palnum) return -1;

	return no;
}

/* カーソル位置変更 */

static void _list_setcurpos(_pallist *p,int no)
{
	p->pdat->curno = no;
	p->pdat->seltop = -1;

	mWidgetRedraw(MLK_WIDGET(p));
}


//===============


/* ボタン押し時 */

static void _list_on_press(_pallist *p,mEventPointer *ev)
{
	int no;

	no = _list_getpalno(p, ev->x, ev->y);
	if(no == -1) return;

	if((ev->btt == MLK_BTT_LEFT && (ev->state & MLK_STATE_SHIFT))
		|| ev->btt == MLK_BTT_RIGHT)
	{
		//Shift+L or Right: 指定位置まで選択

		_data_select(p->pdat, no);
		
		mWidgetRedraw(MLK_WIDGET(p));
	}
	else if(ev->btt == MLK_BTT_LEFT)
	{
		//カーソル位置変更

		if(!_data_is_in_cursel(p->pdat, no))
			_list_setcurpos(p, no);

		//D&D判定開始

		p->fdrag = _DRAGF_PRESS;
		p->dragno = no;

		mWidgetGrabPointer(MLK_WIDGET(p));
	}
}

/* 移動時 */

static void _list_on_motion(_pallist *p,int x,int y)
{
	int no;

	no = _list_getpalno(p, x, y);

	//D&D開始の判定中
	// :押し時の位置からはずれたらD&D開始

	if(p->fdrag == _DRAGF_PRESS && no != p->dragno)
	{
		p->fdrag = _DRAGF_DRAG;
		p->dragno = -1;
	}

	//D&D中

	if(p->fdrag == _DRAGF_DRAG)
	{
		//範囲外は終端へ
		
		if(no == -1) no = p->pdat->palnum;

		//挿入先調整 (ここで -1 の場合、挿入先無効)

		no = _data_adjust_insert(p->pdat, no);

		//挿入先変更
		
		if(no != p->dragno)
		{
			p->dragno = no;
			mWidgetRedraw(MLK_WIDGET(p));
		}
	}
}

/* グラブ解放 */

static void _list_ungrab(_pallist *p)
{
	if(p->fdrag)
	{
		if(p->fdrag == _DRAGF_PRESS)
		{
			//ドラッグ判定中で、位置移動なし
			// :選択範囲内を押した場合は、カーソル位置が変わっていないので変更

			if(p->pdat->curno != p->dragno)
				_list_setcurpos(p, p->dragno);
		}
		else if(p->fdrag == _DRAGF_DRAG)
		{
			//D&D

			if(p->dragno != -1)
			{
				_data_dnd_move(p->pdat, p->dragno);
				mWidgetRedraw(MLK_WIDGET(p));
			}
		}

		//
	
		p->fdrag = 0;
		mWidgetUngrabPointer();
	}
}


//===============


/* イベントハンドラ */

static int _list_event_handle(mWidget *wg,mEvent *ev)
{
	_pallist *p = (_pallist *)wg;

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.act == MEVENT_POINTER_ACT_MOTION)
			{
				if(p->fdrag)
					_list_on_motion(p, ev->pt.x, ev->pt.y);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_PRESS)
			{
				if(p->fdrag == 0)
					_list_on_press(p, (mEventPointer *)ev);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_RELEASE)
			{
				if(ev->pt.btt == MLK_BTT_LEFT)
					_list_ungrab(p);
			}
			break;

		//垂直ホイール
		case MEVENT_SCROLL:
			if(ev->scroll.vert_step)
			{
				if(mScrollBarAddPos(p->scr, ev->scroll.vert_step * 3))
					mWidgetRedraw(wg);
			}
			break;

		case MEVENT_FOCUS:
			if(ev->focus.is_out)
				_list_ungrab(p);
			break;
	}

	return 1;
}

/* 描画 */

static void _list_draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	_pallist *p = (_pallist *)wg;
	_data *data;
	uint8_t *ps;
	int ix,h,x,y,no,num,seltop,selend,curno,dragno;
	mPixCol colsel;

	data = p->pdat;
	h = wg->h;

	//背景

	mPixbufFillBox(pixbuf, 0, 0, wg->w, h, mRGBtoPix(_COL_BKGND));

	//パレット

	no = mScrollBarGetPos(p->scr) * p->xnum;
	ps = data->palbuf + no * 3;
	num = data->palnum;

	curno = data->curno;
	seltop = data->seltop;
	selend = data->selend;
	dragno = (p->fdrag == _DRAGF_DRAG)? p->dragno: -1;

	colsel = mRGBtoPix(_COL_SEL);

	for(y = 0; y < h; y += _CELLH)
	{
		for(ix = p->xnum, x = 0; ix > 0; ix--, x += _CELLW, ps += 3, no++)
		{
			if(no >= num)
			{
				//挿入先が終端
				
				if(no == dragno)
					mPixbufLineV(pixbuf, x, y, _CELLH, mRGBtoPix(_COL_INSERT));

				return;
			}

			//選択背景

			if(seltop != -1 && seltop <= no && no <= selend)
				mPixbufFillBox(pixbuf, x, y, _CELLW, _CELLH, colsel);

			//カーソル枠

			if(no == curno)
				mPixbufBox(pixbuf, x + 1, y + 1, _CELLW - 2, _CELLH - 2, mRGBtoPix(_COL_CURFRAME));

			//ドラッグ挿入先

			if(no == dragno)
				mPixbufLineV(pixbuf, x, y, _CELLH, mRGBtoPix(_COL_INSERT));

			//枠

			mPixbufBox(pixbuf, x + 2, y + 2, _CELLW - 4, _CELLH - 4, 0);

			//色

			mPixbufFillBox(pixbuf, x + 3, y + 3, _CELLW - 6, _CELLH - 6,
				mRGBtoPix_sep(ps[0], ps[1], ps[2]));
		}
	}
}

/* サイズ変更時 */

static void _list_resize(mWidget *wg)
{
	_pallist *p = (_pallist *)wg;

	p->xnum = wg->w / _CELLW;
	if(p->xnum < 1) p->xnum = 1;

	_list_set_scroll(p);
}

/* パレットリスト作成 */

static _pallist *_create_pallist(mWidget *parent)
{
	_pallist *p;

	p = (_pallist *)mWidgetNew(parent, sizeof(_pallist));

	p->wg.flayout = MLF_EXPAND_WH;
	p->wg.fevent |= MWIDGET_EVENT_POINTER | MWIDGET_EVENT_SCROLL;
	p->wg.event = _list_event_handle;
	p->wg.draw = _list_draw_handle;
	p->wg.resize = _list_resize;
	p->wg.initW = _CELLW * 16;
	p->wg.initH = _CELLH * 16;

	return p;
}


/***********************************
 * ダイアログ
 ***********************************/


/* 編集後のリスト更新 */

static void _update_list(_dialog *p)
{
	_list_set_scroll(p->list);

	mWidgetRedraw(MLK_WIDGET(p->list));
}

/* 追加/挿入 */

static void _cmd_add_insert(_dialog *p,mlkbool insert)
{
	const char *text;
	int num;
	mlkbool ret;

	text = MLK_TR2(TRGROUP_PANEL_COLOR_PALETTE, TRID_PALEDIT_ADDINSERT);

	num = 1;

	if(!mSysDlg_inputTextNum(MLK_WINDOW(p), text, text,
		MSYSDLG_INPUTTEXT_F_SET_DEFAULT,
		1, PALETTE_MAX_COLNUM, &num))
		return;

	if(insert)
		ret = _data_insert(&p->dat, num);
	else
		ret = _data_append(&p->dat, num);

	if(ret)
		_update_list(p);
}

/* RGB値セット */

static void _cmd_set_rgb(_dialog *p)
{
	mStr str = MSTR_INIT;
	RGB8 c;
	uint8_t *pd;

	//値取得

	mLineEditGetTextStr(p->edit_rgb, &str);

	ColorText_to_RGB8(&c, str.buf);

	mStrFree(&str);

	//エディットクリア

	mLineEditSetText(p->edit_rgb, NULL);

	//セット

	pd = p->dat.palbuf + p->dat.curno * 3;

	pd[0] = c.r;
	pd[1] = c.g;
	pd[2] = c.b;

	//カーソルを進める

	if(p->dat.curno < p->dat.palnum - 1)
		p->dat.curno++;

	mWidgetRedraw(MLK_WIDGET(p->list));
}

/* イベントハンドラ */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	_dialog *p = (_dialog *)wg;

	switch(ev->type)
	{
		case MEVENT_NOTIFY:
			switch(ev->notify.id)
			{
				//スクロール
				case WID_SCROLL:
					if(ev->notify.notify_type == MSCROLLBAR_N_ACTION
						&& (ev->notify.param2 & MSCROLLBAR_ACTION_F_CHANGE_POS))
						mWidgetRedraw(MLK_WIDGET(p->list));
					break;
				//RGB入力: Enter
				case WID_EDIT_RGB:
					if(ev->notify.notify_type == MLINEEDIT_N_ENTER)
						_cmd_set_rgb(p);
					break;
			}
			break;

		case MEVENT_COMMAND:
			switch(ev->cmd.id)
			{
				//範囲削除
				case 0:
					if(_data_delete(&p->dat))
						_update_list(p);
					break;
				//グラデーション
				case 1:
					if(_data_gradation(&p->dat))
						_update_list(p);
					break;
				//追加/挿入
				case 2:
				case 3:
					_cmd_add_insert(p, (ev->cmd.id == 3));
					break;
			}
			break;
	}
	
	return mDialogEventDefault_okcancel(wg, ev);
}


//==========================
// main
//==========================


/* 破棄ハンドラ */

static void _destroy_handle(mWidget *wg)
{
	mFree(((_dialog *)wg)->dat.palbuf);
}

/* ダイアログ作成 */

static _dialog *_create_dlg(mWindow *parent,PaletteListItem *pi)
{
	_dialog *p;
	mWidget *ct,*ct2;
	mIconBar *ib;
	int i;

	//作成
	
	p = (_dialog *)mDialogNew(parent, sizeof(_dialog), MTOPLEVEL_S_DIALOG_NORMAL);
	if(!p) return NULL;

	p->wg.event = _event_handle;
	p->wg.destroy = _destroy_handle;

	//編集用バッファ作成

	p->dat.palbuf = (uint8_t *)mMalloc(PALETTE_MAX_COLNUM * 3);
	if(!p->dat.palbuf)
	{
		mWidgetDestroy(MLK_WIDGET(p));
		return NULL;
	}

	p->dat.palnum = pi->palnum;
	p->dat.seltop = -1;

	memcpy(p->dat.palbuf, pi->palbuf, pi->palnum * 3);

	//

	MLK_TRGROUP(TRGROUP_PANEL_COLOR_PALETTE);

	mContainerSetPadding_same(MLK_CONTAINER(p), 8);

	mToplevelSetTitle(MLK_TOPLEVEL(p), MLK_TR(TRID_PALEDIT_TITLE));

	//--------- 1段目

	ct = mContainerCreateHorz(MLK_WIDGET(p), 0, MLF_EXPAND_WH, 0);

	//リスト

	p->list = _create_pallist(ct);
	p->list->pdat = &p->dat;

	//スクロールバー

	p->scr = mScrollBarCreate(ct, WID_SCROLL, MLF_EXPAND_H, 0, MSCROLLBAR_S_VERT);

	p->list->scr = p->scr;

	//mIconBar

	ib = mIconBarCreate(ct, 0, 0, MLK_MAKE32_4(4,0,0,0),
		MICONBAR_S_VERT | MICONBAR_S_BUTTON_FRAME | MICONBAR_S_TOOLTIP | MICONBAR_S_DESTROY_IMAGELIST);

	mIconBarSetImageList(ib, mImageListLoadPNG("!/img/colpal_edit.png", 16));
	mIconBarSetTooltipTrGroup(ib, TRGROUP_PANEL_COLOR_PALETTE);

	for(i = 0; i < 4; i++)
		mIconBarAdd(ib, i, i, TRID_PALEDIT_TOOLTIP_TOP + i, 0);

	//------ 2段目

	ct = mContainerCreateHorz(MLK_WIDGET(p), 5, MLF_EXPAND_W, MLK_MAKE32_4(0,10,0,0));

	mLabelCreate(ct, 0, 0, 0, MLK_TR(TRID_PALEDIT_RGBINPUT));

	p->edit_rgb = mLineEditCreate(ct, WID_EDIT_RGB, MLF_EXPAND_W, 0, MLINEEDIT_S_NOTIFY_ENTER);

	mLabelCreate(ct, 0, 0, MLABEL_S_BORDER, MLK_TR(TRID_PALEDIT_RGBINPUT_HELP));

	//------ 3段目

	ct = mContainerCreateHorz(MLK_WIDGET(p), 10, MLF_EXPAND_W, MLK_MAKE32_4(0,15,0,0));

	//ヘルプ

	mLabelCreate(ct, MLF_EXPAND_X, 0, MLABEL_S_BORDER, MLK_TR(TRID_PALEDIT_HELP));

	//OK,CANCEL

	ct2 = mContainerCreateVert(ct, 4, MLF_BOTTOM, 0); 

	mButtonCreate(ct2, MLK_WID_OK, MLF_EXPAND_W, 0, 0, MLK_TR_SYS(MLK_TRSYS_OK));
	mButtonCreate(ct2, MLK_WID_CANCEL, MLF_EXPAND_W, 0, 0, MLK_TR_SYS(MLK_TRSYS_CANCEL));
	
	return p;
}

/* OK 時、パレットデータセット */

static void _set_palette(_data *p,PaletteListItem *pi)
{
	//数が異なる場合、再確保

	if(p->palnum != pi->palnum)
		PaletteListItem_resize(pi, p->palnum);

	//データコピー

	memcpy(pi->palbuf, p->palbuf, p->palnum * 3);

	PaletteList_setModify();
}

/** パレット編集ダイアログ実行 */

mlkbool PanelColorPalette_dlg_paledit(mWindow *parent,PaletteListItem *pi)
{
	_dialog *p;
	int ret;

	p = _create_dlg(parent, pi);
	if(!p) return FALSE;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	ret = mDialogRun(MLK_DIALOG(p), FALSE);

	if(ret)
		_set_palette(&p->dat, pi);

	mWidgetDestroy(MLK_WIDGET(p));

	return ret;
}


