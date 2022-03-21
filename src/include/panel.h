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

/********************************************
 * Panel : パネル
 ********************************************/

typedef struct _mPanelState mPanelState;

typedef mWidget *(*PanelHandle_create)(mPanel *p,int id,mWidget *parent);


mPanel *Panel_new(int no,int exsize,uint32_t panel_style,
	uint32_t window_style,mPanelState *state,PanelHandle_create create);

mlkbool Panel_isCreated(int no);
mlkbool Panel_isVisible(int no);
void *Panel_getExPtr(int no);
mWidget *Panel_getContents(int no);
mWindow *Panel_getDialogParent(int no);
void Panel_relayout(int no);

int Panel_id_to_no(int id);
int Panel_getPaneNo(int no);

//パネル全体処理
void Panels_destroy(void);
void Panels_showInit(void);
void Panels_toggle_winmode(int type);
void Panels_relocate(void);
