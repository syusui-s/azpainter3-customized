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

#ifndef MLK_GUI_H
#define MLK_GUI_H

#include "mlk.h"

typedef uintptr_t mCursor;
typedef struct _mToplevelSaveState mToplevelSaveState;

typedef struct _mWidget    mWidget;
typedef struct _mContainer mContainer;
typedef struct _mWindow    mWindow;
typedef struct _mToplevel  mToplevel;
typedef struct _mDialog    mDialog;
typedef struct _mPopup     mPopup;
typedef struct _mTooltip   mTooltip;
typedef struct _mMenu      mMenu;
typedef union  _mEvent     mEvent;
typedef struct _mAccelerator mAccelerator;
typedef struct _mMenuItem  mMenuItem;
typedef struct _mColumnItem mColumnItem;

typedef struct _mLabel mLabel;
typedef struct _mButton mButton;
typedef struct _mCheckButton mCheckButton;
typedef struct _mArrowButton mArrowButton;
typedef struct _mColorButton mColorButton;
typedef struct _mImgButton mImgButton;
typedef struct _mFontButton mFontButton;
typedef struct _mLineEdit mLineEdit;
typedef struct _mColorPrev mColorPrev;
typedef struct _mComboBox mComboBox;
typedef struct _mGroupBox mGroupBox;
typedef struct _mMenuBar mMenuBar;
typedef struct _mIconBar mIconBar;
typedef struct _mProgressBar mProgressBar;
typedef struct _mScrollBar mScrollBar;
typedef struct _mSliderBar mSliderBar;
typedef struct _mListHeader mListHeader;
typedef struct _mScrollView mScrollView;
typedef struct _mScrollViewPage mScrollViewPage;
typedef struct _mListView mListView;
typedef struct _mListViewPage mListViewPage;
typedef struct _mInputAccel mInputAccel;
typedef struct _mHSVPicker mHSVPicker;
typedef struct _mContainerView mContainerView;
typedef struct _mTab mTab;
typedef struct _mSplitter mSplitter;
typedef struct _mMultiEdit mMultiEdit;
typedef struct _mExpander mExpander;
typedef struct _mFileListView mFileListView;
typedef struct _mFileDialog mFileDialog;
typedef struct _mConfListView mConfListView;
typedef struct _mPopupProgress mPopupProgress;
typedef struct _mPanelHeader mPanelHeader;
typedef struct _mPanel mPanel;
typedef struct _mPager mPager;
typedef struct _mFileInput mFileInput;


#define MLK_WID_OK		1
#define MLK_WID_CANCEL	2

#define MLK_WIDGET(p)      ((mWidget *)(p))
#define MLK_CONTAINER(p)   ((mContainer *)(p))
#define MLK_WINDOW(p)      ((mWindow *)(p))
#define MLK_TOPLEVEL(p)    ((mToplevel *)(p))
#define MLK_DIALOG(p)      ((mDialog *)(p))
#define MLK_POPUP(p)       ((mPopup *)(p))
#define MLK_TOOLTIP(p)     ((mTooltip *)(p))

#define MLK_TRGROUP_ID_SYSTEM  0xffff

#define MLK_TRGROUP(id)  mGuiTransSetGroup(id)
#define MLK_TR(id)       mGuiTransGetText(id)
#define MLK_TR2(gid,id)  mGuiTransGetText2(gid, id)
#define MLK_TR_SYS(id)   mGuiTransGetText2(MLK_TRGROUP_ID_SYSTEM, id)


struct _mToplevelSaveState
{
	int x,y,w,h,norm_x,norm_y;
	uint8_t flags;
};

enum MTOPLEVEL_SAVESTATE_FLAGS
{
	MTOPLEVEL_SAVESTATE_F_HAVE_POS = 1<<0,
	MTOPLEVEL_SAVESTATE_F_MAXIMIZED = 1<<1,
	MTOPLEVEL_SAVESTATE_F_FULLSCREEN = 1<<2
};


enum MLK_ACCELKEY
{
	MLK_ACCELKEY_SHIFT = 0x10000,
	MLK_ACCELKEY_CTRL  = 0x20000,
	MLK_ACCELKEY_ALT   = 0x40000,
	MLK_ACCELKEY_LOGO  = 0x80000,
	MLK_ACCELKEY_MASK_KEY = 0xffff,
	MLK_ACCELKEY_MASK_STATE = 0xffff0000
};

enum MLK_STATE
{
	MLK_STATE_SHIFT = 1<<0,
	MLK_STATE_CTRL  = 1<<1,
	MLK_STATE_ALT   = 1<<2,
	MLK_STATE_LOGO  = 1<<3,
	MLK_STATE_NUM_LOCK  = 1<<4,
	MLK_STATE_CAPS_LOCK = 1<<5,

	MLK_STATE_MASK_MODS = MLK_STATE_SHIFT | MLK_STATE_CTRL | MLK_STATE_ALT | MLK_STATE_LOGO
};

enum MLK_BTT
{
	MLK_BTT_NONE,
	MLK_BTT_LEFT,
	MLK_BTT_RIGHT,
	MLK_BTT_MIDDLE,
	MLK_BTT_SCR_UP,
	MLK_BTT_SCR_DOWN,
	MLK_BTT_SCR_LEFT,
	MLK_BTT_SCR_RIGHT
};

enum MLK_TRSYS
{
	MLK_TRSYS_OK = 1,
	MLK_TRSYS_CANCEL,
	MLK_TRSYS_YES,
	MLK_TRSYS_NO,
	MLK_TRSYS_SAVE,
	MLK_TRSYS_NOTSAVE,
	MLK_TRSYS_ABORT,
	MLK_TRSYS_NOTSHOW_THIS_MES,
	MLK_TRSYS_SELECT_COLOR,
	MLK_TRSYS_OPEN_FILE,
	MLK_TRSYS_SAVE_FILE,
	MLK_TRSYS_SELECT_DIRECTORY,
	MLK_TRSYS_OPEN,
	MLK_TRSYS_FILENAME,
	MLK_TRSYS_FILESIZE,
	MLK_TRSYS_FILEMODIFY,
	MLK_TRSYS_HOME_DIRECTORY,
	MLK_TRSYS_SHOW_HIDDEN_FILES,
	MLK_TRSYS_MES_OVERWRITE_FILE,
	MLK_TRSYS_MES_FILENAME_INVALID,

	MLK_TRSYS_SELECT_FONT,
	MLK_TRSYS_FONT_STYLE,
	MLK_TRSYS_FONT_ITALIC,
	MLK_TRSYS_FONT_SIZE,
	MLK_TRSYS_FONT_FILE,
	MLK_TRSYS_FONT_DETAIL,
	MLK_TRSYS_FONT_PREVIEW_TEXT
};

/* function */

#ifdef __cplusplus
extern "C" {
#endif

int mGuiInit(int argc,char **argv,int *argtop);
int mGuiInitBackend(void);
void mGuiEnd(void);

void mGuiSetWMClass(const char *name,const char *classname);
void mGuiSetEnablePenTablet(void);
void mGuiSetBlockUserAction(mlkbool on);

mFontSystem *mGuiGetFontSystem(void);
mFont *mGuiGetDefaultFont(void);

void mGuiQuit(void);
void mGuiRun(void);
void mGuiRunModal(mWindow *modal);
void mGuiRunPopup(mPopup *popup,mWidget *send);
mWindow *mGuiGetCurrentModal(void);

void mGuiThreadLock(void);
void mGuiThreadUnlock(void);
void mGuiThreadWakeup(void);

void mGuiSetPath_data_exe(const char *path);
void mGuiSetPath_config_home(const char *path);

const char *mGuiGetPath_data_text(void);
const char *mGuiGetPath_config_text(void);

void mGuiGetPath_data(mStr *str,const char *path);
void mGuiGetPath_config(mStr *str,const char *path);
char *mGuiGetPath_data_sp(const char *path);

mlkerr mGuiCreateConfigDir(const char *subpath);
mlkbool mGuiCopyFile_dataToConfig(const char *srcpath,const char *dstpath);

void mGuiWriteIni_system(void *fp);
void mGuiReadIni_system(mIniRead *ini);

void mGuiSetTranslationEmbed(const void *buf);
void mGuiLoadTranslation(const void *defbuf,const char *lang,const char *path);
void mGuiTransSetGroup(uint16_t id);
void mGuiTransSaveGroup(void);
void mGuiTransRestoreGroup(void);
const char *mGuiTransGetText(uint16_t id);
const char *mGuiTransGetTextRaw(uint16_t id);
const char *mGuiTransGetTextDefault(uint16_t id);
const char *mGuiTransGetText2(uint16_t groupid,uint16_t strid);
const char *mGuiTransGetText2Raw(uint16_t groupid,uint16_t strid);

void mGuiCalcHintSize(void);
mlkbool mGuiRenderWindows(void);
void mGuiDrawWidgets(void);
void mGuiUpdateAllWindows(void);

void mGuiGetDefaultFontInfo(mFontInfo *info);
const char *mGuiGetIconTheme(void);

void mGuiFileDialog_showHiddenFiles(void);

#ifdef __cplusplus
}
#endif

#endif
