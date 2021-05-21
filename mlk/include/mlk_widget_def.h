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

#ifndef MLK_WIDGET_DEF_H
#define MLK_WIDGET_DEF_H

typedef struct _mWindowDecoInfo mWindowDecoInfo;
typedef int (*mFuncWidgetHandle_event)(mWidget *,mEvent *);

/*--------*/

typedef struct
{
	uint16_t left,top,right,bottom;
}mWidgetRect;

typedef struct _mWidgetLabelTextLineInfo
{
	int pos,len,width;
}mWidgetLabelTextLineInfo;

typedef struct _mWidgetLabelText
{
	char *text;
	mWidgetLabelTextLineInfo *linebuf;
	mSize szfull;
	int fcopy;
}mWidgetLabelText;

typedef struct _mWidgetTextEdit
{
	mWidget *widget;
	uint32_t *unibuf;
	uint16_t *wbuf;
	int textlen,
		alloclen,
		curpos,
		curx,
		maxwidth,
		seltop,
		selend,
		multi_curline,
		multi_maxlines,
		is_multiline;
}mWidgetTextEdit;

typedef struct _mCIManager
{
	mList list;
	mColumnItem *item_focus;
	int is_multi_sel;
}mCIManager;


/*---- mWidget ----*/

struct _mWidget
{
	mWindow *toplevel;
	mWidget *parent,
		*prev,*next,
		*first,*last,
		*notify_widget,
		*construct_next;
	mFont *font;
	mCursor cursor,
		cursor_cache;
	
	int x,y,w,h,
		id,
		absX,absY,
		hintW,hintH,
		cursor_cache_type;
	uint16_t initW, initH,
		hintRepW, hintRepH,
		hintMinW, hintMinH;
	uint32_t fstate,
		foption,
		flayout,
		fui,
		fevent,
		ftype;
	uint8_t facceptkey,
		notify_to,
		notify_to_rep;
	intptr_t param1,
		param2;
	
	mWidgetRect margin;
	mRect draw_rect;
	
	void (*destroy)(mWidget *);
	int (*event)(mWidget *,mEvent *);
	void (*draw)(mWidget *,mPixbuf *);
	void (*draw_bkgnd)(mWidget *,mPixbuf *,mBox *);
	void (*calc_hint)(mWidget *);
	void (*calc_layout_size)(mWidget *,int *w,int *h);
	void (*layout)(mWidget *);
	void (*resize)(mWidget *);
	void (*construct)(mWidget *);
	void (*get_inputmethod_pos)(mWidget *p,mPoint *pt);
};


enum MWIDGET_TYPE_FLAGS
{
	MWIDGET_TYPE_CONTAINER = 1<<0,
	MWIDGET_TYPE_WINDOW = 1<<1,
	MWIDGET_TYPE_WINDOW_TOPLEVEL = 1<<2,
	MWIDGET_TYPE_WINDOW_POPUP = 1<<3,
	MWIDGET_TYPE_DIALOG = 1<<4,
	MWIDGET_TYPE_CHECKBUTTON = 1<<5
};

enum MWIDGET_STATE_FLAGS
{
	MWIDGET_STATE_VISIBLE      = 1<<0,
	MWIDGET_STATE_ENABLED      = 1<<1,
	MWIDGET_STATE_FOCUSED      = 1<<2,
	MWIDGET_STATE_TAKE_FOCUS   = 1<<3,
	MWIDGET_STATE_ENTER_SEND   = 1<<4,
	MWIDGET_STATE_ENABLE_DROP  = 1<<5,
	MWIDGET_STATE_ENABLE_KEY_REPEAT = 1<<6,
	MWIDGET_STATE_ENABLE_INPUT_METHOD = 1<<7
};

enum MWIDGET_OPTION_FLAGS
{
	MWIDGET_OPTION_NO_DRAW_BKGND = 1<<0,
	MWIDGET_OPTION_KEYEVENT_ALL_TO_WINDOW  = 1<<1,
	MWIDGET_OPTION_KEYEVENT_NORM_TO_WINDOW = 1<<2,
	MWIDGET_OPTION_SCROLL_TO_POINTER = 1<<3,
	MWIDGET_OPTION_NO_TAKE_FOCUS = 1<<4
};

enum MWIDGET_UI_FLAGS
{
	MWIDGET_UI_DESTROY      = 1<<0,
	MWIDGET_UI_FOLLOW_DESTROY = 1<<1,
	MWIDGET_UI_UPDATE       = 1<<2,
	MWIDGET_UI_FOLLOW_DRAW  = 1<<3,
	MWIDGET_UI_DRAW         = 1<<4,
	MWIDGET_UI_FOLLOW_CALC  = 1<<5,
	MWIDGET_UI_CALC         = 1<<6,
	MWIDGET_UI_LAYOUTED     = 1<<7,
	MWIDGET_UI_DRAWED_BKGND = 1<<8,
	MWIDGET_UI_DECO_UPDATE  = 1<<9,
	MWIDGET_UI_CONSTRUCT    = 1<<10
};

enum MWIDGET_EVENT_FLAGS
{
	MWIDGET_EVENT_POINTER = 1<<0,
	MWIDGET_EVENT_SCROLL  = 1<<1,
	MWIDGET_EVENT_KEY     = 1<<2,
	MWIDGET_EVENT_CHAR    = 1<<3,
	MWIDGET_EVENT_STRING  = 1<<4,
	MWIDGET_EVENT_PENTABLET = 1<<5
};

enum MWIDGET_ACCEPTKEY_FLAGS
{
	MWIDGET_ACCEPTKEY_TAB    = 1<<0,
	MWIDGET_ACCEPTKEY_ENTER  = 1<<1,
	MWIDGET_ACCEPTKEY_ESCAPE = 1<<2,
	
	MWIDGET_ACCEPTKEY_ALL = 0xff
};

enum MWIDGET_NOTIFYTO
{
	MWIDGET_NOTIFYTO_PARENT_NOTIFY,
	MWIDGET_NOTIFYTO_SELF,
	MWIDGET_NOTIFYTO_PARENT,
	MWIDGET_NOTIFYTO_PARENT2,
	MWIDGET_NOTIFYTO_TOPLEVEL,
	MWIDGET_NOTIFYTO_WIDGET
};

enum MWIDGET_NOTIFYTOREP
{
	MWIDGET_NOTIFYTOREP_NONE,
	MWIDGET_NOTIFYTOREP_SELF
};

enum MWIDGET_LAYOUT_FLAGS
{
	MLF_FIX_HINT_W = 0,
	MLF_FIX_HINT_H = 0,
	MLF_FIX_W      = 1<<0,
	MLF_FIX_H      = 1<<1,
	MLF_EXPAND_X   = 1<<2,
	MLF_EXPAND_Y   = 1<<3,
	MLF_EXPAND_W   = 1<<4,
	MLF_EXPAND_H   = 1<<5,

	MLF_LEFT       = 0,
	MLF_TOP        = 0,
	MLF_RIGHT      = 1<<6,
	MLF_BOTTOM     = 1<<7,
	MLF_CENTER     = 1<<8,
	MLF_MIDDLE     = 1<<9,

	MLF_GRID_COL_W = 1<<10,
	MLF_GRID_ROW_H = 1<<11,

	MLF_CENTER_XY = MLF_CENTER | MLF_MIDDLE,
	MLF_EXPAND_XY = MLF_EXPAND_X | MLF_EXPAND_Y,
	MLF_EXPAND_WH = MLF_EXPAND_W | MLF_EXPAND_H,
	MLF_FIX_WH    = MLF_FIX_W | MLF_FIX_H
};


/*---- mContainer ----*/

#define MLK_CONTAINER_DEF mWidget wg; mContainerData ct;

typedef struct
{
	int type,
		grid_cols;
	uint16_t sep,
		sep_grid_v;
	mWidgetRect padding;
	void (*calc_hint)(mWidget *);
}mContainerData;

struct _mContainer
{
	mWidget wg;
	mContainerData ct;
};

enum MCONTAINER_TYPE
{
	MCONTAINER_TYPE_VERT,
	MCONTAINER_TYPE_HORZ,
	MCONTAINER_TYPE_GRID
};


/*--- mWindow ---*/

#define MLK_WINDOW_DEF MLK_CONTAINER_DEF mWindowData win;

typedef struct
{
	mWidgetRect w;

	void (*setwidth)(mWidget *p,int type);
	void (*update)(mWidget *p,int type,mWindowDecoInfo *info);
	void (*draw)(mWidget *p,mPixbuf *pixbuf,mWindowDecoInfo *info);
}mWindowDecoData;

typedef struct
{
	void *bkend;

	mPixbuf *pixbuf;
	mWidget *parent,
		*focus,
		*focus_ready;
	mRect update_rc;
	mWindowDecoData deco;

	int win_width,
		win_height,
		norm_width,
		norm_height;
	uint8_t fstate,
		focus_ready_type;
}mWindowData;

struct _mWindow
{
	MLK_CONTAINER_DEF
	mWindowData win;
};

enum MWINDOW_STATE_FLAGS
{
	MWINDOW_STATE_MAP = 1<<0,
	MWINDOW_STATE_MAXIMIZED  = 1<<1,
	MWINDOW_STATE_FULLSCREEN = 1<<2
};


/*--- mToplevel ---*/

#define MLK_TOPLEVEL_DEF MLK_WINDOW_DEF mToplevelData top;

typedef struct
{
	mMenuBar *menubar;
	mAccelerator *accelerator;
	uint32_t fstyle;
	int (*keydown)(mWidget *wg,uint32_t key,uint32_t state);
}mToplevelData;

struct _mToplevel
{
	MLK_WINDOW_DEF
	mToplevelData top;
};

enum MTOPLEVEL_STYLE
{
	MTOPLEVEL_S_TITLE      = 1<<1,
	MTOPLEVEL_S_BORDER     = 1<<2,
	MTOPLEVEL_S_CLOSE      = 1<<3,
	MTOPLEVEL_S_SYSMENU    = 1<<4,
	MTOPLEVEL_S_MINIMIZE   = 1<<5,
	MTOPLEVEL_S_MAXIMIZE   = 1<<6,
	MTOPLEVEL_S_TAB_MOVE   = 1<<7,
	MTOPLEVEL_S_NO_INPUT_METHOD = 1<<8,
	MTOPLEVEL_S_NO_RESIZE  = 1<<9,
	MTOPLEVEL_S_DIALOG     = 1<<10,
	MTOPLEVEL_S_PARENT     = 1<<11,
	MTOPLEVEL_S_ENABLE_PENTABLET = 1<<12,
	
	MTOPLEVEL_S_NORMAL = MTOPLEVEL_S_TITLE | MTOPLEVEL_S_BORDER
		| MTOPLEVEL_S_CLOSE | MTOPLEVEL_S_SYSMENU | MTOPLEVEL_S_MINIMIZE
		| MTOPLEVEL_S_MAXIMIZE,

	MTOPLEVEL_S_DIALOG_NORMAL = MTOPLEVEL_S_NORMAL | MTOPLEVEL_S_DIALOG
		| MTOPLEVEL_S_TAB_MOVE | MTOPLEVEL_S_PARENT
};


/*--- mDialog ---*/

#define MLK_DIALOG_DEF MLK_TOPLEVEL_DEF mDialogData dlg;

typedef struct
{
	intptr_t retval;
}mDialogData;

struct _mDialog
{
	MLK_TOPLEVEL_DEF
	mDialogData dlg;
};


/*--- mPopup ---*/

#define MLK_POPUP_DEF MLK_WINDOW_DEF mPopupData pop;

typedef struct
{
	uint32_t fstyle;
	void (*quit)(mWidget *,mlkbool is_cancel);
}mPopupData;

struct _mPopup
{
	MLK_WINDOW_DEF
	mPopupData pop;
};

enum MPOPUP_STYLE
{
	MPOPUP_S_NO_GRAB = 1<<0,
	MPOPUP_S_NO_EVENT = 1<<1
};

#endif
