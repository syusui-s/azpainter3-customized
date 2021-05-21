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
 * [Panel] カラーホイール
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_panel.h"
#include "mlk_event.h"

#include "def_widget.h"

#include "draw_main.h"

#include "panel.h"
#include "panel_func.h"
#include "colorwheel.h"



//========================
// ハンドラ
//========================


/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY && ev->notify.id == 100)
	{
		if(ev->notify.notify_type == COLORWHEEL_N_CHANGE_COL)
		{
			//色変更
			// param1:RGB, param2:CHANGECOL
			
			drawColor_setDrawColor(ev->notify.param1);
			
			PanelColor_setColor_fromWheel(ev->notify.param2);
		}
	}

	return 1;
}

/* パネル内容作成 */

static mWidget *_panel_create_handle(mPanel *panel,int id,mWidget *parent)
{
	mWidget *top;

	//コンテナ

	top = mContainerCreateVert(parent, 0, MLF_EXPAND_WH, 0);

	top->event = _event_handle;
	top->notify_to = MWIDGET_NOTIFYTO_SELF;

	mContainerSetPadding_pack4(MLK_CONTAINER(top), MLK_MAKE32_4(8,2,0,2));

	//ホイール

	top->param1 = (intptr_t)ColorWheel_new(top, 100, drawColor_getDrawColor());
	
	return top;
}

/** 作成 */

void PanelColorWheel_new(mPanelState *state)
{
	mPanel *p;

	p = Panel_new(PANEL_COLOR_WHEEL, 0, 0, 0, state, _panel_create_handle);

	mPanelCreateWidget(p);
}

/** 色をセット
 *
 * col: CHANGECOL */

void PanelColorWheel_setColor(uint32_t col)
{
	mWidget *p = Panel_getContents(PANEL_COLOR_WHEEL);

	if(p)
		ColorWheel_setColor((ColorWheel *)p->param1, col);
}

