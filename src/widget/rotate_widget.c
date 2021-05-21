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
 * RotateWidget
 *
 * 角度指定ウィジェット
 *****************************************/

#include <math.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_pixbuf.h"
#include "mlk_event.h"
#include "mlk_guicol.h"

/*
  <通知>
  param1: 角度
*/

//-------------------------

typedef struct _RotateWidget
{
	mWidget wg;

	int angle,
		fdrag;
	double rd;
}RotateWidget;

//-------------------------



/* カーソル位置から角度セット */

static void _set_angle(RotateWidget *p,mEvent *ev)
{
	int ct,n;
	double rd;

	ct = p->wg.w / 2;

	rd = atan2(ev->pt.y - ct, ev->pt.x - ct);

	n = (int)(-rd / MLK_MATH_PI * 180);

	if(n != p->angle)
	{
		p->angle = n;
		p->rd = rd;

		mWidgetRedraw(MLK_WIDGET(p));

		mWidgetEventAdd_notify(MLK_WIDGET(p), NULL, 0, n, 0);
	}
}

/* グラブ解除 */

static void _release_grab(RotateWidget *p)
{
	if(p->fdrag)
	{
		p->fdrag = 0;
		mWidgetUngrabPointer();
	}
}

/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	RotateWidget *p = (RotateWidget *)wg;

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.act == MEVENT_POINTER_ACT_MOTION)
			{
				//移動

				if(p->fdrag)
					_set_angle(p, ev);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_PRESS)
			{
				//押し

				if(ev->pt.btt == MLK_BTT_LEFT && !p->fdrag)
				{
					_set_angle(p, ev);

					p->fdrag = 1;
					mWidgetGrabPointer(wg);
				}
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

	return TRUE;
}

/* 描画 */

static void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	RotateWidget *p = (RotateWidget *)wg;
	int ct,r;

	//背景

	mWidgetDrawBkgnd(wg, NULL);

	//円の枠

	mPixbufEllipse(pixbuf, 0, 0, wg->w - 1, wg->h - 1, MGUICOL_PIX(FRAME));

	//角度の線

	ct = wg->w / 2;
	r = ct - 1;

	mPixbufLine(pixbuf, ct, ct,
		round(ct + r * cos(p->rd)), round(ct + r * sin(p->rd)),
		MGUICOL_PIX(TEXT));
}


//====================


/** 作成 */

RotateWidget *RotateWidget_new(mWidget *parent,int id,uint32_t margin_pack4,int size)
{
	RotateWidget *p;

	p = (RotateWidget *)mWidgetNew(parent, sizeof(RotateWidget));
	if(!p) return NULL;

	p->wg.id = id;
	p->wg.flayout = MLF_FIX_WH;
	p->wg.fevent |= MWIDGET_EVENT_POINTER;
	p->wg.event = _event_handle;
	p->wg.draw = _draw_handle;
	p->wg.w = p->wg.h = size;

	mWidgetSetMargin_pack4(MLK_WIDGET(p), margin_pack4);

	p->rd = 0;

	return p;
}

/** 角度をセット */

mlkbool RotateWidget_setAngle(RotateWidget *p,int angle)
{
	while(angle < -180) angle += 360;
	while(angle > 180) angle -= 360;

	if(angle == p->angle)
		return FALSE;
	else
	{
		p->angle = angle;
		p->rd = -angle / 180.0 * MLK_MATH_PI;

		mWidgetRedraw(MLK_WIDGET(p));

		return TRUE;
	}
}

