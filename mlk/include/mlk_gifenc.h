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

#ifndef MLK_GIFENC_H
#define MLK_GIFENC_H

typedef struct _mGIFEnc mGIFEnc;

typedef struct _mGIFEncInfo
{
	uint16_t width,
		height,
		palette_num;
	uint8_t bits,
		bkgnd_index;
	uint8_t *palette_buf;  //R-G-B
}mGIFEncInfo;

typedef struct _mGIFEncImage
{
	uint16_t offx,
		offy,
		width,
		height;
	uint8_t *palette_buf;
	int palette_num;
}mGIFEncImage;


enum MGIFENC_BKGND
{
	MGIFENC_BKGND_NONE,
	MGIFENC_BKGND_KEEP,
	MGIFENC_BKGND_FILL,
	MGIFENC_BKGND_RESTORE
};


#ifdef __cplusplus
extern "C" {
#endif

mGIFEnc *mGIFEnc_new(mGIFEncInfo *info);
void mGIFEnc_close(mGIFEnc *p);

mlkbool mGIFEnc_openFile(mGIFEnc *p,const char *filename);
mlkbool mGIFEnc_openFILEptr(mGIFEnc *p,void *fp);

int mGIFEnc_getBitsFromColnum(int num);

mlkbool mGIFEnc_writeHeader(mGIFEnc *p);
mlkbool mGIFEnc_writeGrpCtrl(mGIFEnc *p,int transparent,int bkgnd_type,uint16_t time,mlkbool user_input);
mlkbool mGIFEnc_writeLoop(mGIFEnc *p,uint16_t loopcnt);
mlkbool mGIFEnc_writeEnd(mGIFEnc *p);

mlkbool mGIFEnc_startImage(mGIFEnc *p,mGIFEncImage *img);
mlkbool mGIFEnc_writeImage(mGIFEnc *p,uint8_t *buf,int size);
mlkbool mGIFEnc_endImage(mGIFEnc *p);

#ifdef __cplusplus
}
#endif

#endif
