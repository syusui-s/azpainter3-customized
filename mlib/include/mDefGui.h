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

#ifndef MLIB_DEF_GUI_H
#define MLIB_DEF_GUI_H

typedef uint32_t  mRgbCol;
typedef uint32_t  mPixCol;
typedef uintptr_t mCursor;
typedef struct _mWidget    mWidget;
typedef struct _mContainer mContainer;
typedef struct _mWindow    mWindow;
typedef struct _mMenu      mMenu;
typedef struct _mEvent     mEvent;
typedef struct _mPixbuf    mPixbuf;
typedef struct _mImageBuf  mImageBuf;
typedef struct _mImageList mImageList;

#define M_WID_OK		1
#define M_WID_CANCEL	2

#define M_WIDGET(p)    ((mWidget *)(p))
#define M_CONTAINER(p) ((mContainer *)(p))
#define M_WINDOW(p)    ((mWindow *)(p))

#define MACCKEY_SHIFT   0x8000
#define MACCKEY_CTRL    0x4000
#define MACCKEY_ALT     0x2000
#define MACCKEY_KEYMASK 0x0fff
#define MACCKEY_MODMASK 0xe000

enum M_MODSTATE
{
	M_MODS_SHIFT = 1<<0,
	M_MODS_CTRL  = 1<<1,
	M_MODS_ALT   = 1<<2,
	
	M_MODS_MASK_KEY = M_MODS_SHIFT|M_MODS_CTRL|M_MODS_ALT
};

enum M_BUTTON
{
	M_BTT_NONE,
	M_BTT_LEFT,
	M_BTT_MIDDLE,
	M_BTT_RIGHT,
	M_BTT_SCR_UP,
	M_BTT_SCR_DOWN,
	M_BTT_SCR_LEFT,
	M_BTT_SCR_RIGHT
};


typedef struct
{
	uint16_t left,top,right,bottom;
}mWidgetSpacing;


/* function */

#ifdef __cplusplus
extern "C" {
#endif

mPixCol mRGBtoPix(mRgbCol col);
mPixCol mRGBtoPix2(uint8_t r,uint8_t g,uint8_t b);
mPixCol mGraytoPix(uint8_t c);
mRgbCol mPixtoRGB(mPixCol col);
mRgbCol mBlendRGB_256(mRgbCol colsrc,mRgbCol coldst,int a);

#ifdef __cplusplus
}
#endif

#endif
