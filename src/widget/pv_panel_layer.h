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

/*******************************
 * (panel) レイヤの内部用
 *******************************/

typedef struct _PanelLayerList PanelLayerList;

//TRGROUP_PANEL_LAYER
enum
{
	TRID_LAYER_FILLREF,
	TRID_LAYER_LOCK,
	TRID_LAYER_CHECK,
	TRID_LAYER_TONE_GRAY,

	TRID_LAYER_ALPHAMASK_OFF = 100,
	TRID_LAYER_ALPHAMASK_ALPHA,
	TRID_LAYER_ALPHAMASK_TP,
	TRID_LAYER_ALPHAMASK_NOTTP,

	TRID_LAYER_CMD_TOOLTIP_TOP = 200,

	TRID_LAYER_MENU_HELP = 300
};

/* subwg */
mWidget *PanelLayer_param_create(mWidget *parent);
void PanelLayer_param_setValue(mWidget *wg);

/* layer_list */
PanelLayerList *PanelLayerList_new(mWidget *parent);
void PanelLayerList_setScrollInfo(PanelLayerList *p);
void PanelLayerList_drawLayer(PanelLayerList *p,LayerItem *pi);
void PanelLayerList_changeCurrent_visible(PanelLayerList *p,LayerItem *lastitem,mlkbool update_all);
