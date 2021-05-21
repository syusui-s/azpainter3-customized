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

/********************************
 * APD v4
 ********************************/

typedef struct _apd4save apd4save;
typedef struct _apd4load apd4load;
typedef struct _LayerItem LayerItem;

typedef struct
{
	int width,height,dpi,bits,coltype,layernum;
	uint32_t bkgndcol;
}apd4info;

#define APD4_CHUNK_ID_THUMBNAIL  MLK_MAKE32_4('t','h','u','m')

/* load */

mlkerr apd4load_open(apd4load **ppdst,const char *filename,mPopupProgress *prog);
void apd4load_close(apd4load *p);

void apd4load_getInfo(apd4load *p,apd4info *info);

mlkerr apd4load_nextChunk(apd4load *p,uint32_t *pid,uint32_t *psize);
mlkerr apd4load_skipChunk(apd4load *p);
mlkerr apd4load_readChunk_thumbnail(apd4load *p,mSize *psize);
mlkerr apd4load_readChunk_thumbnail_image(apd4load *p,uint8_t *dstbuf,int pxnum);

mlkerr apd4load_readLayers(apd4load *p);
mlkerr apd4load_readLayer_append(apd4load *p,LayerItem **ppitem);

/* save */

mlkerr apd4save_saveSingleLayer(LayerItem *item,const char *filename,mPopupProgress *prog);

mlkerr apd4save_open(apd4save **ppdst,const char *filename,mPopupProgress *prog);
void apd4save_close(apd4save *p);

mlkerr apd4save_writeHeadInfo(apd4save *p,int layernum);

mlkerr apd4save_writeChunk_thumbnail(apd4save *p,uint8_t **ppbuf,int width,int height);
mlkerr apd4save_writeChunk_picture(apd4save *p,uint8_t **ppbuf,int width,int height,int stepnum);
mlkerr apd4save_writeChunk_end(apd4save *p);

mlkerr apd4save_writeLayer(apd4save *p,LayerItem *pi,mlkbool parent_root,int stepnum);
mlkerr apd4save_writeLayer_end(apd4save *p);

