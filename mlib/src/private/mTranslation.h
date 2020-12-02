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

#ifndef MLIB_TRANSLATION_H
#define MLIB_TRANSLATION_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mTranslation
{
	uint8_t *buf,
		*curgroupItem;
	int groupNum,
		curgroupItemNum;
	uint32_t offsetItem,
		offsetText;
	mBool bLoadFile;
}mTranslation;


void mTranslationFree(mTranslation *p);
void mTranslationSetEmbed(mTranslation *p,const void *buf);

mBool mTranslationLoadFile(mTranslation *p,const char *filename,mBool localestr);
mBool mTranslationLoadFile_dir(mTranslation *p,const char *lang,const char *dirpath);

mBool mTranslationSetGroup(mTranslation *p,uint16_t id);
const char *mTranslationGetText(mTranslation *p,uint16_t id);
const char *mTranslationGetText2(mTranslation *p,uint16_t groupid,uint16_t itemid);

#ifdef __cplusplus
}
#endif

#endif
