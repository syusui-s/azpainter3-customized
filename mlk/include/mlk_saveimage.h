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

#ifndef MLK_SAVEIMAGE_H
#define MLK_SAVEIMAGE_H

typedef struct _mSaveImage mSaveImage;

typedef mlkerr (*mFuncSaveImage)(mSaveImage *p,void *opt);
typedef void (*mFuncSaveImageProgress)(mSaveImage *p,int percent);


#define MSAVEIMAGE_MAXSIZE_WEBP 16383
#define MSAVEIMAGE_MAXSIZE_PSD  30000


/*---- enum ----*/

enum MSAVEIMAGE_OPEN
{
	MSAVEIMAGE_OPEN_FILENAME,
	MSAVEIMAGE_OPEN_FP
};

enum MSAVEIMAGE_RESOUNIT
{
	MSAVEIMAGE_RESOUNIT_NONE,
	MSAVEIMAGE_RESOUNIT_ASPECT,
	MSAVEIMAGE_RESOUNIT_DPI,
	MSAVEIMAGE_RESOUNIT_DPM
};

enum MSAVEIMAGE_COLTYPE
{
	MSAVEIMAGE_COLTYPE_GRAY,
	MSAVEIMAGE_COLTYPE_PALETTE,
	MSAVEIMAGE_COLTYPE_RGB,
	MSAVEIMAGE_COLTYPE_CMYK
};


/*---- struct ----*/

typedef struct _mSaveImageOpen
{
	int type;
	union
	{
		const char *filename;
		void *fp;
	};
}mSaveImageOpen;

struct _mSaveImage
{
	mSaveImageOpen open;
	
	int coltype,
		width,
		height,
		bits_per_sample,
		samples_per_pixel,
		palette_num,
		reso_unit,
		reso_horz,
		reso_vert;

	uint8_t *palette_buf;

	void *param1,
		*param2;

	mFuncSaveImageProgress progress;
	mlkerr (*setrow)(mSaveImage *p,int y,uint8_t *buf,int line_bytes);
	mlkerr (*setrow_ch)(mSaveImage *p,int y,int ch,uint8_t *buf,int line_bytes);
};


/*---- option ----*/

/* PNG option */

enum MSAVEOPT_PNG_MASK
{
	MSAVEOPT_PNG_MASK_COMP_LEVEL = 1<<0,
	MSAVEOPT_PNG_MASK_TRANSPARENT = 1<<1
};

typedef struct _mSaveOptPNG
{
	uint32_t mask;
	int comp_level,
		transparent;
	uint16_t transR,transG,transB;
}mSaveOptPNG;

/* JPEG option */

enum MSAVEOPT_JPEG_MASK
{
	MSAVEOPT_JPEG_MASK_QUALITY = 1<<0,
	MSAVEOPT_JPEG_MASK_PROGRESSION = 1<<1,
	MSAVEOPT_JPEG_MASK_SAMPLING_FACTOR = 1<<2
};

typedef struct _mSaveOptJPEG
{
	uint32_t mask;
	int quality,
		progression,
		sampling_factor;
}mSaveOptJPEG;

/* GIF option */

enum MSAVEOPT_GIF_MASK
{
	MSAVEOPT_GIF_MASK_TRANSPARENT = 1<<0,
};

typedef struct _mSaveOptGIF
{
	uint32_t mask;
	int transparent;
}mSaveOptGIF;

/* WEBP option */

enum MSAVEOPT_WEBP_MASK
{
	MSAVEOPT_WEBP_MASK_LOSSY = 1<<0,
	MSAVEOPT_WEBP_MASK_LEVEL = 1<<1,
	MSAVEOPT_WEBP_MASK_QUALITY = 1<<2
};

typedef struct _mSaveOptWEBP
{
	uint32_t mask;
	int lossy,level;
	float quality;
}mSaveOptWEBP;

/* TIFF option */

enum MSAVEOPT_TIFF_MASK
{
	MSAVEOPT_TIFF_MASK_COMPRESSION = 1<<0,
	MSAVEOPT_TIFF_MASK_ICCPROFILE = 1<<1
};

enum MSAVEOPT_TIFF_COMPRESSION
{
	MSAVEOPT_TIFF_COMPRESSION_NONE = 1,
	MSAVEOPT_TIFF_COMPRESSION_CCITT_RLE = 2,
	MSAVEOPT_TIFF_COMPRESSION_LZW = 5,
	MSAVEOPT_TIFF_COMPRESSION_PACKBITS = 32773,
	MSAVEOPT_TIFF_COMPRESSION_DEFLATE = 32946
};

typedef struct _mSaveOptTIFF
{
	uint32_t mask;
	int compression;
	const void *profile_buf;
	uint32_t profile_size;
}mSaveOptTIFF;

/* PSD option */

enum MSAVEOPT_PSD_MASK
{
	MSAVEOPT_PSD_MASK_COMPRESSION = 1<<0,
	MSAVEOPT_PSD_MASK_ICCPROFILE = 1<<1
};

typedef struct _mSaveOptPSD
{
	uint32_t mask;
	int compression;
	const void *profile_buf;
	uint32_t profile_size;
}mSaveOptPSD;

/*-----*/

typedef union _mSaveImageOpt
{
	uint32_t mask;
	mSaveOptPNG png;
	mSaveOptJPEG jpg;
	mSaveOptGIF gif;
	mSaveOptTIFF tiff;
	mSaveOptWEBP webp;
	mSaveOptPSD psd;
}mSaveImageOpt;


/*---- function ----*/

#ifdef __cplusplus
extern "C" {
#endif

void mSaveImage_init(mSaveImage *p);

void *mSaveImage_openFile(mSaveImage *p);
void mSaveImage_closeFile(mSaveImage *p,void *fp);
mlkbool mSaveImage_getDPI(mSaveImage *p,int *horz,int *vert);
mlkbool mSaveImage_getDPM(mSaveImage *p,int *horz,int *vert);
uint8_t *mSaveImage_createPaletteRGB(mSaveImage *p);
mlkbool mSaveImage_createPalette_fromRGB8_array(uint8_t **ppbuf,int width,int height,uint8_t **dst_buf,int *dst_palnum);
void mSaveImage_convertImage_RGB8array_to_palette(uint8_t **ppbuf,int width,int height,uint8_t *palbuf,int palnum);

mlkerr mSaveImageBMP(mSaveImage *p,void *opt);
mlkerr mSaveImagePNG(mSaveImage *p,void *opt);
mlkerr mSaveImageGIF(mSaveImage *p,void *opt);
mlkerr mSaveImageJPEG(mSaveImage *p,void *opt);
mlkerr mSaveImageWEBP(mSaveImage *si,void *opt);
mlkerr mSaveImageTIFF(mSaveImage *si,void *opt);
mlkerr mSaveImageTGA(mSaveImage *si,void *opt);
mlkerr mSaveImagePSD(mSaveImage *si,void *opt);

#ifdef __cplusplus
}
#endif

#endif
