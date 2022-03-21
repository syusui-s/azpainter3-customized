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

#ifndef MLK_EVENT_H
#define MLK_EVENT_H

typedef struct _mEventBase
{
	int type;
	mWidget *widget;
}mEventBase;

typedef struct
{
	int type;
	mWidget *widget;

	uint32_t key,
		state,
		raw_keysym;
	int raw_code;
	uint8_t is_grab_pointer;
}mEventKey;

typedef struct
{
	int type;
	mWidget *widget;

	char ch;
}mEventChar;

typedef struct
{
	int type;
	mWidget *widget;

	int width,height;
	uint32_t state,state_mask,flags;
}mEventConfigure;

typedef struct
{
	int type;
	mWidget *widget;

	int act,x,y,btt,raw_btt,
		fixed_x,fixed_y;
	uint32_t state;
}mEventPointer;

typedef struct
{
	int type;
	mWidget *widget;

	int horz_val,
		vert_val,
		horz_step,
		vert_step;
	uint32_t state;
	uint8_t is_grab_pointer;
}mEventScroll;

typedef struct
{
	int type;
	mWidget *widget;

	int from;
	mlkbool is_out;
}mEventFocus;

typedef struct
{
	int type;
	mWidget *widget;

	int x,y,
		fixed_x,fixed_y;
}mEventEnter;

typedef struct
{
	int type;
	mWidget *widget;

	mWidget *widget_from;
	int notify_type,id;
	intptr_t param1,param2;
}mEventNotify;

typedef struct
{
	int type;
	mWidget *widget;

	int id,from;
	intptr_t param;
	void *from_ptr;
}mEventCommand;

typedef struct
{
	int type;
	mWidget *widget;

	int id;
	intptr_t param;
}mEventTimer;

typedef struct
{
	int type;
	mWidget *widget;

	mMenu *menu;
	int submenu_id;
	mlkbool is_menubar;
}mEventMenuPopup;

typedef struct
{
	int type;
	mWidget *widget;

	int len;
	char text[1];
}mEventString;

typedef struct
{
	int type;
	mWidget *widget;

	char **files;
}mEventDropFiles;

typedef struct
{
	int type;
	mWidget *widget;

	int act,btt,raw_btt;
	uint32_t state,flags;
	double x,y,pressure;
}mEventPenTablet;

typedef struct
{
	int type;
	mWidget *widget;

	int decotype,
		x,y,btt;
}mEventWinDeco;

typedef struct
{
	int type;
	mWidget *widget;

	mPanel *panel;
	int id,act;
	intptr_t param1,param2;
}mEventPanel;

typedef union _mEvent
{
	struct
	{
		int type;
		mWidget *widget;
	};
	mEventKey key;
	mEventChar ch;
	mEventCommand cmd;
	mEventConfigure config;
	mEventFocus focus;
	mEventMenuPopup popup;
	mEventNotify notify;
	mEventPenTablet pentab;
	mEventPointer pt;
	mEventScroll scroll;
	mEventString str;
	mEventTimer timer;
	mEventEnter enter;
	mEventWinDeco deco;
	mEventPanel panel;
	mEventDropFiles dropfiles;
}mEvent;


/*------------*/

enum MEVENT_TYPE
{
	MEVENT_CLOSE = 1,
	MEVENT_MAP,
	MEVENT_UNMAP,
	MEVENT_CONFIGURE,
	MEVENT_FOCUS,
	MEVENT_ENTER,
	MEVENT_LEAVE,
	MEVENT_KEYDOWN,
	MEVENT_KEYUP,
	MEVENT_CHAR,
	MEVENT_STRING,
	MEVENT_POINTER,
	MEVENT_POINTER_MODAL,
	MEVENT_SCROLL,
	MEVENT_TIMER,
	MEVENT_NOTIFY,
	MEVENT_COMMAND,
	MEVENT_MENU_POPUP,
	MEVENT_PENTABLET,
	MEVENT_PENTABLET_MODAL,
	MEVENT_WINDECO,
	MEVENT_PANEL,
	MEVENT_DROP_FILES,

	MEVENT_USER = 10000
};

enum MEVENT_POINTER_ACT
{
	MEVENT_POINTER_ACT_MOTION = 0,
	MEVENT_POINTER_ACT_PRESS,
	MEVENT_POINTER_ACT_RELEASE,
	MEVENT_POINTER_ACT_DBLCLK
};

enum MEVENT_CONFIGURE_FLAGS
{
	MEVENT_CONFIGURE_F_SIZE = 1<<0,
	MEVENT_CONFIGURE_F_STATE = 1<<1
};

enum MEVENT_CONFIGURE_STATE
{
	MEVENT_CONFIGURE_STATE_MAXIMIZED = 1<<0,
	MEVENT_CONFIGURE_STATE_FULLSCREEN = 1<<1
};

enum MEVENT_FOCUS_FROM
{
	MEVENT_FOCUS_FROM_UNKNOWN,
	MEVENT_FOCUS_FROM_WINDOW_FOCUS,
	MEVENT_FOCUS_FROM_WINDOW_RESTORE,
	MEVENT_FOCUS_FROM_KEY_MOVE,
	MEVENT_FOCUS_FROM_FORCE_UNGRAB
};

enum MEVENT_COMMAND_FROM
{
	MEVENT_COMMAND_FROM_UNKNOWN,
	MEVENT_COMMAND_FROM_MENU,
	MEVENT_COMMAND_FROM_ACCELERATOR,
	MEVENT_COMMAND_FROM_ICONBAR_BUTTON,
	MEVENT_COMMAND_FROM_ICONBAR_DROP
};

enum MEVENT_PENTABLET_FLAGS
{
	MEVENT_PENTABLET_F_HAVE_PRESSURE = 1<<0,
	MEVENT_PENTABLET_F_STYLUS = 1<<1,
	MEVENT_PENTABLET_F_ERASER = 1<<2
};

enum MEVENT_WINDECO_TYPE
{
	MEVENT_WINDECO_TYPE_ENTER,
	MEVENT_WINDECO_TYPE_LEAVE,
	MEVENT_WINDECO_TYPE_MOTION,
	MEVENT_WINDECO_TYPE_PRESS
};

#endif
