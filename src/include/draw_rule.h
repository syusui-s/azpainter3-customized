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

/**********************************
 * AppDraw: 定規関連
 **********************************/

enum
{
	DRAW_RULE_TYPE_OFF,
	DRAW_RULE_TYPE_LINE,
	DRAW_RULE_TYPE_GRID,
	DRAW_RULE_TYPE_CONCLINE,
	DRAW_RULE_TYPE_CIRCLE,
	DRAW_RULE_TYPE_ELLIPSE,
	DRAW_RULE_TYPE_SYMMETRY,

	DRAW_RULE_TYPE_NUM
};

void drawRule_setType(AppDraw *p,int type);
void drawRule_setType_onoff(AppDraw *p,int type);
mlkbool drawRule_isVisibleGuide(AppDraw *p);

void drawRule_onPress(AppDraw *p,mlkbool dotpen);
mlkbool drawRule_onPress_setting(AppDraw *p);

void drawRule_setting_line(AppDraw *p);
void drawRule_setting_point(AppDraw *p);
void drawRule_setting_ellipse(AppDraw *p);

void drawRule_setRecord(AppDraw *p,int no);
void drawRule_readRecord(AppDraw *p,int no);

