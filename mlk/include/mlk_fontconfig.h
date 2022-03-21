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

#ifndef MLK_FONTCONFIG_H
#define MLK_FONTCONFIG_H

typedef struct _FcPattern * mFcPattern;

#ifdef __cplusplus
extern "C" {
#endif

mlkbool mFontConfig_init(void);
void mFontConfig_finish(void);

mFcPattern mFontConfig_matchFontInfo(const mFontInfo *info);
mFcPattern mFontConfig_matchFamily(const char *family,const char *style);

void mFCPattern_free(mFcPattern pat);
mlkbool mFCPattern_getFile(mFcPattern pat,char **file,int *index);

char *mFCPattern_getText(mFcPattern pat,const char *object);
mlkbool mFCPattern_getBool_def(mFcPattern pat,const char *object,mlkbool def);
int mFCPattern_getInt_def(mFcPattern pat,const char *object,int def);
double mFCPattern_getDouble_def(mFcPattern pat,const char *object,double def);
mlkbool mFCPattern_getDouble(mFcPattern pat,const char *object,double *dst);
mlkbool mFCPattern_getMatrix(mFcPattern pat,const char *object,double *matrix);

#ifdef __cplusplus
}
#endif

#endif
