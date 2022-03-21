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

#ifndef MLK_TRANSLATION_H
#define MLK_TRANSLATION_H

typedef struct _mTranslation mTranslation;

#ifdef __cplusplus
extern "C" {
#endif

void mTranslationFree(mTranslation *p);

mTranslation *mTranslationLoadDefault(const void *buf);

mTranslation *mTranslationLoadFile(const char *filename,mlkbool locale);
mTranslation *mTranslationLoadFile_lang(const char *path,const char *lang,const char *prefix);

mlkbool mTranslationSetGroup(mTranslation *p,uint16_t id);
const char *mTranslationGetText(mTranslation *p,uint16_t id);
const char *mTranslationGetText2(mTranslation *p,uint16_t groupid,uint16_t itemid);

#ifdef __cplusplus
}
#endif

#endif
