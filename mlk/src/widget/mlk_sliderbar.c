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
 * mSliderBar [スライダー]
 *****************************************/

#include <math.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_sliderbar.h"
#include "mlk_pixbuf.h"
#include "mlk_event.h"
#include "mlk_key.h"
#include "mlk_guicol.h"

#include "mlk_pv_widget.h"


//=============================
// sub
//=============================


/** つまみの半径取得 */

static int _get_grip_half_w(mSliderBar *p)
{
	return (p->sl.fstyle & MSLIDERBAR_S_SMALL)? 4: 5;
}

/** 内部での位置変更時 */

static void _change_pos(mSliderBar *p,int pos,uint32_t act_flags)
{
	//位置セット

	if(mSliderBarSetPos(p, pos))
		act_flags |= MSLIDERBAR_ACTION_F_CHANGE_POS;
	
	//キー操作/ドラッグ/スクロール時、位置が変わっていない場合は送らない
	//(ボタン押し/離し時は常に送る)
	
	if((act_flags & (MSLIDERBAR_ACTION_F_KEY_PRESS
			| MSLIDERBAR_ACTION_F_DRAG
			| MSLIDERBAR_ACTION_F_SCROLL))
		&& !(act_flags & MSLIDERBAR_ACTION_F_CHANGE_POS))
		return;
	
	//ハンドラ

	if(p->sl.action)
		(p->sl.action)(MLK_WIDGET(p), pos, act_flags);
}

/** action デフォルト関数 */

static void _action_handle(mWidget *p,int pos,uint32_t flags)
{
	mWidgetEventAdd_notify(p, NULL,
		MSLIDERBAR_N_ACTION, pos, flags);
}


//=============================
// main
//=============================


/**@ mSliderBar データ解放 */

void mSliderBarDestroy(mWidget *wg)
{

}

/**@ 作成 */

mSliderBar *mSliderBarNew(mWidget *parent,int size,uint32_t fstyle)
{
	mSliderBar *p;
	int s1,s2;
	
	if(size < sizeof(mSliderBar))
		size = sizeof(mSliderBar);
	
	p = (mSliderBar *)mWidgetNew(parent, size);
	if(!p) return NULL;
	
	p->wg.draw = mSliderBarHandle_draw;
	p->wg.event = mSliderBarHandle_event;
	
	p->wg.fstate |= MWIDGET_STATE_TAKE_FOCUS | MWIDGET_STATE_ENABLE_KEY_REPEAT;
	p->wg.fevent |= MWIDGET_EVENT_POINTER | MWIDGET_EVENT_KEY | MWIDGET_EVENT_SCROLL;

	p->sl.fstyle = fstyle;
	p->sl.action = _action_handle;
	p->sl.max = 100;
	
	//推奨サイズ
	
	s1 = (fstyle & MSLIDERBAR_S_SMALL)? 11: 15; //水平時高さ
	s2 = (_get_grip_half_w(p) * 2 + 1) + 4;		//水平時幅
	
	if(fstyle & MSLIDERBAR_S_VERT)
	{
		p->wg.hintW = s1;
		p->wg.hintH = s2;
	}
	else
	{
		p->wg.hintW = s2;
		p->wg.hintH = s1;
	}
	
	return p;
}

/**@ 作成 */

mSliderBar *mSliderBarCreate(mWidget *parent,int id,
	uint32_t flayout,uint32_t margin_pack,uint32_t fstyle)
{
	mSliderBar *p;

	p = mSliderBarNew(parent, 0, fstyle);
	if(!p) return NULL;

	__mWidgetCreateInit(MLK_WIDGET(p), id, flayout, margin_pack);
	
	return p;
}

/**@ 現在位置を取得 */

int mSliderBarGetPos(mSliderBar *p)
{
	return p->sl.pos;
}

/**@ 値のステータスをセット */

void mSliderBarSetStatus(mSliderBar *p,int min,int max,int pos)
{
	if(min >= max) max = min + 1;
	
	if(pos < min) pos = min;
	else if(pos > max) pos = max;
	
	p->sl.min = min;
	p->sl.max = max;
	p->sl.pos = pos;
	
	mWidgetRedraw(MLK_WIDGET(p));
}

/**@ 値の範囲をセット
 *
 * @d:現在位置は、範囲内に収められる。 */

void mSliderBarSetRange(mSliderBar *p,int min,int max)
{
	if(min >= max) max = min + 1;
	
	p->sl.min = min;
	p->sl.max = max;

	if(p->sl.pos < min)
		p->sl.pos = min;
	else if(p->sl.pos > max)
		p->sl.pos = max;
	
	mWidgetRedraw(MLK_WIDGET(p));
}

/**@ 現在位置をセット
 *
 * @r:位置が変わったか */

mlkbool mSliderBarSetPos(mSliderBar *p,int pos)
{
	if(pos < p->sl.min)
		pos = p->sl.min;
	else if(pos > p->sl.max)
		pos = p->sl.max;
	
	if(pos == p->sl.pos)
		return FALSE;
	else
	{
		p->sl.pos = pos;
		mWidgetRedraw(MLK_WIDGET(p));
		return TRUE;
	}
}


//========================
// イベント
//========================


/* ボタン押し/ドラッグ時
 *
 * return: 操作可能な位置か */

static mlkbool _on_press_drag(mSliderBar *p,int x,int y,uint32_t act_flags)
{
	int max,pos,gw;

	gw = _get_grip_half_w(p);

	//位置と幅
	
	if(p->sl.fstyle & MSLIDERBAR_S_VERT)
		max = p->wg.h, pos = y;
	else
		max = p->wg.w, pos = x;

	//バーの範囲がない
	
	max -= gw * 2 + 1;
	if(max <= 0) return FALSE;

	//バー位置
	
	pos -= gw;
	
	if(pos < 0) pos = 0;
	else if(pos > max) pos = max;
	
	pos = round((double)pos / max * (p->sl.max - p->sl.min) + p->sl.min);
	
	_change_pos(p, pos, act_flags);
	
	return TRUE;
}

/* キー押し時 */

static int _event_keydown(mSliderBar *p,uint32_t key)
{
	int pos,dir;

	if(p->sl.fgrab) return FALSE;

	//位置

	if(key == MKEY_HOME || key == MKEY_KP_HOME)
		//HOME
		pos = p->sl.min;
	else if(key == MKEY_END || key == MKEY_KP_END)
		//END
		pos = p->sl.max;
	else
	{
		//矢印キー

		dir = mKeyGetArrowDir(key);
		if(dir < 0) return FALSE;

		pos = p->sl.pos;

		if(p->sl.fstyle & MSLIDERBAR_S_VERT)
		{
			//垂直
			
			if(dir & 1)
				pos += (dir & 2)? 1: -1;
		}
		else
		{
			//水平

			if(!(dir & 1))
				pos += (dir & 2)? 1: -1;
		}
	}

	_change_pos(p, pos, MSLIDERBAR_ACTION_F_KEY_PRESS);
	
	return TRUE;
}

/* スクロール */

static void _event_scroll(mSliderBar *p,mEventScroll *ev)
{
	int n;

	if(ev->horz_step)
		n = ev->horz_step;
	else if(ev->vert_step)
	{
		n = ev->vert_step;

		//水平バーでの垂直スクロール時、上方向で +1、下方向で -1
		if(!(p->sl.fstyle & MSLIDERBAR_S_VERT))
			n = -n;
	}
	else
		return;

	_change_pos(p, p->sl.pos + n, MSLIDERBAR_ACTION_F_SCROLL);
}

/* グラブ解放 */

static void _release_grab(mSliderBar *p)
{
	if(p->sl.fgrab)
	{
		p->sl.fgrab = 0;
		mWidgetUngrabPointer();
		
		_change_pos(p, p->sl.pos, MSLIDERBAR_ACTION_F_BUTTON_RELEASE);
	}
}

/**@ event ハンドラ関数 */

int mSliderBarHandle_event(mWidget *wg,mEvent *ev)
{
	mSliderBar *p = MLK_SLIDERBAR(wg);

	switch(ev->type)
	{
		//ポインタ
		case MEVENT_POINTER:
			if(ev->pt.act == MEVENT_POINTER_ACT_MOTION)
			{
				//移動
				
				if(p->sl.fgrab)
					_on_press_drag(p, ev->pt.x, ev->pt.y, MSLIDERBAR_ACTION_F_DRAG);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_PRESS)
			{
				//押し
				
				if(ev->pt.btt == MLK_BTT_LEFT && !p->sl.fgrab)
				{
					if(_on_press_drag(p, ev->pt.x, ev->pt.y, MSLIDERBAR_ACTION_F_BUTTON_PRESS))
					{
						mWidgetSetFocus_redraw(wg, FALSE);
						mWidgetGrabPointer(wg);
						
						p->sl.fgrab = TRUE;
					}
				}
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_RELEASE)
			{
				//離し

				if(ev->pt.btt == MLK_BTT_LEFT)
					_release_grab(p);
			}
			break;

		//キー押し
		case MEVENT_KEYDOWN:
			return _event_keydown(p, ev->key.key);

		//スクロール
		case MEVENT_SCROLL:
			if(!p->sl.fgrab)
				_event_scroll(p, (mEventScroll *)ev);
			break;

		//フォーカス
		case MEVENT_FOCUS:
			if(ev->focus.is_out)
				_release_grab(p);
		
			mWidgetRedraw(wg);
			break;

		default:
			return FALSE;
	}

	return TRUE;
}


//==========================


/**@ draw ハンドラ関数 */

void mSliderBarHandle_draw(mWidget *wg,mPixbuf *pixbuf)
{
	mSliderBar *p = MLK_SLIDERBAR(wg);
	mPixCol colf,colgrip;
	int w,h,n,gw;
	
	//背景
	
	mWidgetDrawBkgnd(wg, NULL);

	//色

	colf = (mWidgetIsEnable(wg))? MGUICOL_PIX(FRAME): MGUICOL_PIX(FRAME_DISABLE);
	colgrip = (wg->fstate & MWIDGET_STATE_FOCUSED)? MGUICOL_PIX(FRAME_FOCUS): colf;

	//バー

	w = wg->w;
	h = wg->h;
	gw = _get_grip_half_w(p);
	
	if(p->sl.fstyle & MSLIDERBAR_S_VERT)
	{
		//------- 垂直

		//バー
		
		mPixbufBox(pixbuf, w / 2 - 1, gw, 3, h - gw * 2, colf);
		
		//つまみ
		
		n = (int)((double)(h - gw * 2 - 1) / (p->sl.max - p->sl.min) * (p->sl.pos - p->sl.min) + 0.5);
		
		mPixbufBox(pixbuf, 0, n, w, gw * 2 + 1, colgrip);
			
		mPixbufFillBox(pixbuf, 1, n + 1, w - 2, gw * 2 + 1 - 2, MGUICOL_PIX(FACE));

		mPixbufLineH(pixbuf, 2, n + gw, w - 4, colgrip);
	}
	else
	{
		//------- 水平
		
		//バー
		
		mPixbufBox(pixbuf, gw, h / 2 - 1, w - gw * 2, 3, colf);
		
		//つまみ
		
		n = (int)((double)(w - gw * 2 - 1) / (p->sl.max - p->sl.min) * (p->sl.pos - p->sl.min) + 0.5);

		mPixbufBox(pixbuf, n, 0, gw * 2 + 1, h, colgrip);
			
		mPixbufFillBox(pixbuf, n + 1, 1, gw * 2 + 1 - 2, h - 2, MGUICOL_PIX(FACE));

		mPixbufLineV(pixbuf, n + gw, 2, h - 4, colgrip);
	}
}

