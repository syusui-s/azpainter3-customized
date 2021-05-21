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

/************************************************
 * グラデーション編集のバーウィジェット
 ************************************************/

#include <string.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_event.h"
#include "mlk_pixbuf.h"
#include "mlk_guicol.h"

#include "gradation_list.h"

#include "pv_gradedit.h"


//-----------------

struct _GradEditBar
{
	mWidget wg;

	uint8_t *buf,		//データバッファ (最大ポイント数分確保)
		*curpt_buf,		//カレントポイントの位置
		*another_buf;	//もう一方のデータバッファ (RGB なら Alpha のデータ)
	int point_num,		//ポイント数
		point_bytes,	//一つのポイントのバイト数
		curpt_index,	//カレントポイントのインデックス位置
		fdrag;
	mlkbool mode_alpha,	//編集タイプ (FALSE=RGB, TRUE=Alpha)
		single_col;		//単色化
};

//-----------------

#define _BUF_TOPBYTES  3	//データの先頭バイト数
#define _UNDER_HEIGHT  15	//ポイントの部分の高さ
#define _POINT_WIDTH   11	//ポイントの幅
#define _POINT_HALFW   5	//ポイントの半径

#define _POINTPOS_BIT  GRAD_DAT_POS_BIT
#define _POINTPOS_MAX  (1 << GRAD_DAT_POS_BIT)

#define _GET_POINTBUF(p,pos)  (p->buf + (pos) * p->point_bytes)

//-----------------


//=============================
// ポイントデータ
//=============================


/* ポイントの位置を取得 */

static int _get_ptpos(uint8_t *buf)
{
	return ((buf[0] << 8) | buf[1]) & GRAD_DAT_POS_MASK;
}

/* バッファから 16bit 値を取得 */

static int _get_bufval16(uint8_t *buf)
{
	return (buf[0] << 8) | buf[1];
}

/* ポイント位置変更 (色タイプは維持) */

static void _set_ptpos(uint8_t *buf,int pos)
{
	buf[0] = (buf[0] & 0xc0) | (pos >> 8);
	buf[1] = (uint8_t)pos;
}

/* ポイントのアルファ値変更 */

static void _set_ptalpha(uint8_t *buf,int val)
{
	buf[2] = (uint8_t)(val >> 8);
	buf[3] = (uint8_t)val;
}


//=============================
// sub
//=============================


/* 通知 */

static void _notify(GradEditBar *p,int type)
{
	mWidgetEventAdd_notify(MLK_WIDGET(p), NULL, type, 0, 0);
}

/* 更新と通知(データ変更) */

static void _update_and_notify(GradEditBar *p)
{
	mWidgetRedraw(MLK_WIDGET(p));

	_notify(p, GRADEDITBAR_N_MODIFY);
}

/* データ位置からスクリーンX位置取得 */

static int _getpos_dat_to_screen(GradEditBar *p,int pos)
{
	if(pos == 0)
		return _POINT_HALFW;
	else if(pos == _POINTPOS_MAX)
		return p->wg.w - 1 - _POINT_HALFW;
	else
		return ((pos * (p->wg.w - 1 - _POINT_WIDTH)) >> _POINTPOS_BIT) + _POINT_HALFW + 1;
}

/* スクリーンX位置からデータ位置取得 */

static int _getpos_screen_to_dat(GradEditBar *p,int x)
{
	return (x - _POINT_HALFW - 1) * _POINTPOS_MAX / (p->wg.w - _POINT_HALFW * 2 - 3);
}

/* カーソル位置からポイントのインデックス位置を取得
 *
 * return: [-2] ポイントがなく、バーの範囲外 [-1] 範囲内でポイントなし */

static int _get_ptindex_by_screen(GradEditBar *p,int sx,int sy)
{
	int x,y,i;
	uint8_t *pp;

	y = p->wg.h - _UNDER_HEIGHT + 4;

	//現在のポイントを優先的に判定

	x = _getpos_dat_to_screen(p, _get_ptpos(p->curpt_buf));

	if(x - _POINT_HALFW <= sx && sx <= x + _POINT_HALFW
		&& y <= sy && sy <= y + _UNDER_HEIGHT - 4)
		return p->curpt_index;

	//他ポイント

	pp = p->buf;

	for(i = 0; i < p->point_num; i++, pp += p->point_bytes)
	{
		x = _getpos_dat_to_screen(p, _get_ptpos(pp));

		if(x - _POINT_HALFW <= sx && sx <= x + _POINT_HALFW
			&& y <= sy && sy <= y + _UNDER_HEIGHT - 4)
			return i;
	}

	//ポイントなし

	if(sx <= _POINT_HALFW || sx >= p->wg.w - _POINT_HALFW - 1)
		return -2;
	else
		return -1;
}

/* データ位置を前後のポイントの間になるように調整 */

static int _adjust_datpos(GradEditBar *p,uint8_t *buf,int pos)
{
	int pos1,pos2;

	pos1 = _get_ptpos(buf - p->point_bytes);
	pos2 = _get_ptpos(buf + p->point_bytes);

	if(pos <= pos1)
		pos = pos1 + 1;
	else if(pos >= pos2)
		pos = pos2 - 1;

	return pos;
}


//=============================
// コマンド
//=============================


/* 新規ポイント作成 */

static void _new_point(GradEditBar *p,int datpos)
{
	uint8_t *pp,*insbuf,*p1,*p2;
	int i,insindex,pos1,pos2,a1,a2,bytes;

	bytes = p->point_bytes;

	//ポイント数制限

	if(p->point_num >= GRAD_DAT_MAX_POINT) return;

	//バッファ内の挿入位置
	//[!] 同じ位置にはセットできないようにする

	pp = p->buf;
	insindex = -1;

	for(i = 0; i < p->point_num - 1; i++, pp += bytes)
	{
		if(_get_ptpos(pp) < datpos && datpos < _get_ptpos(pp + bytes))
		{
			insbuf = pp + bytes;
			insindex = i + 1;
			break;
		}
	}

	if(insindex == -1) return;

	//挿入位置を空ける

	memmove(insbuf + bytes, insbuf, (p->point_num - insindex) * bytes);

	//位置セット

	insbuf[0] = datpos >> 8;
	insbuf[1] = (uint8_t)datpos;

	//値セット

	p1 = insbuf - bytes;
	p2 = insbuf + bytes;

	pos1 = _get_ptpos(p1);
	pos2 = _get_ptpos(p2);

	if(p->mode_alpha)
	{
		//A

		a1 = _get_bufval16(p1 + 2);
		a2 = _get_bufval16(p2 + 2);

		_set_ptalpha(insbuf, (a2 - a1) * (datpos - pos1) / (pos2 - pos1) + a1);
	}
	else
	{
		//RGB (指定色)

		insbuf[0] |= 0x80;

		if((*p1 >> 6) != 2 || (*p2 >> 6) != 2)
		{
			//左右いずれかが描画色/背景色の場合は黒

			insbuf[2] = insbuf[3] = insbuf[4] = 0;
		}
		else
		{
			//左右が共に指定色ならその中間色
					
			pos2 -= pos1;
			pos1 = datpos - pos1;
			p1 += 2;
			p2 += 2;

			for(i = 0; i < 3; i++)
				insbuf[2 + i] = (p2[i] - p1[i]) * pos1 / pos2 + p1[i];
		}
	}

	//

	p->point_num++;
	p->curpt_buf = insbuf;
	p->curpt_index = insindex;

	_update_and_notify(p);
}

/* 指定インデックスのポイントにカレントの値をセット */

static void _set_currentval_toindex(GradEditBar *p,int index)
{
	uint8_t *buf;

	buf = _GET_POINTBUF(p, index);

	if(p->mode_alpha)
		_set_ptalpha(buf, _get_bufval16(p->curpt_buf + 2));
	else
	{
		*buf = (*buf & 0x3f) | (p->curpt_buf[0] & 0xc0);
	
		memcpy(buf + 2, p->curpt_buf + 2, 3);
	}

	_update_and_notify(p);
}

/* カレントから指定インデックスの間を等間隔に */

static void _even_points_toindex(GradEditBar *p,int index)
{
	uint8_t *buf1,*buf2;
	int index1,index2,pos1,posd,i;

	//位置

	if(index < p->curpt_index)
		index1 = index, index2 = p->curpt_index;
	else
		index1 = p->curpt_index, index2 = index;

	//同じ位置、または隣接している場合は対象外

	if(index1 == index2 || index1 + 1 == index2) return;

	//

	buf1 = _GET_POINTBUF(p, index1);
	buf2 = _GET_POINTBUF(p, index2);

	pos1 = _get_ptpos(buf1);
	posd = _get_ptpos(buf2) - pos1;

	buf1 += p->point_bytes;

	for(i = index1 + 1; i < index2; i++, buf1 += p->point_bytes)
	{
		_set_ptpos(buf1,
			(int)((double)(i - index1) / (index2 - index1) * posd + pos1 + 0.5));
	}

	//

	_update_and_notify(p);
}


//=============================
// イベント
//=============================


/* ボタン押し */

static void _event_press(GradEditBar *p,mEvent *ev)
{
	int index,n,state;

	//カーソル位置のポイント

	index = _get_ptindex_by_screen(p, ev->pt.x, ev->pt.y);
	if(index == -2) return;

	//

	state = ev->pt.state & MLK_STATE_MASK_MODS;

	if(index == -1)
	{
		//空位置 : 新規ポイント作成 (装飾キーがある場合は無効)

		if(state) return;

		_new_point(p, _getpos_screen_to_dat(p, ev->pt.x));
	}
	else if(state == MLK_STATE_CTRL)
	{
		//+Ctrl : 等間隔
		_even_points_toindex(p, index);
	}
	else if(state == MLK_STATE_SHIFT)
	{
		//+Shift : カレントの値をセット
		_set_currentval_toindex(p, index);
	}
	else if(state == MLK_STATE_ALT)
	{
		//+Alt : 削除
		GradEditBar_deletePoint(p, index);
	}
	else
	{
		//------ 既存ポイント

		//カレント変更

		if(p->curpt_index != index)
		{
			p->curpt_index = index;
			p->curpt_buf = _GET_POINTBUF(p, index);

			mWidgetRedraw(MLK_WIDGET(p));
			
			_notify(p, GRADEDITBAR_N_CHANGE_CURRENT);
		}

		//ドラッグ開始 (端のポイントは除く)

		n = _get_ptpos(p->curpt_buf);

		if(n != 0 && n != _POINTPOS_MAX)
		{
			p->fdrag = 1;
			mWidgetGrabPointer(MLK_WIDGET(p));
		}
	}
}

/* カーソル移動 */

static void _event_motion(GradEditBar *p,mEvent *ev)
{
	int pos;

	//位置

	pos = _getpos_screen_to_dat(p, ev->pt.x);
	pos = _adjust_datpos(p, p->curpt_buf, pos);

	//位置変更

	if(pos != _get_ptpos(p->curpt_buf))
	{
		_set_ptpos(p->curpt_buf, pos);

		_update_and_notify(p);
	}
}

/* グラブ解放 */

static void _release_grab(GradEditBar *p)
{
	if(p->fdrag)
	{
		p->fdrag = FALSE;
		mWidgetUngrabPointer();
	}
}

/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	GradEditBar *p = (GradEditBar *)wg;

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.act == MEVENT_POINTER_ACT_MOTION)
			{
				//移動

				if(p->fdrag)
					_event_motion(p, ev);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_PRESS)
			{
				//押し
				
				if(ev->pt.btt == MLK_BTT_LEFT && !p->fdrag)
					_event_press(p, ev);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_RELEASE)
			{
				//離し
				
				if(ev->pt.btt == MLK_BTT_LEFT)
					_release_grab(p);
			}
			break;

		case MEVENT_FOCUS:
			if(ev->focus.is_out)
				_release_grab(p);
			break;
	}

	return 1;
}


//=============================
// 描画
//=============================


/* 一つのポイント描画 */

static void _draw_point_one(GradEditBar *p,mPixbuf *pixbuf,uint8_t *ptbuf)
{
	int x,y,type;
	uint32_t col;

	//中心位置

	x = _getpos_dat_to_screen(p, _get_ptpos(ptbuf));
	y = p->wg.h - _UNDER_HEIGHT;

	//枠

	col = (ptbuf == p->curpt_buf)? mRGBtoPix(0xff0000): 0;

	mPixbufLineV(pixbuf, x, y, 4, col);
	mPixbufBox(pixbuf, x - _POINT_HALFW, y + 4, _POINT_WIDTH, _UNDER_HEIGHT - 4, col);

	//色

	if(p->mode_alpha)
	{
		col = _get_bufval16(ptbuf + 2) * 255 >> GRAD_DAT_ALPHA_BIT;
		col = mRGBtoPix_sep(col,col,col);
	}
	else
	{
		type = *ptbuf >> 6;

		if(type == 0)
			col = 0;
		else if(type == 1)
			col = MGUICOL_PIX(WHITE);
		else
			col = mRGBtoPix_sep(ptbuf[2], ptbuf[3], ptbuf[4]);
	}

	mPixbufFillBox(pixbuf,
		x - _POINT_HALFW + 1, y + 5, _POINT_WIDTH - 2, _UNDER_HEIGHT - 6, col);
}

/* ポイントすべて描画 */

static void _draw_points(GradEditBar *p,mPixbuf *pixbuf)
{
	uint8_t *buf;
	int i;

	//カレント以外

	buf = p->buf;

	for(i = p->point_num; i > 0; i--, buf += p->point_bytes)
	{
		if(buf != p->curpt_buf)
			_draw_point_one(p, pixbuf, buf);
	}

	//カレントは最後に描画

	_draw_point_one(p, pixbuf, p->curpt_buf);
}

/* A 編集時のグラデーション描画 */

static void _draw_alpha_grad(GradEditBar *p,mPixbuf *pixbuf,int x,int y,int w,int h)
{
	uint8_t *ps,*pd;
	int ix,pos,pos1,pos2,a1,a2,bpp;
	mBox box;

	if(!mPixbufClip_getBox_d(pixbuf, &box, x, y, w, h))
		return;

	ps = p->buf;
	pd = mPixbufGetBufPtFast(pixbuf, box.x, box.y);
	bpp = pixbuf->pixel_bytes;

	x = box.x - x;
	y = box.y - y;

	for(ix = box.w; ix; ix--, x++, pd += bpp)
	{
		pos = (x << _POINTPOS_BIT) / (w - 1);

		//値

		while(pos > _get_ptpos(ps + 4))
			ps += 4;

		pos1 = _get_bufval16(ps);
		a1 = _get_bufval16(ps + 2);

		pos2 = _get_bufval16(ps + 4);
		a2 = _get_bufval16(ps + 6);

		a1 = (a2 - a1) * (pos - pos1) / (pos2 - pos1) + a1;
		a1 = a1 * 255 >> GRAD_DAT_ALPHA_BIT;

		//縦に描画

		mPixbufBufLineV(pixbuf, pd, box.h, mRGBtoPix_sep(a1,a1,a1));
	}
}

/* 描画ハンドラ */

static void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	GradEditBar *p = (GradEditBar *)wg;

	//背景

	mWidgetDrawBkgnd(wg, NULL);

	//枠

	mPixbufBox(pixbuf, _POINT_HALFW, 0, wg->w - _POINT_HALFW * 2, wg->h - _UNDER_HEIGHT, 0);

	//グラデーション

	if(p->mode_alpha)
	{
		_draw_alpha_grad(p, pixbuf,
			_POINT_HALFW + 1, 1,
			p->wg.w - 2 - _POINT_HALFW * 2, p->wg.h - 2 - _UNDER_HEIGHT);
	}
	else
	{
		GradItem_drawPixbuf(pixbuf,
			_POINT_HALFW + 1, 1, wg->w - 2 - _POINT_HALFW * 2, wg->h - 2 - _UNDER_HEIGHT,
			p->buf, p->another_buf, p->single_col);
	}

	//ポイント

	_draw_points(p, pixbuf);
}


//=============================
// main
//=============================


/* 破棄ハンドラ */

static void _destroy_handle(mWidget *wg)
{
	mFree(((GradEditBar *)wg)->buf);
}

/** 作成 */

GradEditBar *GradEditBar_new(mWidget *parent,int id,mlkbool mode_alpha,uint8_t *buf)
{
	GradEditBar *p;

	p = (GradEditBar *)mWidgetNew(parent, sizeof(GradEditBar));

	p->wg.id = id;
	p->wg.flayout = MLF_EXPAND_W;
	p->wg.fevent |= MWIDGET_EVENT_POINTER;
	p->wg.destroy = _destroy_handle;
	p->wg.event = _event_handle;
	p->wg.draw = _draw_handle;
	p->wg.hintW = 60;
	p->wg.hintH = 40;
	p->wg.initW = 400;

	p->mode_alpha = mode_alpha;
	p->point_bytes = (mode_alpha)? 4: 5;

	//バッファ確保

	p->buf = (uint8_t *)mMalloc(GRAD_DAT_MAX_POINT * p->point_bytes);

	//データをコピー

	if(mode_alpha)
	{
		//A
		p->point_num = buf[2];
		memcpy(p->buf, buf + _BUF_TOPBYTES + buf[1] * 5, p->point_num * p->point_bytes);
	}
	else
	{
		//RGB
		p->point_num = buf[1];
		memcpy(p->buf, buf + _BUF_TOPBYTES, p->point_num * p->point_bytes);
	}

	//カレントポイント

	p->curpt_buf = p->buf;

	return p;
}

/** もう一方のモードのデータバッファをセット (描画時に必要) */

void GradEditBar_setAnotherModeBuf(GradEditBar *p,uint8_t *buf)
{
	p->another_buf = buf;
}

/** 単色化のフラグをセット
 *
 * type: [0]OFF [正]ON [負]反転 */

void GradEditBar_setSingleColor(GradEditBar *p,int type)
{
	if(type < 0)
		p->single_col ^= 1;
	else
		p->single_col = (type != 0);
}


//---------- 取得


/** ポイント数取得 */

int GradEditBar_getPointNum(GradEditBar *p)
{
	return p->point_num;
}

/** 現在のデータバッファ取得 */

uint8_t *GradEditBar_getBuf(GradEditBar *p)
{
	return p->buf;
}

/** カレントポイントの位置を取得 */

int GradEditBar_getCurrentPos(GradEditBar *p)
{
	return _get_ptpos(p->curpt_buf);
}

/** カレントポイントの色とタイプを取得
 *
 * return: 色タイプ */

int GradEditBar_getCurrentColor(GradEditBar *p,uint32_t *col)
{
	uint8_t *buf = p->curpt_buf;

	*col = (buf[2] << 16) | (buf[3] << 8) | buf[4];

	return buf[0] >> 6;
}

/** カレントポイントの不透明度を取得 (0-1000) */

int GradEditBar_getCurrentOpacity(GradEditBar *p)
{
	return (int)((double)_get_bufval16(p->curpt_buf + 2) / (1 << GRAD_DAT_ALPHA_BIT) * 1000.0 + 0.5);
}

/** カレントと一つ右との間の最大分割数を取得 */

int GradEditBar_getMaxSplitNum_toRight(GradEditBar *p)
{
	int num;

	if(p->curpt_index == p->point_num - 1)
		return 0;
	else
	{
		num = _get_ptpos(p->curpt_buf + p->point_bytes) - _get_ptpos(p->curpt_buf);

		//ポイント最大数との調整

		if(p->point_num + num - 1 > GRAD_DAT_MAX_POINT)
			num = GRAD_DAT_MAX_POINT - (p->point_num - 1);

		return num;
	}
}


//---------- セット


/** カレントポイントの位置をセット
 *
 * pos: 0-1000 */

mlkbool GradEditBar_setCurrentPos(GradEditBar *p,int pos)
{
	int curpos;

	curpos = _get_ptpos(p->curpt_buf);

	//端の位置は変更できない

	if(curpos != 0 && curpos != _POINTPOS_MAX)
	{
		pos = _adjust_datpos(p, p->curpt_buf, (int)(pos / 1000.0 * _POINTPOS_MAX + 0.5));

		if(pos != curpos)
		{
			_set_ptpos(p->curpt_buf, pos);
		
			mWidgetRedraw(MLK_WIDGET(p));

			return TRUE;
		}
	}

	return FALSE;
}

/** 不透明度をセット
 *
 * val: 0-1000 */

void GradEditBar_setOpacity(GradEditBar *p,int val)
{
	_set_ptalpha(p->curpt_buf, (int)(val / 1000.0 * (1 << GRAD_DAT_ALPHA_BIT) + 0.5));
	
	mWidgetRedraw(MLK_WIDGET(p));
}

/** 色タイプ変更 */

void GradEditBar_setColorType(GradEditBar *p,int type)
{
	*(p->curpt_buf) = (*(p->curpt_buf) & 0x3f) | (type << 6);

	mWidgetRedraw(MLK_WIDGET(p));
}

/** 色の RGB 値を変更 */

void GradEditBar_setColorRGB(GradEditBar *p,uint32_t col)
{
	uint8_t *buf = p->curpt_buf;

	buf[2] = MLK_RGB_R(col);
	buf[3] = MLK_RGB_G(col);
	buf[4] = MLK_RGB_B(col);

	mWidgetRedraw(MLK_WIDGET(p));
}


//---------- コマンド


/** カレントポイントを左右に移動 */

void GradEditBar_moveCurrentPoint(GradEditBar *p,mlkbool right)
{
	if(right)
	{
		//右

		if(p->curpt_index == p->point_num - 1) return;

		p->curpt_index++;
		p->curpt_buf += p->point_bytes;
	}
	else
	{
		//左

		if(p->curpt_index == 0) return;

		p->curpt_index--;
		p->curpt_buf -= p->point_bytes;
	}

	mWidgetRedraw(MLK_WIDGET(p));
}

/** ポイントを削除 (データ変更の通知も行う)
 *
 * index: 負の値でカレントポイント */

void GradEditBar_deletePoint(GradEditBar *p,int index)
{
	uint8_t *buf;

	if(index < 0) index = p->curpt_index;

	//端のポイントは除外

	if(index == 0 || index == p->point_num - 1) return;

	//詰める

	buf = _GET_POINTBUF(p, index);

	memmove(buf, buf + p->point_bytes, (p->point_num - index - 1) * p->point_bytes);

	//

	p->point_num--;

	_update_and_notify(p);
}

/** カレントポイントの位置を左右の中間位置にセット */

void GradEditBar_moveCurrentPos_middle(GradEditBar *p)
{
	int pos1,pos2;

	//端のポイントは除外

	if(p->curpt_index == 0 || p->curpt_index == p->point_num - 1)
		return;

	pos1 = _get_ptpos(p->curpt_buf - p->point_bytes);
	pos2 = _get_ptpos(p->curpt_buf + p->point_bytes);

	_set_ptpos(p->curpt_buf, pos1 + (pos2 - pos1) / 2);

	_update_and_notify(p);
}

/** すべて等間隔に */

void GradEditBar_evenAllPoints(GradEditBar *p)
{
	uint8_t *buf;
	int i;
	double d;

	//両端を除くポイントを対象に

	buf = _GET_POINTBUF(p, 1);
	d = (double)_POINTPOS_MAX / (p->point_num - 1);

	for(i = 1; i < p->point_num - 1; i++, buf += p->point_bytes)
		_set_ptpos(buf, (int)(i * d + 0.5));

	_update_and_notify(p);
}

/** ポイントを反転 */

void GradEditBar_reversePoints(GradEditBar *p)
{
	uint8_t *p1,*p2,tmp[5];
	int i;

	//値を入れ替え

	p1 = p->buf;
	p2 = _GET_POINTBUF(p, p->point_num - 1);

	for(i = p->point_num / 2; i; i--)
	{
		memcpy(tmp, p1, p->point_bytes);
		memcpy(p1, p2, p->point_bytes);
		memcpy(p2, tmp, p->point_bytes);

		p1 += p->point_bytes;
		p2 -= p->point_bytes;
	}

	//位置を反転

	p1 = p->buf;

	for(i = p->point_num; i; i--, p1 += p->point_bytes)
		_set_ptpos(p1, _POINTPOS_MAX - _get_ptpos(p1));

	_update_and_notify(p);
}

/** カレントと一つ右の間を分割 */

void GradEditBar_splitPoint_toRight(GradEditBar *p,int num)
{
	uint8_t *buf1,*buf2,rgb1[3],rgb2[3],type1,type2;
	int pos1,pos2,i,j,pos,a1,a2;
	double d_pos,d;

	num--;

	buf1 = p->curpt_buf;
	buf2 = buf1 + p->point_bytes;

	//位置と値

	pos1 = _get_ptpos(buf1);
	pos2 = _get_ptpos(buf2);

	if(p->mode_alpha)
	{
		a1 = _get_bufval16(buf1 + 2);
		a2 = _get_bufval16(buf2 + 2);
	}
	else
	{
		type1 = *buf1 >> 6;
		type2 = *buf2 >> 6;
		
		memcpy(rgb1, buf1 + 2, 3);
		memcpy(rgb2, buf2 + 2, 3);
	}

	//挿入位置を空ける

	memmove(buf2 + num * p->point_bytes, buf2,
		(p->point_num - p->curpt_index) * p->point_bytes);

	//新規分をセット

	buf1 += p->point_bytes;
	
	d_pos = (double)(pos2 - pos1) / (num + 1);

	for(i = 0; i < num; i++, buf1 += p->point_bytes)
	{
		//位置
		
		pos = (int)((i + 1) * d_pos + pos1 + 0.5);
	
		buf1[0] = pos >> 8;
		buf1[1] = pos & 255;

		//色/A

		d = (double)(pos - pos1) / (pos2 - pos1);

		if(p->mode_alpha)
			_set_ptalpha(buf1, (int)(d * (a2 - a1) + a1 + 0.5));
		else
		{
			*buf1 |= 0x80;
		
			if(type1 == 2 && type2 == 2)
			{
				//左右が両方指定色の場合は中間色
				
				for(j = 0; j < 3; j++)
					buf1[2 + j] = (uint8_t)(d * (rgb2[j] - rgb1[j]) + rgb1[j] + 0.5);
			}
			else
				buf1[2] = buf1[3] = buf1[4] = 0;
		}
	}

	//

	p->point_num += num;

	_update_and_notify(p);
}

