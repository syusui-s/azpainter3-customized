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

#ifndef MLK_IMAGELIST_H
#define MLK_IMAGELIST_H

struct _mImageList
{
	uint8_t *buf;
	int w,h,
		pitch,
		eachw,
		num;
};

enum MIMAGELIST_PUT_FLAGS
{
	MIMAGELIST_PUTF_DISABLE = 1<<0
};


#ifdef __cplusplus
extern "C" {
#endif

void mImageListFree(mImageList *p);

mImageList *mImageListNew(int w,int h,int eachw);
mImageList *mImageListLoadPNG(const char *filename,int eachw);
mImageList *mImageListLoadPNG_buf(const uint8_t *buf,int bufsize,int eachw);
mImageList *mImageListCreate_1bit_mono(const uint8_t *buf,int w,int h,int eachw);
mImageList *mImageListCreate_1bit_textcol(const uint8_t *buf,int w,int h,int eachw);

mlkbool mImageListLoad_icontheme(mImageList *p,int pos,mIconTheme theme,const char *names,int iconsize);
mImageList *mImageListCreate_icontheme(mIconTheme theme,const char *names,int iconsize);

void mImageListClear(mImageList *p);
void mImageListSetTransparentColor(mImageList *p,mRgbCol col);
void mImageListReplaceTextColor_mono(mImageList *p);

void mImageListPutPixbuf(mImageList *p,mPixbuf *pixbuf,int dx,int dy,int index,uint32_t flags);
void mImageListSetOne_imagebuf(mImageList *p,int index,mImageBuf *src);

#ifdef __cplusplus
}
#endif

#endif
