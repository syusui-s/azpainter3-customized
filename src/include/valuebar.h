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

/************************************
 * スピン・小数部付きのバー
 ************************************/

#define VALUEBAR(p)  ((ValueBar *)(p))

typedef struct _ValueBar ValueBar;

//通知 param2
enum
{
	VALUEBAR_ACT_F_CHANGE_POS = 1<<0,
	VALUEBAR_ACT_F_PRESS = 1<<1,
	VALUEBAR_ACT_F_RELEASE = 1<<2,
	VALUEBAR_ACT_F_DRAG = 1<<3,
	VALUEBAR_ACT_F_SPIN = 1<<4,
	VALUEBAR_ACT_F_WHEEL = 1<<5,

	VALUEBAR_ACT_F_ONCE = VALUEBAR_ACT_F_RELEASE | VALUEBAR_ACT_F_SPIN | VALUEBAR_ACT_F_WHEEL
};


ValueBar *ValueBar_new(mWidget *parent,int id,uint32_t flayout,
	int dig,int min,int max,int pos);

int ValueBar_getPos(ValueBar *p);
mlkbool ValueBar_setPos(ValueBar *p,int pos);
void ValueBar_setStatus(ValueBar *p,int min,int max,int pos);
void ValueBar_setStatus_dig(ValueBar *p,int dig,int min,int max,int pos);

