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

#ifndef MLK_LOADIMAGE_H
#define MLK_LOADIMAGE_H

typedef struct _mIO mIO;
typedef struct _mImageConv mImageConv;

typedef struct _mLoadImageOpen mLoadImageOpen;
typedef struct _mLoadImageType mLoadImageType;
typedef struct _mLoadImage mLoadImage;

typedef mlkbool (*mFuncLoadImageCheck)(mLoadImageType *p,uint8_t *buf,int size);
typedef void (*mFuncLoadImageProgress)(mLoadImage *p,int percent);

#define MLOADIMAGE_CHECKFORMAT_SKIP ((mFuncLoadImageCheck)1)

#define MLOADIMAGE_FORMAT_TAG_BMP  0x424d5020
#define MLOADIMAGE_FORMAT_TAG_PNG  0x504e4720
#define MLOADIMAGE_FORMAT_TAG_JPEG 0x4a504548
#define MLOADIMAGE_FORMAT_TAG_GIF  0x47494620
#define MLOADIMAGE_FORMAT_TAG_WEBP 0x57454250
#define MLOADIMAGE_FORMAT_TAG_TIFF 0x54494646
#define MLOADIMAGE_FORMAT_TAG_TGA  0x54474120
#define MLOADIMAGE_FORMAT_TAG_PSD  0x50534420


/*---- enum ----*/

enum MLOADIMAGE_OPEN
{
	MLOADIMAGE_OPEN_FILENAME = 0,
	MLOADIMAGE_OPEN_FP,
	MLOADIMAGE_OPEN_BUF
};

enum MLOADIMAGE_COLTYPE
{
	MLOADIMAGE_COLTYPE_PALETTE = 0,
	MLOADIMAGE_COLTYPE_RGB,
	MLOADIMAGE_COLTYPE_RGBA,
	MLOADIMAGE_COLTYPE_GRAY,
	MLOADIMAGE_COLTYPE_GRAY_A,
	MLOADIMAGE_COLTYPE_CMYK
};

enum MLOADIMAGE_RESOUNIT
{
	MLOADIMAGE_RESOUNIT_NONE = 0,
	MLOADIMAGE_RESOUNIT_UNKNOWN,
	MLOADIMAGE_RESOUNIT_ASPECT,
	MLOADIMAGE_RESOUNIT_DPI,
	MLOADIMAGE_RESOUNIT_DPCM,
	MLOADIMAGE_RESOUNIT_DPM
};

enum MLOADIMAGE_FLAGS
{
	MLOADIMAGE_FLAGS_ALLOW_16BIT = 1<<0,
	MLOADIMAGE_FLAGS_ALLOW_CMYK = 1<<1,
	MLOADIMAGE_FLAGS_TRANSPARENT_TO_ALPHA = 1<<2,
	MLOADIMAGE_FLAGS_COPY_PALETTE = 1<<3
};

enum MLOADIMAGE_CONVERT_TYPE
{
	MLOADIMAGE_CONVERT_TYPE_RAW = 0,
	MLOADIMAGE_CONVERT_TYPE_RGB,
	MLOADIMAGE_CONVERT_TYPE_RGBA
};


/*---- struct ----*/

struct _mLoadImageType
{
	uint32_t format_tag;
	mlkerr (*open)(mLoadImage *);
	mlkerr (*getimage)(mLoadImage *);
	void (*close)(mLoadImage *);
};

typedef struct _mLoadImageTransparent
{
	uint16_t r,g,b,flag;
}mLoadImageTransparent;

struct _mLoadImageOpen
{
	int type;
	union
	{
		const char *filename;
		void *fp;
		const void *buf;
	};
	mlkfoff size;
};

struct _mLoadImage
{
	void *handle;
	char *errmessage;

	mLoadImageOpen open;
	mLoadImageTransparent trns;

	int32_t width,
		height,
		coltype,
		bits_per_sample,
		reso_unit,
		reso_horz,
		reso_vert,
		line_bytes,
		palette_num,
		convert_type;
	uint32_t flags;

	uint8_t **imgbuf,
		*palette_buf;

	mFuncLoadImageProgress progress;
	void *param;
};


/*---- function ----*/

#ifdef __cplusplus
extern "C" {
#endif

mIO *mLoadImage_openIO(mLoadImageOpen *p);
mlkerr mLoadImage_createHandle(mLoadImage *p,int size,int endian);
void mLoadImage_closeHandle(mLoadImage *p);
mlkerr mLoadImage_setPalette(mLoadImage *p,uint8_t *buf,int size,int palnum);
void mLoadImage_setImageConv(mLoadImage *p,mImageConv *dst);

void mLoadImage_init(mLoadImage *p);
mlkerr mLoadImage_checkFormat(mLoadImageType *dst,mLoadImageOpen *open,mFuncLoadImageCheck *checks,int headsize);
mlkbool mLoadImage_allocImage(mLoadImage *p,int line_bytes);
mlkbool mLoadImage_allocImageFromBuf(mLoadImage *p,void *buf,int line_bytes);
void mLoadImage_freeImage(mLoadImage *p);
int mLoadImage_getLineBytes(mLoadImage *p);
mlkbool mLoadImage_getDPI(mLoadImage *p,int *horz,int *vert);

mlkbool mLoadImage_checkBMP(mLoadImageType *p,uint8_t *buf,int size);
mlkbool mLoadImage_checkPNG(mLoadImageType *p,uint8_t *buf,int size);
mlkbool mLoadImage_checkJPEG(mLoadImageType *p,uint8_t *buf,int size);
mlkbool mLoadImage_checkGIF(mLoadImageType *p,uint8_t *buf,int size);
mlkbool mLoadImage_checkWEBP(mLoadImageType *p,uint8_t *buf,int size);
mlkbool mLoadImage_checkTIFF(mLoadImageType *p,uint8_t *buf,int size);
mlkbool mLoadImage_checkTGA(mLoadImageType *p,uint8_t *buf,int size);
mlkbool mLoadImage_checkPSD(mLoadImageType *p,uint8_t *buf,int size);

mlkbool mLoadImagePNG_getGamma(mLoadImage *p,uint32_t *dst);
uint8_t *mLoadImagePNG_getICCProfile(mLoadImage *p,uint32_t *psize);
uint8_t *mLoadImageTIFF_getICCProfile(mLoadImage *p,uint32_t *psize);
uint8_t *mLoadImagePSD_getICCProfile(mLoadImage *p,uint32_t *psize);

#ifdef __cplusplus
}
#endif

#endif
