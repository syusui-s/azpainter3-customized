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

#ifndef MLIB_PSD_SAVE_H
#define MLIB_PSD_SAVE_H

#include "mPSDDef.h"

typedef struct _mPSDSave mPSDSave;

typedef struct
{
	int width,
		height,
		img_channels,
		bits,		//1,8,16
		colmode;
}mPSDSaveInfo;

typedef struct
{
	int top,left,bottom,right,
		channels;
	uint32_t blendmode;
	const char *name;
	uint8_t opacity,
		hide;
}mPSDSaveLayerInfo;


#ifdef __cplusplus
extern "C" {
#endif

mPSDSave *mPSDSave_openFile(const char *filename,mPSDSaveInfo *info,mBool packbits);
void mPSDSave_close(mPSDSave *p);

uint8_t *mPSDSave_getLineImageBuf(mPSDSave *p);

void mPSDSave_beginResource(mPSDSave *p);
void mPSDSave_writeResource_resolution_dpi(mPSDSave *p,int dpi_horz,int dpi_vert);
void mPSDSave_writeResource_currentLayer(mPSDSave *p,int no);
void mPSDSave_endResource(mPSDSave *p);

void mPSDSave_writeLayerNone(mPSDSave *p);

mBool mPSDSave_beginLayer(mPSDSave *p,int num);
void mPSDSave_writeLayerInfo(mPSDSave *p,mPSDSaveLayerInfo *info);
mBool mPSDSave_endLayerInfo(mPSDSave *p);
mBool mPSDSave_beginLayerImage(mPSDSave *p,mBox *box);
void mPSDSave_beginLayerImageChannel(mPSDSave *p,int channel_id);
void mPSDSave_writeLayerImageChannelLine(mPSDSave *p);
void mPSDSave_endLayerImageChannel(mPSDSave *p);
void mPSDSave_endLayerImage(mPSDSave *p);
void mPSDSave_endLayer(mPSDSave *p);

int mPSDSave_getLayerImageMaxWidth(mPSDSave *p);

mBool mPSDSave_beginImage(mPSDSave *p);
void mPSDSave_beginImageChannel(mPSDSave *p);
void mPSDSave_writeImageChannelLine(mPSDSave *p);
void mPSDSave_endImageChannel(mPSDSave *p);

#ifdef __cplusplus
}
#endif

#endif
