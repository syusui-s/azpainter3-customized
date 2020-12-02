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

#ifndef MLIB_PSD_LOAD_H
#define MLIB_PSD_LOAD_H

#include "mPSDDef.h"

typedef struct
{
	int err,
		width,
		height,
		bits,
		img_channels,
		colmode,
		layernum;
}mPSDLoad;

typedef struct
{
	mBox box_img,
		box_mask;
	uint32_t blendmode;
	char *name;
	uint8_t opacity,
		group,
		flags,
		layermask_defcol,
		layermask_flags;
}mPSDLoadLayerInfo;

enum MPSDLOAD_LAYERINFO_FLAGS
{
	MPSDLOAD_LAYERINFO_F_HIDE = 1<<0
};

enum MPSDLOAD_LAYERGROUP
{
	MPSDLOAD_LAYERGROUP_NONE,
	MPSDLOAD_LAYERGROUP_EXPAND,
	MPSDLOAD_LAYERGROUP_FOLDED,
	MPSDLOAD_LAYERGROUP_END
};

enum MPSDLOAD_LAYERMASK_FLAGS
{
	MPSDLOAD_LAYERMASK_F_RELATIVE = 1<<0,
	MPSDLOAD_LAYERMASK_F_DISABLE  = 1<<1,
	MPSDLOAD_LAYERMASK_F_REVERSE  = 1<<2
};

enum MPSDLOAD_ERR
{
	MPSDLOAD_ERR_OK = 0,
	MPSDLOAD_ERR_ALLOC,
	MPSDLOAD_ERR_OPENFILE,
	MPSDLOAD_ERR_NOT_PSD,
	MPSDLOAD_ERR_CORRUPTED,
	MPSDLOAD_ERR_INVALID_VALUE,
	MPSDLOAD_ERR_UNSUPPORTED,

	MPSDLOAD_ERR_NUM
};


#ifdef __cplusplus
extern "C" {
#endif

void mPSDLoad_close(mPSDLoad *p);
mPSDLoad *mPSDLoad_new(void);

mBool mPSDLoad_openFile(mPSDLoad *p,const char *filename);

mBool mPSDLoad_isHaveLayer(mPSDLoad *p);
uint8_t *mPSDLoad_getLineImageBuf(mPSDLoad *p);
const char *mPSDLoad_getErrorMessage(int err);

mBool mPSDLoad_readResource_currentLayer(mPSDLoad *p,int *pno);
mBool mPSDLoad_readResource_resolution_dpi(mPSDLoad *p,int *horz,int *vert);

mBool mPSDLoad_beginLayer(mPSDLoad *p);
mBool mPSDLoad_getLayerInfo(mPSDLoad *p,int no,mPSDLoadLayerInfo *info);
void mPSDLoad_getLayerImageMaxSize(mPSDLoad *p,mSize *size);
mBool mPSDLoad_beginLayerImage(mPSDLoad *p,mPSDLoadLayerInfo *info,mBool skip_group);
int mPSDLoad_beginLayerImageChannel(mPSDLoad *p,uint16_t channel_id);
mBool mPSDLoad_readLayerImageChannelLine(mPSDLoad *p);
mBool mPSDLoad_readLayerImageChannelLine_mask(mPSDLoad *p);
void mPSDLoad_endLayerImage(mPSDLoad *p);
void mPSDLoad_endLayer(mPSDLoad *p);

mBool mPSDLoad_beginImage(mPSDLoad *p);
mBool mPSDLoad_beginImageChannel(mPSDLoad *p);
mBool mPSDLoad_readImageChannelLine(mPSDLoad *load);

#ifdef __cplusplus
}
#endif

#endif
