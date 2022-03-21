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

/*****************************************
 * AppDraw: APD 読み込み (ver 1,2)
 *****************************************/

#include <stdio.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_popup_progress.h"
#include "mlk_zlib.h"
#include "mlk_stdio.h"
#include "mlk_util.h"

#include "def_draw.h"

#include "layerlist.h"
#include "layeritem.h"
#include "def_tileimage.h"
#include "tileimage.h"
#include "imagecanvas.h"
#include "blendcolor.h"

#include "draw_main.h"


//------------------

typedef struct
{
	mPopupProgress *prog;

	FILE *fp;
	mZlib *zlib;
	uint8_t *tmpbuf;
}_load_apd;

//------------------

//APDv1:合成モード変換テーブル (「色相」以降はなし)

static const uint8_t g_apd_v1_blendmode[] = {
	BLENDMODE_NORMAL, BLENDMODE_MUL, BLENDMODE_LUMINOUS_ADD, BLENDMODE_SUB,
	BLENDMODE_SCREEN, BLENDMODE_OVERLAY, BLENDMODE_SOFT_LIGHT, BLENDMODE_HARD_LIGHT,
	BLENDMODE_DODGE, BLENDMODE_BURN, BLENDMODE_LINEAR_BURN, BLENDMODE_VIVID_LIGHT,
	BLENDMODE_LINEAR_LIGHT, BLENDMODE_PIN_LIGHT, BLENDMODE_DIFF,
	BLENDMODE_LIGHT, BLENDMODE_DARK
};

//------------------



//==============================
// ver 1 (LE)
//==============================
/*
 * AzPainter(Windows) で保存されたもの。
 * BGRA 8bit 幅x高さのベタイメージ (ボトムアップ)
 *
 * imgcanvas に RGBA8 としてセットして変換。 */


/* レイヤ読み込み */

static mlkerr _load_ver1_layers(_load_apd *p,int layernum,int layerinfo_seek)
{
	FILE *fp = p->fp;
	LayerItem *item;
	TileImage *img;
	uint8_t **ppdst,*pd,tmp8;
	uint32_t size;
	uint8_t blendmode,opacity,flags;
	int i,j,imgw,imgh,pitch;
	mlkerr ret;
	char name[32];

	imgw = APPDRAW->imgw;
	imgh = APPDRAW->imgh;
	pitch = imgw * 4;

	mPopupProgressThreadSetMax(p->prog, 5 * layernum);
	
	//

	for(item = NULL; layernum > 0; layernum--)
	{
		//レイヤ追加 (最下位のレイヤから順)

		item = LayerList_addLayer(APPDRAW->layerlist, item);
		if(!item) return MLKERR_ALLOC;

		//イメージ作成

		img = TileImage_new(TILEIMAGE_COLTYPE_RGBA, imgw, imgh);
		if(!img) return MLKERR_ALLOC;

		LayerItem_replaceImage(item, img, LAYERTYPE_RGBA);

		//レイヤ情報

		if(fread(name, 1, 32, fp) != 32
			|| mFILEreadByte(fp, &blendmode)
			|| mFILEreadByte(fp, &opacity)
			|| mFILEreadByte(fp, &flags)
			|| fseek(fp, layerinfo_seek, SEEK_CUR)
			|| mFILEreadLE32(fp, &size)) //イメージ圧縮サイズ
			return MLKERR_DAMAGED;

		//名前セット

		name[31] = 0;
		LayerList_setItemName_shiftjis(APPDRAW->layerlist, item, name);

		//

		item->opacity = opacity;

		if(!(flags & 1))
			item->flags &= ~LAYERITEM_F_VISIBLE;

		//合成モード

		if(blendmode >= 17)
			//"色相"以上は通常
			item->blendmode = BLENDMODE_NORMAL;
		else
			item->blendmode = g_apd_v1_blendmode[blendmode];

		//イメージのデコード (ボトムアップ)

		mZlibDecReset(p->zlib);
		mZlibDecSetSize(p->zlib, size);

		ppdst = APPDRAW->imgcanvas->ppbuf + imgh - 1;

		for(i = imgh; i; i--, ppdst--)
		{
			pd = *ppdst;
			
			ret = mZlibDecRead(p->zlib, pd, pitch);
			if(ret) return ret;

			//BGRA -> RGBA 変換

			for(j = imgw; j; j--, pd += 4)
			{
				tmp8 = pd[2];
				pd[2] = pd[0];
				pd[0] = tmp8;
			}
		}

		ret = mZlibDecFinish(p->zlib);
		if(ret) return ret;

		//変換

		TileImage_convertFromCanvas(img, APPDRAW->imgcanvas, p->prog, 5);
	}

	return MLKERR_OK;
}

/* ver 1 読み込み */

static mlkerr _load_ver1(_load_apd *p)
{
	FILE *fp = p->fp;
	uint32_t size,layerinfosize;
	uint16_t imgw,imgh,layernum,layercnt,layersel;
	mlkerr ret;

	//基本情報

	if(mFILEreadFormatLE(fp, "ihhhhh",
		&size, &imgw, &imgh, &layernum, &layercnt, &layersel)
		|| fseek(fp, size - 10, SEEK_CUR))
		return MLKERR_DAMAGED;
	
	//

	if(fseek(fp, 4, SEEK_CUR)	//プレビュー画像:幅,高さ
		|| mFILEreadLE32(fp, &size)	//圧縮サイズ
		|| fseek(fp, size, SEEK_CUR)
		|| mFILEreadLE32(fp, &layerinfosize)) //レイヤ情報一つのサイズ
		return MLKERR_DAMAGED;

	//新規イメージ

	if(!drawImage_newCanvas_openFile(APPDRAW, imgw, imgh, 8, -1))
		return MLKERR_ALLOC;

	//レイヤ

	ret = _load_ver1_layers(p, layernum, layerinfosize - 35);

	//カレントレイヤ

	APPDRAW->curlayer = LayerList_getItemAtIndex(APPDRAW->layerlist, layersel);

	return ret;
}


//==============================
// ver 2 (BigEndian)
//==============================
/*
  AzPainter(Linux) ver 1.x で保存されたもの。
  64x64 タイルイメージ。レイヤカラータイプあり。
*/


/* タイル読み込み */

static mlkerr _load_ver2_tile(_load_apd *p,FILE *fp,TileImage *img,uint32_t tilenum)
{
	int tilew;
	uint32_t pos,size,maxpos,tilesize;
	mlkerr ret;

	tilew = img->tilew;
	maxpos = img->tilew * img->tileh;
	tilesize = img->tilesize;

	mPopupProgressThreadSubStep_begin(p->prog, 6, tilenum);

	for(; tilenum; tilenum--)
	{
		//タイル位置、圧縮サイズ
		
		if(mFILEreadBE32(fp, &pos)
			|| mFILEreadBE32(fp, &size)
			|| pos >= maxpos
			|| size > tilesize)
			return MLKERR_DAMAGED;

		//読み込み

		if(size == tilesize)
		{
			//無圧縮
			
			if(mFILEreadOK(fp, p->tmpbuf, size))
				return MLKERR_DAMAGED;
		}
		else
		{
			//zlib
			
			mZlibDecReset(p->zlib);

			ret = mZlibDecReadOnce(p->zlib, p->tmpbuf, tilesize, size);
			if(ret) return ret;
		}

		//タイルセット

		if(!TileImage_setTile_fromSave(img, pos % tilew, pos / tilew, p->tmpbuf))
			return MLKERR_ALLOC;

		//

		mPopupProgressThreadSubStep_inc(p->prog);
	}

	return MLKERR_OK;
}

/* ver2: レイヤ読み込み */

static mlkerr _load_ver2_layers(_load_apd *p,FILE *fp)
{
	LayerItem *item;
	TileImage *img;
	char *name;
	TileImageInfo imginfo;
	uint32_t col,flags,tilenum;
	int32_t offx,offy;
	uint16_t layernum,parent,tilew,tileh;
	uint8_t lflags,coltype,opacity,blendmode,amask,itemflags;
	mlkerr ret;

	//レイヤ数

	if(mFILEreadBE16(fp, &layernum))
		return MLKERR_ALLOC;

	//タイルバッファ

	p->tmpbuf = (uint8_t *)mMalloc(64 * 64 * 2 * 4);
	if(!p->tmpbuf) return MLKERR_ALLOC;

	//

	mPopupProgressThreadSetMax(p->prog, layernum * 6);

	while(1)
	{
		//ツリー情報
		// parent = 親のレイヤ位置 (0xfffe で終了、0xffff でルート)
	
		if(mFILEreadBE16(fp, &parent) || parent == 0xfffe)
			break;

		//フラグ
		// 0bit: フォルダ
		// 1bit: カレントレイヤ
		
		if(mFILEreadByte(fp, &lflags))
			return MLKERR_DAMAGED;

		//レイヤ追加

		item = LayerList_addLayer_index(APPDRAW->layerlist, (parent == 0xffff)? -1: parent, -1);
		if(!item) return MLKERR_ALLOC;

		//カレントレイヤ

		if(lflags & 2)
			APPDRAW->curlayer = item;

		//名前

		if(mFILEreadStr_variable(fp, &name) < 0)
			return MLKERR_DAMAGED;

		item->name = name;

		//情報

		if(mFILEreadFormatBE(fp, "bbbbii",
			&coltype, &opacity, &blendmode, &amask, &col, &flags))
			return MLKERR_DAMAGED;

		if(coltype > 3) return MLKERR_INVALID_VALUE;

		item->opacity = opacity;
		item->alphamask = (amask > 3)? 0: amask;
		item->blendmode = (blendmode >= 17)? 0: blendmode; //"差の絶対値" まで同じ

		//フラグ

		itemflags = 0;
		
		if(flags & (1<<0)) itemflags |= LAYERITEM_F_VISIBLE;
		if(flags & (1<<1)) itemflags |= LAYERITEM_F_FOLDER_OPENED;
		if(flags & (1<<2)) itemflags |= LAYERITEM_F_LOCK;
		if(flags & (1<<3)) itemflags |= LAYERITEM_F_FILLREF;
		if(flags & (1<<5)) itemflags |= LAYERITEM_F_MASK_UNDER;
		if(flags & (1<<7)) itemflags |= LAYERITEM_F_CHECKED;

		item->flags = itemflags;

		//

		if(lflags & 1)
		{
			//フォルダ

			mPopupProgressThreadAddPos(p->prog, 6);
		}
		else
		{
			//----- イメージ

			//情報

			if(mFILEreadFormatBE(fp, "iihhi",
				&offx, &offy, &tilew, &tileh, &tilenum))
				return MLKERR_DAMAGED;

			//イメージ作成

			imginfo.offx = offx;
			imginfo.offy = offy;
			imginfo.tilew = tilew;
			imginfo.tileh = tileh;

			img = TileImage_newFromInfo(coltype, &imginfo);
			if(!img) return MLKERR_ALLOC;

			LayerItem_replaceImage(item, img, coltype);
			LayerItem_setLayerColor(item, col);

			//タイル

			if(tilenum == 0)
				//空イメージ
				mPopupProgressThreadAddPos(p->prog, 6);
			else
			{
				ret = _load_ver2_tile(p, fp, img, tilenum);
				if(ret) return ret;
			}
		}
	}

	//カレントレイヤがセットされていない場合

	if(!APPDRAW->curlayer)
		APPDRAW->curlayer = LayerList_getTopItem(APPDRAW->layerlist);

	return MLKERR_OK;
}

/* ver 2 読み込み */

static mlkerr _load_ver2(_load_apd *p,FILE *fp)
{
	uint32_t size;
	uint16_t info[3];
	uint8_t sig,fhave_info = FALSE;
	int i;

	while(mFILEreadByte(fp, &sig) == 0)
	{
		if(sig == 'L')
		{
			//レイヤ

			if(!fhave_info) return MLKERR_DAMAGED;

			return _load_ver2_layers(p, fp);
		}
		else
		{
			//------ サイズ + データ
		
			if(mFILEreadBE32(fp, &size))
				return MLKERR_DAMAGED;

			//イメージ情報
			
			if(sig == 'I' && !fhave_info)
			{
				//0:imgw, 1:imgh, 2:dpi

				for(i = 0; i < 3; i++)
				{
					if(mFILEreadBE16(fp, info + i))
						return MLKERR_DAMAGED;
				}

				//新規イメージ

				if(!drawImage_newCanvas_openFile(APPDRAW, info[0], info[1], 16, info[2]))
					return MLKERR_ALLOC;

				size -= 6;
				fhave_info = TRUE;
			}

			//次へ

			if(fseek(fp, size, SEEK_CUR))
				return MLKERR_DAMAGED;
		}
	}

	return MLKERR_OK;
}


//==============================
// 共通
//==============================


/* 読み込みメイン */

static mlkerr _load_main(_load_apd *p)
{
	FILE *fp = p->fp;
	uint8_t ver;
	mlkerr ret;
	
	//ヘッダ

	if(mFILEreadStr_compare(fp, "AZPDATA")
		|| mFILEreadByte(fp, &ver)
		|| ver >= 2)
		return MLKERR_UNSUPPORTED;

	//zlib 初期化

	p->zlib = mZlibDecNew(16 * 1024, MZLIB_WINDOWBITS_ZLIB);
	if(!p->zlib) return MLKERR_ALLOC;

	mZlibSetIO_stdio(p->zlib, fp);

	//読み込み

	if(ver == 0)
		ret = _load_ver1(p);
	else
		ret = _load_ver2(p, fp);

	//解放

	mZlibFree(p->zlib);
	mFree(p->tmpbuf);

	return ret;
}

/** APD v1/v2 読み込み */

mlkerr drawFile_load_apd_v1v2(const char *filename,mPopupProgress *prog)
{
	_load_apd dat;
	mlkerr ret;

	mMemset0(&dat, sizeof(_load_apd));
	dat.prog = prog;

	dat.fp = mFILEopen(filename, "rb");
	if(!dat.fp) return MLKERR_OPEN;

	ret = _load_main(&dat);

	fclose(dat.fp);

	return ret;
}

