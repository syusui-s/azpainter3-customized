/*$
 Copyright (C) 2013-2020 Azel.

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

#define MINC_X11_XI2
#include "mSysX11.h"

#include "mList.h"
#include "mEvent.h"
#include "x11_xinput2.h"


//----------------

#define _DEVICEID_MASTER_POINTER  2

typedef struct
{
	mListItem i;
	
	int device_id,
		button_num,
		pressure_number;
	uint32_t flags;
	double pressure_min,
		pressure_max,
		pressure_last;
}DeviceItem;

#define _DEVITEM(p)  ((DeviceItem *)(p))

//----------------


#if defined(MLIB_ENABLE_PENTABLET) && defined(HAVE_XEXT_XINPUT2)


//==============================


/** デバイス名からタイプ取得
 *
 * @return [1]stylus [2]eraser */

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

static void _create_device_list()
{
	XIDeviceInfo *xidevi,*pi;
	XIAnyClassInfo *pclass;
	XIValuatorClassInfo *pvalclass;
	DeviceItem *item;
	int i,j,devnum,bttnum,pressnum;
	double pressmin,pressmax;
	Atom label_abs_pressure;

	xidevi = XIQueryDevice(XDISP, XIAllDevices, &devnum);
	if(!xidevi) return;

	label_abs_pressure = XInternAtom(XDISP, "Abs Pressure", False);

	for(i = 0, pi = xidevi; i < devnum; i++, pi++)
	{
		//スレイブデバイスのみ(仮想デバイスは除く)

		if(pi->use != XISlavePointer
			|| strncasecmp(pi->name, "virtual core", 12) == 0)
			continue;

		//

		bttnum = 0;
		pressnum = -1;
		pressmin = pressmax = 1;

		//

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
				}
			}
		}

		//追加

		item = (DeviceItem *)mListAppendNew(&MAPP_SYS->listXiDev,
					sizeof(DeviceItem), NULL);
		if(item)
		{
			item->device_id = pi->deviceid;
			item->button_num = bttnum;
			item->pressure_number = pressnum;
			item->pressure_min = pressmin;
			item->pressure_max = pressmax;
			item->flags = 0;

			j = _get_device_type(pi->name);
			if(j == 1)
				item->flags |= MEVENT_PENTAB_FLAGS_STYLUS;
			else if(j == 2)
				item->flags |= MEVENT_PENTAB_FLAGS_ERASER;
		}
	}

	XIFreeDeviceInfo(xidevi);
}


//==============================


/** 初期化
 *
 * @return -1 でエラー。戻り値はイベント処理時に使う。 */

int mX11XI2_init()
{
	int opcode,event,error,major = 2,minor = 0;

	if(!XQueryExtension(XDISP, "XInputExtension", &opcode, &event, &error))
		return -1;

	if(XIQueryVersion(XDISP, &major, &minor) == BadRequest || major < 2)
		return -1;

	_create_device_list();

	return opcode;
}

/** ウィンドウが XI2 イベントを受け取るようにする */

void mX11XI2_pt_selectEvent(Window id)
{
	XIEventMask em;
	uint8_t mask[1] = {0};

	em.deviceid = _DEVICEID_MASTER_POINTER;
	em.mask_len = 1;
	em.mask     = mask;

	XISetMask(mask, XI_ButtonPress);
	XISetMask(mask, XI_ButtonRelease);
	XISetMask(mask, XI_Motion);

	XISelectEvents(XDISP, id, &em, 1);
}

/** グラブ */

mBool mX11XI2_pt_grab(Window id,Cursor cursor,int device_id)
{
	XIEventMask em;
	uint8_t mask[1] = {0};

	if(MAPP_SYS->xi2_grab)
		return FALSE;
	else
	{
		em.deviceid = _DEVICEID_MASTER_POINTER;
		em.mask_len = 1;
		em.mask     = mask;

		XISetMask(mask, XI_ButtonPress);
		XISetMask(mask, XI_ButtonRelease);
		XISetMask(mask, XI_Motion);

		if(XIGrabDevice(XDISP, _DEVICEID_MASTER_POINTER,
			id, CurrentTime, cursor,
			GrabModeAsync, GrabModeAsync, False, &em))
			return FALSE;
		else
		{
			MAPP_SYS->xi2_grab = 1;
			MAPP_SYS->xi2_grab_deviceid = device_id;
			return TRUE;
		}
	}
}

/** グラブ解除 */

void mX11XI2_pt_ungrab()
{
	if(MAPP_SYS->xi2_grab)
	{
		XIUngrabDevice(XDISP, _DEVICEID_MASTER_POINTER, CurrentTime);
		XFlush(XDISP);

		MAPP_SYS->xi2_grab = 0;
	}
}

/** イベントから情報取得 */

uint32_t mX11XI2_getEventInfo(void *event,double *pressure,mBool *pentablet)
{
	XIDeviceEvent *ev = (XIDeviceEvent *)event;
	DeviceItem *pi;
	int i,pos;
	double press;

	//デバイスアイテム

	for(pi = _DEVITEM(MAPP_SYS->listXiDev.top); pi; pi = _DEVITEM(pi->i.next))
	{
		if(pi->device_id == ev->sourceid) break;
	}

	//筆圧取得

	if(!pi || pi->pressure_number == -1)
	{
		//筆圧情報なし
		
		*pressure = 1;
		*pentablet = FALSE;
	}
	else
	{
		/* valuators から取得
		 * (デバイスによっては、前回と同じ値の場合は値が送られてこない時があるので、
		 *  値がない場合は前回の値を使う) */

		if(!XIMaskIsSet(ev->valuators.mask, pi->pressure_number))
			press = pi->pressure_last;
		else
		{
			for(i = pos = 0; i < pi->pressure_number; i++)
			{
				if(XIMaskIsSet(ev->valuators.mask, i)) pos++;
			}

			press = (ev->valuators.values[pos] - pi->pressure_min) / (pi->pressure_max - pi->pressure_min);
		}

		pi->pressure_last = press;

		*pressure = press;
		*pentablet = TRUE;
	}

	return (pi)? pi->flags: 0;
}

#endif
