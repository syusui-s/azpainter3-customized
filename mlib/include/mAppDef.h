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

#ifndef MLIB_APPDEF_H
#define MLIB_APPDEF_H

typedef struct _mAppPrivate mAppPrivate;
typedef struct _mAppSystem  mAppSystem;

typedef struct _mApp
{
	mAppPrivate *pv;
	mAppSystem  *sys;

	mWidget *widgetRoot,
			*widgetOver,
			*widgetGrab,
			*widgetGrabKey;
	
	mFont *fontDefault;

	char *pathConfig,
		*pathData,
		*res_appname,
		*res_classname,
		*argv_progname;

	int depth;
	uint32_t maskR,maskG,maskB;
	uint16_t filedialog_config[3];
	uint8_t flags,
		r_shift_left,
		g_shift_left,
		b_shift_left,
		r_shift_right,
		g_shift_right,
		b_shift_right;
}mApp;

extern mApp *g_mApp;


enum MAPP_FLAGS
{
	MAPP_FLAGS_DEBUG_EVENT  = 1<<0,
	MAPP_FLAGS_DISABLE_GRAB = 1<<1,
	MAPP_FLAGS_BLOCK_USER_ACTION = 1<<2
};

#define MAPP     (g_mApp)
#define MAPP_SYS (g_mApp->sys)
#define MAPP_PV  (g_mApp->pv)

#endif
