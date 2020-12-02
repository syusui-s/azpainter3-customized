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

#ifndef MLIB_LOADIMAGE_H
#define MLIB_LOADIMAGE_H

typedef struct _mIOFileBuf mIOFileBuf;
typedef struct _mLoadImage mLoadImage;
typedef mBool (*mLoadImageFunc)(mLoadImage *);

#define M_LOADIMAGE(p)   ((mLoadImage *)(p))


/*---- enum ----*/

enum MLOADIMAGE_FORMAT
{
	MLOADIMAGE_FORMAT_RAW,
	MLOADIMAGE_FORMAT_RGB,
	MLOADIMAGE_FORMAT_RGBA
};

enum MLOADIMAGE_FLAGS
{
	MLOADIMAGE_FLAGS_ALLOW_16BIT = 1<<0,
	MLOADIMAGE_FLAGS_LEAVE_TRANSPARENT = 1<<1
};

enum MLOADIMAGE_SOURCE_TYPE
{
	MLOADIMAGE_SOURCE_TYPE_PATH,
	MLOADIMAGE_SOURCE_TYPE_BUF,
	MLOADIMAGE_SOURCE_TYPE_STDIO
};

enum MLOADIMAGE_RESOLITION_UNIT
{
	MLOADIMAGE_RESOLITION_UNIT_NONE,
	MLOADIMAGE_RESOLITION_UNIT_UNKNOWN,
	MLOADIMAGE_RESOLITION_UNIT_ASPECT,
	MLOADIMAGE_RESOLITION_UNIT_DPI,
	MLOADIMAGE_RESOLITION_UNIT_DPCM,
	MLOADIMAGE_RESOLITION_UNIT_DPM
};

enum MLOADIMAGE_COLTYPE
{
	MLOADIMAGE_COLTYPE_PALETTE,
	MLOADIMAGE_COLTYPE_GRAY,
	MLOADIMAGE_COLTYPE_GRAY_ALPHA,
	MLOADIMAGE_COLTYPE_RGB,
	MLOADIMAGE_COLTYPE_RGBA
};

enum MLOADIMAGE_CHECKFORMAT_FLAGS
{
	MLOADIMAGE_CHECKFORMAT_F_BMP  = 1<<0,
	MLOADIMAGE_CHECKFORMAT_F_PNG  = 1<<1,
	MLOADIMAGE_CHECKFORMAT_F_JPEG = 1<<2,
	MLOADIMAGE_CHECKFORMAT_F_GIF  = 1<<3,

	MLOADIMAGE_CHECKFORMAT_F_ALL = 0xff
};


/*---- error ----*/

enum MLOADIMAGE_ERR
{
	MLOADIMAGE_ERR_OK,
	MLOADIMAGE_ERR_OPENFILE,
	MLOADIMAGE_ERR_CORRUPT,
	MLOADIMAGE_ERR_UNSUPPORTED,
	MLOADIMAGE_ERR_INVALID_VALUE,
	MLOADIMAGE_ERR_ALLOC,
	MLOADIMAGE_ERR_DECODE,
	MLOADIMAGE_ERR_LARGE_SIZE,
	MLOADIMAGE_ERR_NONE_IMAGE,

	MLOADIMAGE_ERR_MAX
};


/*---- struct ----*/

typedef struct _mLoadImageSource
{
	int type;
	uint32_t bufsize;
	union
	{
		const char *filename;
		void *fp;
		const void *buf;
	};
}mLoadImageSource;

typedef struct _mLoadImageInfo
{
	int width,
		height,
		palette_num,
		raw_pitch,
		resolution_unit,
		resolution_horz,
		resolution_vert,
		transparent_col;	//-1=none
	uint8_t bottomup,
		sample_bits,		//8,16
		raw_sample_bits,	//8,16
		raw_bits,			//(for 8bit) 0,1,2,4,8,16,24,32
		raw_coltype;
	uint8_t *palette_buf;	//R-G-B-A 4byte x palette_num
}mLoadImageInfo;

struct _mLoadImage
{
	int format,
		max_width,		//0=disable
		max_height;
	uint32_t flags;
	mLoadImageSource src;
	mLoadImageInfo info;
	char *message;		//error message
	void *param1;

	int (*getinfo)(mLoadImage *,mLoadImageInfo *);
	int (*getrow)(mLoadImage *,uint8_t *buf,int pitch);
};


/*---- function ----*/

#ifdef __cplusplus
extern "C" {
#endif

mLoadImage *mLoadImage_create(int size);
void mLoadImage_free(mLoadImage *p);

void mLoadImage_setMessage(mLoadImage *p,const char *mes);
void mLoadImage_setMessage_errno(mLoadImage *p,int err);
const char *mLoadImage_getErrMessage(int err);

mIOFileBuf *mLoadImage_openIOFileBuf(mLoadImageSource *src);
int mLoadImage_getPitch(mLoadImage *p);
mBool mLoadImage_getResolution_dpi(mLoadImage *p,int *phorz,int *pvert);

mBool mLoadImage_checkFormat(mLoadImageSource *src,mLoadImageFunc *pfunc,uint32_t flags);

mBool mLoadImageBMP(mLoadImage *param);
mBool mLoadImagePNG(mLoadImage *param);
mBool mLoadImageGIF(mLoadImage *param);
mBool mLoadImageJPEG(mLoadImage *param);

#ifdef __cplusplus
}
#endif

#endif
