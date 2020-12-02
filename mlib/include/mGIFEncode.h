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

#ifndef MLIB_GIFENCODE_H
#define MLIB_GIFENCODE_H

typedef struct _mGIFEncode mGIFEncode;

typedef struct _mGIFEncodeGlobalInfo
{
	uint16_t width,
		height,
		palette_num;
	uint8_t bits,			//1..8
		bkgnd_index;
	uint8_t *palette_buf;  //R-G-B
}mGIFEncodeGlobalInfo;

typedef struct _mGIFEncodeImage
{
	uint16_t offsetx,
		offsety,
		width,
		height;
	uint8_t *palette_buf;
	int palette_num;
}mGIFEncodeImage;


enum MGIFENCODE_BKGNDTYPE
{
	MGIFENCODE_BKGNDTYPE_NONE,
	MGIFENCODE_BKGNDTYPE_KEEP,
	MGIFENCODE_BKGNDTYPE_FILL_BKGND,
	MGIFENCODE_BKGNDTYPE_RESTORE
};


/*---- function -----*/

#ifdef __cplusplus
extern "C" {
#endif

mGIFEncode *mGIFEncode_create(mGIFEncodeGlobalInfo *info);
void mGIFEncode_close(mGIFEncode *p);

mBool mGIFEncode_open_filename(mGIFEncode *p,const char *filename);
mBool mGIFEncode_open_stdio(mGIFEncode *p,void *fp);

int mGIFEncode_colnumToBits(int num);

void mGIFEncode_writeHeader(mGIFEncode *p);
void mGIFEncode_writeGrpCtrl(mGIFEncode *p,int transparent,int bkgnd_type,uint16_t time,mBool user_input);
void mGIFEncode_writeLoop(mGIFEncode *p,uint16_t loopcnt);
void mGIFEncode_writeEnd(mGIFEncode *p);

void mGIFEncode_startImage(mGIFEncode *p,mGIFEncodeImage *img);
void mGIFEncode_writeImage(mGIFEncode *p,uint8_t *buf,int size);
void mGIFEncode_endImage(mGIFEncode *p);

#ifdef __cplusplus
}
#endif

#endif
