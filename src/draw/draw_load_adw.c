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

/*****************************************
 * AppDraw
 *
 * ADW 読み込み (ver1,ver2)
 *****************************************/

#include <stdio.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_popup_progress.h"
#include "mlk_zlib.h"
#include "mlk_stdio.h"
#include "mlk_unicode.h"
#include "mlk_util.h"

#include "def_draw.h"

#include "layerlist.h"
#include "layeritem.h"
#include "tileimage.h"
#include "imagecanvas.h"

#include "draw_main.h"


//------------------

typedef struct
{
	mPopupProgress *prog;
	
	mZlib *zlib;
}_load_adw;

//------------------


/* ver 2 読み込み (LE)
 *
 * Alpha-8bit 64x64 タイルイメージ */

static mlkerr _load_ver2(_load_adw *p,FILE *fp)
{
	int i;
	uint32_t size,tilenum,col;
	uint16_t wsize,imgw,imgh,dpi,layernum,layersel,layerinfosize,name[25],tx,ty;
	uint8_t opacity,flags,*tile;
	mlkerr ret;
	LayerItem *item;
	TileImage *img;
	mRect rc;

	//プレビューイメージをスキップ

	if(fseek(fp, 4, SEEK_CUR)		//幅、高さ
		|| mFILEreadLE32(fp, &size)	//イメージサイズ
		|| fseek(fp, size, SEEK_CUR))
		return MLKERR_DAMAGED;

	//メイン情報

	if(mFILEreadFormatLE(fp, "hhhhhhh",
		&wsize, &imgw, &imgh, &dpi, &layernum, &layersel, &layerinfosize))
		return MLKERR_DAMAGED;

	fseek(fp, wsize - 12, SEEK_CUR);

	//新規イメージ

	if(!drawImage_newCanvas_openFile(APPDRAW, imgw, imgh, 8, dpi))
		return MLKERR_ALLOC;

	//--------- レイヤ

	mPopupProgressThreadSetMax(p->prog, layernum * 6);

	for(item = NULL; layernum; layernum--)
	{
		//px 範囲とタイル数

		if(mFILEreadFormatLE(fp, "iiiii",
			&rc.x1, &rc.y1, &rc.x2, &rc.y2, &tilenum))
			return MLKERR_DAMAGED;

		//レイヤ情報

		if(mFILEreadOK(fp, name, 50)
			|| mFILEreadFormatLE(fp, "bib", &opacity, &col, &flags)
			|| fseek(fp, 3 + (layerinfosize - 79), SEEK_CUR))
			return MLKERR_DAMAGED;

		//レイヤ追加 (最後のレイヤの上に)

		item = LayerList_addLayer(APPDRAW->layerlist, item);
		if(!item) return MLKERR_ALLOC;

		//イメージ作成 (アルファのみ)

		img = TileImage_newFromRect_forFile(TILEIMAGE_COLTYPE_ALPHA, &rc);
		if(!img) return MLKERR_ALLOC;

		LayerItem_replaceImage(item, img, LAYERTYPE_ALPHA);

		//名前 (UTF-16 => UTF-8)

		for(i = 0; i < 24; i++)
			name[i] = mGetBufLE16(name + i);

		name[24] = 0;

		item->name = mUTF16toUTF8_alloc(name, -1, NULL);

		//

		item->opacity = opacity;

		if(!(flags & 1))
			item->flags &= ~LAYERITEM_F_VISIBLE;

		LayerItem_setLayerColor(item, col);

		//空イメージ

		if(tilenum == 0)
		{
			mPopupProgressThreadAddPos(p->prog, 6);
			continue;
		}

		//タイル
		
		mPopupProgressThreadSubStep_begin(p->prog, 6, tilenum);

		for(; tilenum; tilenum--)
		{
			//タイル位置、圧縮サイズ

			if(mFILEreadFormatLE(fp, "hhh", &tx, &ty, &wsize))
				return MLKERR_DAMAGED;

			if(wsize == 0 || wsize > 4096)
				return MLKERR_INVALID_VALUE;

			//タイル作成

			tile = TileImage_getTileAlloc_atpos(img, tx, ty, FALSE);
			if(!tile) return MLKERR_ALLOC;

			//読み込み

			if(wsize == 4096)
			{
				//無圧縮
				
				if(mFILEreadOK(fp, tile, 4096))
					return MLKERR_DAMAGED;
			}
			else
			{
				//zlib
				
				mZlibDecReset(p->zlib);

				ret = mZlibDecReadOnce(p->zlib, tile, 4096, wsize);
				if(ret) return ret;
			}

			//progress
			
			mPopupProgressThreadSubStep_inc(p->prog);
		}

		//念のため空タイルを解放

		TileImage_freeEmptyTiles(img);
	}

	//カレントレイヤ

	APPDRAW->curlayer = LayerList_getTopItem(APPDRAW->layerlist);

	return MLKERR_OK;
}

/* ver 1 読み込み (LE)
 *
 * Alpha-8bit 幅x高さのベタイメージ */

static mlkerr _load_ver1(_load_adw *p,FILE *fp)
{
	uint32_t size,col;
	uint16_t imgw,imgh,layernum,layercnt,layersel;
	uint8_t comp,opacity,flags;
	int pitch,ix,iy;
	mlkerr ret;
	char name[32];
	LayerItem *item;
	TileImage *img;
	uint8_t *buf,*ps,pixcol[4] = {0,0,0,0};

	//メイン情報

	if(mFILEreadFormatLE(fp, "hhhhhb",
		&imgw, &imgh, &layernum, &layercnt, &layersel, &comp))
		return MLKERR_DAMAGED;

	if(comp != 0) return MLKERR_INVALID_VALUE;

	//プレビューイメージをスキップ

	if(mFILEreadLE32(fp, &size)
		|| fseek(fp, size, SEEK_CUR))
		return MLKERR_DAMAGED;

	//新規イメージ

	if(!drawImage_newCanvas_openFile(APPDRAW, imgw, imgh, 8, -1))
		return MLKERR_ALLOC;

	//Y1行サイズ

	buf = *(APPDRAW->imgcanvas->ppbuf);
	pitch = (imgw + 3) & (~3);

	//------- レイヤ

	mPopupProgressThreadSetMax(p->prog, layernum * 6);

	for(item = NULL; layernum; layernum--)
	{
		//レイヤ情報

		if(mFILEreadOK(fp, name, 32)
			|| mFILEreadFormatLE(fp, "bbii", &opacity, &flags, &col, &size))
			return MLKERR_DAMAGED;

		//レイヤ追加
		// :下層レイヤから順のため、最後のレイヤの上に追加

		item = LayerList_addLayer(APPDRAW->layerlist, item);
		if(!item) return MLKERR_ALLOC;

		//イメージ作成 (アルファのみ)

		img = TileImage_new(TILEIMAGE_COLTYPE_ALPHA, imgw, imgh);
		if(!img) return MLKERR_ALLOC;

		LayerItem_replaceImage(item, img, LAYERTYPE_ALPHA);

		//名前 (Shift-JIS)

		name[31] = 0;
		LayerList_setItemName_shiftjis(APPDRAW->layerlist, item, name);

		//

		item->opacity = opacity;

		if(!(flags & 1))
			item->flags &= ~LAYERITEM_F_VISIBLE;

		LayerItem_setLayerColor(item, col);

		//イメージ読み込み

		mZlibDecReset(p->zlib);
		mZlibDecSetSize(p->zlib, size);

		mPopupProgressThreadSubStep_begin(p->prog, 6, imgh);

		for(iy = 0; iy < imgh; iy++)
		{
			ret = mZlibDecRead(p->zlib, buf, pitch);
			if(ret) return ret;

			ps = buf;

			for(ix = 0; ix < imgw; ix++, ps++)
			{
				if(*ps)
				{
					pixcol[3] = *ps;

					TileImage_setPixel_new(img, ix, iy, pixcol);
				}
			}

			mPopupProgressThreadSubStep_inc(p->prog);
		}

		ret = mZlibDecFinish(p->zlib);
		if(ret) return ret;
	}

	//カレントレイヤ

	APPDRAW->curlayer = LayerList_getTopItem(APPDRAW->layerlist);

	return MLKERR_OK;
}


//==============================


/* 読み込みメイン */

static mlkerr _load_main(_load_adw *p,FILE *fp)
{
	uint8_t ver;
	mlkerr ret;

	//ヘッダ

	if(mFILEreadStr_compare(fp, "AZDWDAT")
		|| mFILEreadByte(fp, &ver)
		|| ver >= 2)
		return MLKERR_UNSUPPORTED;

	//zlib 初期化

	p->zlib = mZlibDecNew(16 * 1024, MZLIB_WINDOWBITS_ZLIB);
	if(!p->zlib) return MLKERR_ALLOC;

	mZlibSetIO_stdio(p->zlib, fp);

	//読み込み

	if(ver == 0)
		ret = _load_ver1(p, fp);
	else
		ret = _load_ver2(p, fp);

	//解放

	mZlibFree(p->zlib);

	return ret;
}

/** ADW 読み込み */

mlkerr drawFile_load_adw(const char *filename,mPopupProgress *prog)
{
	_load_adw dat;
	FILE *fp;
	mlkerr ret;

	mMemset0(&dat, sizeof(_load_adw));
	dat.prog = prog;

	fp = mFILEopen(filename, "rb");
	if(!fp) return MLKERR_OPEN;

	ret = _load_main(&dat, fp);

	fclose(fp);

	return ret;
}

