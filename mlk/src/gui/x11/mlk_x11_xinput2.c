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
 * <X11> XInput2
 *****************************************/

#include <string.h>

#define MLKX11_INC_XI2
#include "mlk_x11.h"

#include "mlk_list.h"
#include "mlk_event.h"


//----------------

#define _DEVICEID_MASTER_POINTER  2

#define _DEVITEM(p)  ((_device_item *)(p))

//デバイスリストアイテム

typedef struct
{
	mListItem i;
	
	int device_id,			//デバイス ID
		button_num,			//ボタン数
		pressure_number;	//筆圧のクラス番号 (-1 で筆圧情報なし)
	uint32_t flags;			//PENTABLET イベントのフラグ
	double pressure_min,	//筆圧最小値
		pressure_max,		//筆圧最大値
		pressure_last;		//最後の筆圧値
}_device_item;

//----------------


#if defined(MLK_X11_HAVE_XINPUT2)


//==============================
// sub
//==============================


/** デバイス名からタイプ取得
 *
 * return: 1=stylus, 2=eraser */

static int _get_device_type(char *name)
{
	int len;

	len = strlen(name);

	if(len >= 6 && strcasecmp(name + len - 6, "stylus") == 0)
		return 1;
	else if(len >= 6 && strcasecmp(name + len - 6, "eraser") == 0)
		return 2;
	else
		return 0;
}

/** デバイスリスト作成 */

static void _create_device_list(void)
{
	mAppX11 *app = MLKAPPX11;
	XIDeviceInfo *xidevi,*pi;
	XIAnyClassInfo *pclass;
	XIValuatorClassInfo *pvalclass;
	_device_item *item;
	int i,j,devnum,bttnum,pressnum;
	double pressmin,pressmax;
	Atom label_abs_pressure;

	//XIDeviceInfo 取得

	xidevi = XIQueryDevice(app->display, XIAllDevices, &devnum);
	if(!xidevi) return;

	//筆圧の Atom 値

	label_abs_pressure = XInternAtom(app->display, "Abs Pressure", False);

	//各デバイスごとに処理

	pi = xidevi;

	for(i = 0; i < devnum; i++, pi++)
	{
		//スレイブデバイスのみ (仮想デバイスは除く)

		if(pi->use != XISlavePointer
			|| strncasecmp(pi->name, "virtual core", 12) == 0)
			continue;

		//

		bttnum = 0;
		pressnum = -1;
		pressmin = pressmax = 1;

		//各クラスからボタンと筆圧の情報取得

		for(j = 0; j < pi->num_classes; j++)
		{
			pclass = pi->classes[j];

			if(pclass->type == XIButtonClass)
			{
				//ボタン数

				bttnum = ((XIButtonClassInfo *)pclass)->num_buttons;
			}
			else if(pclass->type == XIValuatorClass)
			{
				//筆圧

				pvalclass = (XIValuatorClassInfo *)pclass;

				if(pvalclass->label == label_abs_pressure)
				{
					pressnum = pvalclass->number;
					pressmin = pvalclass->min;
					pressmax = pvalclass->max;

					if(pressmin == pressmax)
						pressnum = -1;
				}
			}
		}

		//アイテム追加

		item = (_device_item *)mListAppendNew(&app->list_xidev, sizeof(_device_item));
		if(item)
		{
			item->device_id = pi->deviceid;
			item->button_num = bttnum;
			item->pressure_number = pressnum;
			item->pressure_min = pressmin;
			item->pressure_max = pressmax;

			//フラグ: 筆圧あり

			if(pressnum != -1)
				item->flags |= MEVENT_PENTABLET_F_HAVE_PRESSURE;

			//フラグ: タイプ

			j = _get_device_type(pi->name);

			if(j == 1)
				item->flags |= MEVENT_PENTABLET_F_STYLUS;
			else if(j == 2)
				item->flags |= MEVENT_PENTABLET_F_ERASER;

		#if 0
			mDebug("XI2 device | id(%d) bttnum(%d) pmin(%.3f) pmax(%.3f) flags(%u)\n",
				pi->deviceid, bttnum, pressmin, pressmax, item->flags);
		#endif
		}
	}

	XIFreeDeviceInfo(xidevi);
}


//==============================
// main
//==============================


/** XI2 初期化
 *
 * return: XI2 イベント用のコード。-1 でエラー。 */

int mX11XI2_init(void)
{
	int opcode,event,error,major = 2,minor = 0;

	if(!XQueryExtension(MLKX11_DISPLAY, "XInputExtension", &opcode, &event, &error))
		return -1;

	if(XIQueryVersion(MLKX11_DISPLAY, &major, &minor) == BadRequest
		|| major < 2)
		return -1;

	_create_device_list();

	return opcode;
}

/** ウィンドウが XI2 イベントを受け取るようにする (ウィンドウ作成時) */

void mX11XI2_selectEvent(Window id)
{
	XIEventMask em;
	uint8_t mask[1] = {0};

	em.deviceid = _DEVICEID_MASTER_POINTER;
	em.mask_len = 1;
	em.mask = mask;

	XISetMask(mask, XI_ButtonPress);
	XISetMask(mask, XI_ButtonRelease);
	XISetMask(mask, XI_Motion);

	XISelectEvents(MLKX11_DISPLAY, id, &em, 1);
}

/** XI2 イベントを処理して、情報取得
 *
 * dst_pressure: 筆圧が入る
 * return: PENTABLET イベントのフラグ */

uint32_t mX11XI2_procEvent(void *event,double *dst_pressure)
{
	XIDeviceEvent *ev = (XIDeviceEvent *)event;
	_device_item *pi;
	int i,pos;
	double press;

	//デバイス ID からリストアイテム検索

	for(pi = _DEVITEM(MLKAPPX11->list_xidev.top); pi; pi = _DEVITEM(pi->i.next))
	{
		if(pi->device_id == ev->sourceid) break;
	}

	//筆圧取得

	if(!pi || pi->pressure_number == -1)
	{
		//筆圧のないデバイス
		
		*dst_pressure = 1;
		return 0;
	}
	else
	{
		//valuators から筆圧取得
		// デバイスによっては、前回と同じ筆圧値の場合、値が送られてこない時があるので、
		// 値がない場合は前回の値を使う。

		if(!XIMaskIsSet(ev->valuators.mask, pi->pressure_number))
			press = pi->pressure_last;
		else
		{
			//値の位置
			
			for(i = pos = 0; i < pi->pressure_number; i++)
			{
				if(XIMaskIsSet(ev->valuators.mask, i)) pos++;
			}

			//0.0〜1.0 に正規化

			press = (ev->valuators.values[pos] - pi->pressure_min) / (pi->pressure_max - pi->pressure_min);
		}

		pi->pressure_last = press;

		*dst_pressure = press;

		return pi->flags;
	}
}

#endif //MLK_X11_HAVE_XINPUT2
