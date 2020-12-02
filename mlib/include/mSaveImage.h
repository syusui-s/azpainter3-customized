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

#ifndef MLIB_SAVEIMAGE_H
#define MLIB_SAVEIMAGE_H

typedef struct _mSaveImage mSaveImage;
typedef mBool (*mSaveImageFunc)(mSaveImage *);

#define M_SAVEIMAGE(p)  ((mSaveImage *)(p))


/*---- enum ----*/

enum MSAVEIMAGE_OUTPUT_TYPE
{
	MSAVEIMAGE_OUTPUT_TYPE_PATH,
	MSAVEIMAGE_OUTPUT_TYPE_STDIO
};

enum MSAVEIMAGE_RESOLITION_UNIT
{
	MSAVEIMAGE_RESOLITION_UNIT_NONE,
	MSAVEIMAGE_RESOLITION_UNIT_ASPECT,
	MSAVEIMAGE_RESOLITION_UNIT_DPI,
	MSAVEIMAGE_RESOLITION_UNIT_DPM
};

enum MSAVEIMAGE_COLTYPE
{
	MSAVEIMAGE_COLTYPE_PALETTE,
	MSAVEIMAGE_COLTYPE_GRAY,
	MSAVEIMAGE_COLTYPE_GRAY_ALPHA,
	MSAVEIMAGE_COLTYPE_RGB,
	MSAVEIMAGE_COLTYPE_RGBA
};

enum MSAVEIMAGE_ERR
{
	MSAVEIMAGE_ERR_OK,
	MSAVEIMAGE_ERR_OPENFILE,
	MSAVEIMAGE_ERR_ALLOC,
	MSAVEIMAGE_ERR_FORMAT,

	MSAVEIMAGE_ERR_MAX
};


/*---- struct ----*/

typedef struct _mSaveImageOutput
{
	int type;
	union
	{
		const char *filename;
		void *fp;
	};
}mSaveImageOutput;

struct _mSaveImage
{
	mSaveImageOutput output;

	uint8_t *palette_buf;	//R-G-B-A
	char *message;
	void *param1,
		*internal;
	
	int send_raw_format,
		sample_bits,	//8,16
		coltype,
		bits,			//palette bits (1,2,4,8)
		width,
		height,
		palette_num,
		resolution_unit,
		resolution_horz,
		resolution_vert;

	int (*set_option)(mSaveImage *p);
	int (*send_row)(mSaveImage *p,uint8_t *buf,int pitch);
};


/*---- function ----*/

#ifdef __cplusplus
extern "C" {
#endif

mSaveImage *mSaveImage_create(int size);
void mSaveImage_free(mSaveImage *p);

void mSaveImage_setMessage(mSaveImage *p,const char *mes);
void mSaveImage_setMessage_errno(mSaveImage *p,int err);

mBool mSaveImage_getResolution_dpm(mSaveImage *p,int *horz,int *vert);
mBool mSaveImage_getResolution_dpi(mSaveImage *p,int *horz,int *vert);

void *mSaveImage_openFILE(mSaveImageOutput *output);
uint8_t *mSaveImage_convertPalette_RGB(mSaveImage *p);


mBool mSaveImageBMP(mSaveImage *p);
void mSaveImageBMP_setBottomUp(mSaveImage *p);

mBool mSaveImagePNG(mSaveImage *p);
void mSaveImagePNG_setCompressionLevel(mSaveImage *p,int level);
void mSaveImagePNG_setTransparent_8bit(mSaveImage *p,uint32_t col);

mBool mSaveImageJPEG(mSaveImage *param);
void mSaveImageJPEG_setQuality(mSaveImage *p,int quality);
void mSaveImageJPEG_setProgression(mSaveImage *p);
void mSaveImageJPEG_setSamplingFactor(mSaveImage *p,int factor);

mBool mSaveImageGIF(mSaveImage *param);
void mSaveImageGIF_setTransparent(mSaveImage *p,int index);

#ifdef __cplusplus
}
#endif

#endif
