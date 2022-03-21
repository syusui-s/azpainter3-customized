/*$
 Copyright (C) 2013-2022 Azel.

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

#ifndef MLK_PSD_H
#define MLK_PSD_H

typedef struct _mPSDLoad mPSDLoad;
typedef struct _mPSDSave mPSDSave;

#define MPSD_IMAGE_MAXSIZE  30000

/*--- load ---*/

typedef struct _mPSDHeader
{
	int width,
		height,
		bits,
		colmode,
		img_channels,
		layer_num;
}mPSDHeader;

typedef struct _mPSDLayer
{
	int layerno,
		chnum;
	uint32_t blendmode,
		ch_flags;
	uint8_t opacity,
		clipping,
		flags,
		mask_defcol,
		mask_flags;
	mBox box_img,
		box_mask;
	const char *name;
	void *param;
}mPSDLayer;

typedef struct _mPSDImageInfo
{
	int chid,
		offx,offy,
		width,height,
		rowsize,
		rowsize_mask;
}mPSDImageInfo;

typedef struct _mPSDLayerInfoExItem
{
	mListItem i;
	uint8_t *buf;
	uint32_t key,size;
}mPSDLayerInfoExItem;

typedef struct
{
	uint32_t type,blendmode;
}mPSDInfoSelection;

/*-------------*/

enum MPSD_COLMODE
{
	MPSD_COLMODE_MONO = 0,
	MPSD_COLMODE_GRAYSCALE,
	MPSD_COLMODE_INDEX,
	MPSD_COLMODE_RGB,
	MPSD_COLMODE_CMYK,
	MPSD_COLMODE_MULTI_CHANNELS,
	MPSD_COLMODE_HALFTONE,
	MPSD_COLMODE_LAB
};

enum MPSD_BLENDMODE
{
	MPSD_BLENDMODE_NORMAL        = ('n'<<24)|('o'<<16)|('r'<<8)|'m',
	MPSD_BLENDMODE_DISSOLVE      = ('d'<<24)|('i'<<16)|('s'<<8)|'s',
	MPSD_BLENDMODE_DARKEN        = ('d'<<24)|('a'<<16)|('r'<<8)|'k',
	MPSD_BLENDMODE_MULTIPLY      = ('m'<<24)|('u'<<16)|('l'<<8)|' ',
	MPSD_BLENDMODE_BURN          = ('i'<<24)|('d'<<16)|('i'<<8)|'v',
	MPSD_BLENDMODE_LINEAR_BURN   = ('l'<<24)|('b'<<16)|('r'<<8)|'n',
	MPSD_BLENDMODE_DARKER_COL    = ('d'<<24)|('k'<<16)|('C'<<8)|'l',
	MPSD_BLENDMODE_LIGHTEN       = ('l'<<24)|('i'<<16)|('t'<<8)|'e',
	MPSD_BLENDMODE_SCREEN        = ('s'<<24)|('c'<<16)|('r'<<8)|'n',
	MPSD_BLENDMODE_DODGE         = ('d'<<24)|('i'<<16)|('v'<<8)|' ',
	MPSD_BLENDMODE_LINEAR_DODGE  = ('l'<<24)|('d'<<16)|('d'<<8)|'g',
	MPSD_BLENDMODE_LIGHTER_COL   = ('l'<<24)|('g'<<16)|('C'<<8)|'l',
	MPSD_BLENDMODE_OVERLAY       = ('o'<<24)|('v'<<16)|('e'<<8)|'r',
	MPSD_BLENDMODE_SOFT_LIGHT    = ('s'<<24)|('L'<<16)|('i'<<8)|'t',
	MPSD_BLENDMODE_HARD_LIGHT    = ('h'<<24)|('L'<<16)|('i'<<8)|'t',
	MPSD_BLENDMODE_VIVID_LIGHT   = ('v'<<24)|('L'<<16)|('i'<<8)|'t',
	MPSD_BLENDMODE_LINEAR_LIGHT  = ('l'<<24)|('L'<<16)|('i'<<8)|'t',
	MPSD_BLENDMODE_PIN_LIGHT     = ('p'<<24)|('L'<<16)|('i'<<8)|'t',
	MPSD_BLENDMODE_HARD_MIX      = ('h'<<24)|('M'<<16)|('i'<<8)|'x',
	MPSD_BLENDMODE_DIFFERENCE    = ('d'<<24)|('i'<<16)|('f'<<8)|'f',
	MPSD_BLENDMODE_EXCLUSION     = ('s'<<24)|('m'<<16)|('u'<<8)|'d',
	MPSD_BLENDMODE_SUBTRACT      = ('f'<<24)|('s'<<16)|('u'<<8)|'b',
	MPSD_BLENDMODE_DIVIDE        = ('f'<<24)|('d'<<16)|('i'<<8)|'v',
	MPSD_BLENDMODE_HUE           = ('h'<<24)|('u'<<16)|('e'<<8)|' ',
	MPSD_BLENDMODE_SATURATION    = ('s'<<24)|('a'<<16)|('t'<<8)|' ',
	MPSD_BLENDMODE_COLOR         = ('c'<<24)|('o'<<16)|('l'<<8)|'r',
	MPSD_BLENDMODE_LUMINOSITY    = ('l'<<24)|('u'<<16)|('m'<<8)|' '
};

enum MPSD_CHID
{
	MPSD_CHID_COLOR = 0,
	MPSD_CHID_RED = 0,
	MPSD_CHID_GREEN = 1,
	MPSD_CHID_BLUE = 2,
	MPSD_CHID_CYAN = 0,
	MPSD_CHID_MAGENTA = 1,
	MPSD_CHID_YELLOW = 2,
	MPSD_CHID_KEYPLATE = 3,
	MPSD_CHID_ALPHA = 0xffff,
	MPSD_CHID_MASK = 0xfffe
};

enum MPSD_RESID
{
	MPSD_RESID_RESOLUTION = 0x03ed,
	MPSD_RESID_CURRENT_LAYER = 0x400,
	MPSD_RESID_LAYER_GROUP = 0x402,
	MPSD_RESID_THUMBNAIL_PS5 = 0x40c,
	MPSD_RESID_ICC_PROFILE = 0x40f
};

enum MPSD_LAYER_FLAGS
{
	MPSD_LAYER_FLAGS_PROTECT_TP = 1<<0,
	MPSD_LAYER_FLAGS_HIDE = 1<<1,
	MPSD_LAYER_FLAGS_HAVE_BIT4 = 1<<3,
	MPSD_LAYER_FLAGS_NODISP = 1<<4
};

enum MPSD_LAYER_MASK_FLAGS
{
	MPSD_LAYER_MASK_FLAGS_RELATIVE = 1<<0,
	MPSD_LAYER_MASK_FLAGS_DISABLE  = 1<<1,
	MPSD_LAYER_MASK_FLAGS_REVERSE  = 1<<2
};

enum MPSD_LAYER_CHFLAGS
{
	MPSD_LAYER_CHFLAGS_COL0 = 1<<0,
	MPSD_LAYER_CHFLAGS_COL1 = 1<<1,
	MPSD_LAYER_CHFLAGS_COL2 = 1<<2,
	MPSD_LAYER_CHFLAGS_COL3 = 1<<3,
	MPSD_LAYER_CHFLAGS_ALPHA = 1<<16,
	MPSD_LAYER_CHFLAGS_MASK = 1<<17
};

enum MPSD_INFO_SELECTION_TYPE
{
	MPSD_INFO_SELECTION_TYPE_NORMAL = 0,
	MPSD_INFO_SELECTION_TYPE_FOLDER_OPEN,
	MPSD_INFO_SELECTION_TYPE_FOLDER_CLOSED,
	MPSD_INFO_SELECTION_TYPE_END_LAST
};


#ifdef __cplusplus
extern "C" {
#endif

/* load */

mPSDLoad *mPSDLoad_new(void);
void mPSDLoad_close(mPSDLoad *p);

mlkerr mPSDLoad_openFile(mPSDLoad *p,const char *filename);
mlkerr mPSDLoad_openFILEptr(mPSDLoad *p,void *fp);

mlkerr mPSDLoad_readHeader(mPSDLoad *p,mPSDHeader *dst);
mlkerr mPSDLoad_readResource(mPSDLoad *p);

mlkbool mPSDLoad_res_getResoDPI(mPSDLoad *p,int *horz,int *vert);
int mPSDLoad_res_getCurLayerNo(mPSDLoad *p);
uint8_t *mPSDLoad_res_getICCProfile(mPSDLoad *p,uint32_t *psize);

int mPSDLoad_getImageChRowSize(mPSDLoad *p);
int mPSDLoad_getImageChNum(mPSDLoad *p);

mlkerr mPSDLoad_startImage(mPSDLoad *p);
mlkerr mPSDLoad_setImageCh(mPSDLoad *p,int ch);
mlkerr mPSDLoad_readImageRowCh(mPSDLoad *p,uint8_t *buf);

void mPSDLoad_getLayerImageMaxSize(mPSDLoad *p,mSize *size);
int mPSDLoad_getLayerImageMaxRowSize(mPSDLoad *p);

mlkbool mPSDLoad_getLayerInfo(mPSDLoad *p,int no,mPSDLayer *info);
mlkerr mPSDLoad_getLayerInfoEx(mPSDLoad *p,int no,mPSDLayer *info,mList *list);
void mPSDLoad_setLayerInfo_param(mPSDLoad *p,int no,void *param);

mlkbool mPSDLoad_exinfo_getSelection(mPSDLoad *p,mList *list,mPSDInfoSelection *info);

mlkerr mPSDLoad_startLayer(mPSDLoad *p);
mlkerr mPSDLoad_setLayerImageCh_id(mPSDLoad *p,int layerno,int chid,mPSDImageInfo *info);
mlkerr mPSDLoad_setLayerImageCh_no(mPSDLoad *p,int layerno,int chno,mPSDImageInfo *info);
mlkerr mPSDLoad_readLayerImageRowCh(mPSDLoad *p,uint8_t *buf);
mlkerr mPSDLoad_readLayerMaskImageRow(mPSDLoad *p,uint8_t *buf);

/* save */

mPSDSave *mPSDSave_new(void);
void mPSDSave_close(mPSDSave *p);

void mPSDSave_setCompression_none(mPSDSave *p);

mlkerr mPSDSave_openFile(mPSDSave *p,const char *filename);
mlkerr mPSDSave_openFILEptr(mPSDSave *p,void *fp);

mlkerr mPSDSave_writeHeader(mPSDSave *p,mPSDHeader *hd);

mlkbool mPSDSave_res_setResoDPI(mPSDSave *p,int horz,int vert);
mlkbool mPSDSave_res_setCurLayerNo(mPSDSave *p,int no);
mlkbool mPSDSave_res_setICCProfile(mPSDSave *p,const void *buf,uint32_t size);

mlkerr mPSDSave_writeResource(mPSDSave *p);

int mPSDSave_getImageRowSize(mPSDSave *p);

mlkerr mPSDSave_startImage(mPSDSave *p);
mlkerr mPSDSave_writeImageRowCh(mPSDSave *p,uint8_t *buf);
mlkerr mPSDSave_endImage(mPSDSave *p);

void mPSDSave_getLayerImageMaxSize(mPSDSave *p,mSize *size);
mlkbool mPSDSave_getLayerInfo(mPSDSave *p,int layerno,mPSDLayer *info);

mlkerr mPSDSave_writeLayerNone(mPSDSave *p);

mlkerr mPSDSave_startLayer(mPSDSave *p,int layernum);
mlkerr mPSDSave_setLayerInfo(mPSDSave *p,mPSDLayer *info,mList *exlist);
mlkerr mPSDSave_writeLayerInfo(mPSDSave *p);
mlkerr mPSDSave_startLayerImage(mPSDSave *p);
mlkerr mPSDSave_writeLayerImage_empty(mPSDSave *p);
mlkerr mPSDSave_startLayerImageCh(mPSDSave *p,int chid);
mlkerr mPSDSave_writeLayerImageRowCh(mPSDSave *p,uint8_t *buf);
mlkerr mPSDSave_endLayerImageCh(mPSDSave *p);
void mPSDSave_endLayerImage(mPSDSave *p);
mlkerr mPSDSave_endLayer(mPSDSave *p);

void mPSDSave_exinfo_freeList(mList *list);
mlkbool mPSDSave_exinfo_addSection(mList *list,int type,int blendmode);


#ifdef __cplusplus
}
#endif

#endif
