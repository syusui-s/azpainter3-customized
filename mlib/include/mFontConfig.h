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

#ifndef MLIB_FONTCONFIG_H
#define MLIB_FONTCONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _FcPattern mFcPattern;
typedef struct _mFontInfo mFontInfo;


mBool mFontConfigInit(void);
void mFontConfigEnd(void);

void mFontConfigPatternFree(mFcPattern *pat);
mFcPattern *mFontConfigMatch(mFontInfo *conf);

int mFontConfigGetFileInfo(mFcPattern *pat,char **file,int *index);

char *mFontConfigGetText(mFcPattern *pat,const char *object);
mBool mFontConfigGetBool(mFcPattern *pat,const char *object,mBool def);
int mFontConfigGetInt(mFcPattern *pat,const char *object,int def);
double mFontConfigGetDouble(mFcPattern *pat,const char *object,double def);
mBool mFontConfigGetDouble2(mFcPattern *pat,const char *object,double *dst);
mBool mFontConfigGetMatrix(mFcPattern *pat,const char *object,double *matrix);

#ifdef __cplusplus
}
#endif

#endif
