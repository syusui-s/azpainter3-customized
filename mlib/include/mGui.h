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

#ifndef MLIB_GUI_H
#define MLIB_GUI_H

#ifdef __cplusplus
extern "C" {
#endif

int mAppInit(int *argc,char **argv);
void mAppEnd(void);
void mAppQuit(void);

void mAppSetClassName(const char *appname,const char *classname);
mBool mAppInitPenTablet(void);

void mAppSync(void);
void mAppWakeUpEvent(void);
void mAppMutexLock(void);
void mAppMutexUnlock(void);
void mAppBlockUserAction(mBool on);

void mAppRun(void);
void mAppRunModal(mWindow *modal);
void mAppRunPopup(mWindow *popup);

mWindow *mAppGetCurrentModalWindow(void);

char *mAppGetFilePath(const char *path);
void mAppSetDataPath(const char *path);
void mAppGetDataPath(mStr *str,const char *pathadd);
void mAppSetConfigPath(const char *path,mBool bHome);
void mAppGetConfigPath(mStr *str,const char *pathadd);
int mAppCreateConfigDir(const char *pathadd);
void mAppCopyFile_dataToConfig(const char *path);

mBool mAppSetDefaultFont(const char *format);
mBool mAppLoadThemeFile(const char *filename);

void mAppSetTranslationDefault(const void *defdat);
void mAppLoadTranslation(const void *defdat,const char *lang,const char *pathadd);

void mGuiCalcHintSize(void);
void mGuiDraw(void);
mBool mGuiUpdate(void);
void mGuiUpdateAllWindows(void);

void *mGetFreeTypeLib(void);

void mGetDesktopWorkBox(mBox *box);
void mGetDesktopBox(mBox *box);

uint32_t mKeyRawToCode(uint32_t key);
int mKeyCodeToName(uint32_t c,char *buf,int bufsize);
int mRawKeyCodeToName(int key,char *buf,int bufsize);

#ifdef __cplusplus
}
#endif

#endif
