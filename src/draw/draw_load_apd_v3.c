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

/************************************
 * APD v3 読み込み
 ************************************/

#include <stdio.h>
#include <string.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_popup_progress.h"
#include "mlk_zlib.h"
#include "mlk_stdio.h"
#include "mlk_util.h"

#include "def_macro.h"
#include "def_draw.h"

#include "layerlist.h"
#include "layeritem.h"
#include "def_tileimage.h"
#include "tileimage.h"

#include "draw_main.h"

#include "pv_apd_format.h"


//----------------------

//レイヤ情報

typedef struct
{
	int32_t parent_no;	//親の番号 (-1 でルート)
	mRect rc;
	uint32_t blendid,
		flags;
	uint8_t coltype,
		channels,
		opacity,
		amask,
		r,g,b;
	char *name,
		*texpath;
}_layerinfo;

//apd

typedef struct
{
	FILE *fp;
	mZlib *zlib;
	mPopupProgress *prog;

	uint8_t *tilebuf;	//作業用タイルバッファ
	_layerinfo *layerinfo;	//レイヤ情報データ

	int32_t layernum,
		layer_curno,
		width,
		height,
		dpi;
	uint32_t thunk_size;	//チャンクのサイズ
	long thunk_toppos,		//チャンクの先頭位置
		filesize;			//ファイルの全体サイズ
	fpos_t findthunk_toppos;	//チャンク検索の先頭位置

}_loadv3;

#define _PROGRESS_STEP  10 //レイヤ一つの進捗ステップ数

//カラータイプ
enum
{
	APD3_COLTYPE_8BIT,
	APD3_COLTYPE_1BIT,
	APD3_COLTYPE_16BIT,
	APD3_COLTYPE_FOLDER = 255
};

//----------------------



//============================
// sub
//============================


/* 指定チャンクを探す
 *
 * error: TRUE で、チャンクサイズがファイルサイズを超えていればエラーにする
 * return: 0 で見つかった。-1 で見つからなかった */

static mlkerr _goto_thunk(_loadv3 *p,uint32_t thunk_id,mlkbool error)
{
	uint32_t id,size;

	fsetpos(p->fp, &p->findthunk_toppos);

	while(1)
	{
		//ID、サイズ
		
		if(mFILEreadBE32(p->fp, &id)
			|| mFILEreadBE32(p->fp, &size))
			break;
	
		if(id == thunk_id)
		{
			p->thunk_toppos = ftell(p->fp);
			p->thunk_size = size;

			//ファイルサイズを超えている
			
			if(error && p->thunk_toppos + p->thunk_size > p->filesize)
				return MLKERR_DAMAGED;

			return MLKERR_OK;
		}
		else if(id == _MAKE_ID('E','N','D',' '))
			break;

		//スキップ

		if(fseek(p->fp, size, SEEK_CUR))
			return MLKERR_DAMAGED;
	}

	return -1;
}

/* ヘッダの読み込み */

static mlkerr _read_header(_loadv3 *p,FILE *fp)
{
	uint8_t ver,unit;

	//ヘッダ

	if(mFILEreadStr_compare(fp, "AZPDATA")
		|| mFILEreadByte(fp, &ver)
		|| ver != 2)
		return MLKERR_UNSUPPORTED;

	//情報

	if(mFILEreadFormatBE(fp, "iibi",
		&p->width, &p->height, &unit, &p->dpi))
		return MLKERR_DAMAGED;
		
	if(p->width <= 0 || p->height <= 0)
		return MLKERR_INVALID_VALUE;

	//サイズ制限

	if(p->width > IMAGE_SIZE_MAX
		|| p->height > IMAGE_SIZE_MAX)
		return MLKERR_MAX_SIZE;

	//チャンク先頭位置

	fgetpos(fp, &p->findthunk_toppos);

	//dpi

	if(unit != 1 || p->dpi < 1)
		p->dpi = 96;

	return MLKERR_OK;
}

/* レイヤ情報を解放 */

static void _free_layerinfo(_loadv3 *p)
{
	_layerinfo *pl;
	int i;

	pl = p->layerinfo;

	if(pl)
	{
		for(i = p->layernum; i; i--, pl++)
		{
			mFree(pl->name);
			mFree(pl->texpath);
		}
	
		mFree(p->layerinfo);
		p->layerinfo = NULL;
	}
}


//==========================
// 読み込み処理
//==========================


/* 閉じる */

static void _loadv3_close(_loadv3 *p)
{
	if(p->fp) fclose(p->fp);

	mZlibFree(p->zlib);
	mFree(p->tilebuf);

	_free_layerinfo(p);
	
	mFree(p);
}

/* APDv3 ファイル開く */

static mlkerr _loadv3_open(_loadv3 **dst,const char *filename,mPopupProgress *prog)
{
	_loadv3 *p;
	FILE *fp;
	mlkerr ret;

	p = (_loadv3 *)mMalloc0(sizeof(_loadv3));
	if(!p) return MLKERR_ALLOC;

	p->prog = prog;

	//zlib

	p->zlib = mZlibDecNew(8192, 15);
	if(!p->zlib)
	{
		ret = MLKERR_ALLOC;
		goto ERR;
	}

	//作業用タイルバッファ

	p->tilebuf = (uint8_t *)mMalloc(64 * 64 * 8);
	if(!p->tilebuf)
	{
		ret = MLKERR_ALLOC;
		goto ERR;
	}

	//ファイル開く

	p->fp = fp = mFILEopen(filename, "rb");
	if(!fp)
	{
		ret = MLKERR_OPEN;
		goto ERR;
	}

	//ファイルサイズ
	
	fseek(fp, 0, SEEK_END);
	p->filesize = ftell(fp);
	rewind(fp);

	//

	mZlibSetIO_stdio(p->zlib, fp);

	//ヘッダ読み込み

	ret = _read_header(p, fp);
	if(ret) goto ERR;

	//

	*dst = p;

	return MLKERR_OK;

ERR:
	_loadv3_close(p);
	return ret;
}

/* レイヤヘッダ (LYHD) 読み込み */

static mlkerr _read_layer_header(_loadv3 *p)
{
	uint16_t num,cur;
	mlkerr ret;

	ret = _goto_thunk(p, _MAKE_ID('L','Y','H','D'), TRUE);
	if(ret) return ret;

	if(p->thunk_size < 4)
		return MLKERR_DAMAGED;

	mFILEreadBE16(p->fp, &num);
	mFILEreadBE16(p->fp, &cur);

	//レイヤ数が0
	
	if(num == 0) return MLKERR_INVALID_VALUE;

	//

	p->layernum = num;
	p->layer_curno = cur;

	//レイヤ情報バッファ

	p->layerinfo = (_layerinfo *)mMalloc0(sizeof(_layerinfo) * num);
	if(!p->layerinfo) return MLKERR_ALLOC;

	//進捗最大値

	mPopupProgressThreadSetMax(p->prog, num * _PROGRESS_STEP);

	return MLKERR_OK;
}

/* レイヤツリー読み込み */

static mlkerr _read_layer_tree(_loadv3 *p)
{
	_layerinfo *pinfo;
	mlkerr ret;
	int i;
	uint16_t no;

	ret = _goto_thunk(p, _MAKE_ID('L','Y','T','R'), TRUE);

	pinfo = p->layerinfo;

	if(ret == -1)
	{
		//チャンクがない場合、親はすべてルートとする

		for(i = p->layernum; i; i--, pinfo++)
			pinfo->parent_no = -1;
	}
	else if(ret)
		//エラー
		return ret;
	else
	{
		//先頭レイヤから順に、親のレイヤ番号 (2byte) が並んでいる。
		//0xffff でルート。

		if(p->thunk_size != p->layernum * 2)
			return MLKERR_DAMAGED;

		for(i = p->layernum; i; i--, pinfo++)
		{
			mFILEreadBE16(p->fp, &no);
			
			pinfo->parent_no = (no == 0xffff)? -1: no;
		}
	}

	return MLKERR_OK;
}

/* レイヤ情報の読み込みを開始 */

static mlkerr _begin_layerinfo(_loadv3 *p)
{
	uint16_t ver;
	mlkerr ret;

	ret = _goto_thunk(p, _MAKE_ID('L','Y','I','F'), TRUE);
	if(ret) return ret;

	if(p->thunk_size < 2) return MLKERR_DAMAGED;

	//バージョン

	mFILEreadBE16(p->fp, &ver);

	if(ver != 0) return MLKERR_UNSUPPORTED;

	return MLKERR_OK;
}

/* レイヤ情報を読み込み */

static mlkerr _read_layer_info(_loadv3 *p)
{
	FILE *fp = p->fp;
	_layerinfo *pd;
	int i,n,imgbits;
	uint16_t exsize;
	uint32_t exid;
	mlkerr ret;

	//----- レイヤ情報

	ret = _begin_layerinfo(p);
	if(ret) return ret;

	pd = p->layerinfo;
	imgbits = 8;

	for(i = p->layernum; i; i--, pd++)
	{
		//読み込み

		if(mFILEreadFormatBE(fp, "bbiiiibiibbbb",
			&pd->coltype, &pd->channels,
			&pd->rc.x1, &pd->rc.y1, &pd->rc.x2, &pd->rc.y2,
			&pd->opacity, &pd->blendid, &pd->flags, &pd->amask,
			&pd->r, &pd->g, &pd->b))
			return MLKERR_DAMAGED;

		//カラータイプ

		n = pd->coltype;

		if(n >= 3 && n != 255)
			return MLKERR_UNSUPPORTED;

		//16bit カラー

		if(pd->coltype == 2) imgbits = 16;

		//チャンネル数

		n = pd->channels;

		if(pd->coltype != 255
			&& n != 1 && n != 2 && n != 4)
			return MLKERR_UNSUPPORTED;

		//不透明度

		if(pd->opacity > 128)
			pd->opacity = 128;

		//拡張データ

		while(1)
		{
			//ID
			mFILEreadBE32(fp, &exid);
			if(exid == 0) break;

			//サイズ
			mFILEreadBE16(fp, &exsize);

			if(exid == _MAKE_ID('n','a','m','e'))
			{
				//名前

				if(exsize)
				{
					pd->name = (char *)mMalloc0(exsize + 1);
					
					fread(pd->name, 1, exsize, fp);
				}
			}
			else if(exid == _MAKE_ID('t','e','x','r'))
			{
				//テクスチャパス

				if(exsize)
				{
					pd->texpath = (char *)mMalloc0(exsize + 1);

					fread(pd->texpath, 1, exsize, fp);
				}
			}
			else
				fseek(fp, exsize, SEEK_CUR);
		}
	}

	//----- 新規イメージ
	// :レイヤ情報をすべて読み込まないと、8bit/16bit を判断できない。
	// :A1bit または、フォルダのみの場合は、8bit として読み込む。

	if(!drawImage_newCanvas_openFile(APPDRAW, p->width, p->height, imgbits, p->dpi))
		return MLKERR_ALLOC;

	return MLKERR_OK;
}

/* レイヤを作成 */

static mlkerr _create_layers(_loadv3 *p)
{
	_layerinfo *info;
	LayerItem *pi;
	TileImage *img;
	int i,j,coltype,type;
	uint32_t u32;

	info = p->layerinfo;

	for(i = p->layernum; i; i--, info++)
	{
		//レイヤ作成

		pi = LayerList_addLayer_index(APPDRAW->layerlist, info->parent_no, -1);
		if(!pi) return MLKERR_ALLOC;

		coltype = info->coltype;

		//データ (共通)

		pi->name = mStrdup(info->name);

		pi->opacity = info->opacity;
		pi->alphamask = info->amask;
		pi->flags = info->flags;

		//フォルダ時

		if(coltype == APD3_COLTYPE_FOLDER)
			continue;

		//---- イメージレイヤ

		//レイヤタイプ

		if(info->channels == 4 && (coltype == APD3_COLTYPE_8BIT || coltype == APD3_COLTYPE_16BIT))
			//RGBA: 8,16bit
			type = LAYERTYPE_RGBA;
		else if(info->channels == 2 && coltype == APD3_COLTYPE_16BIT)
			//GRAY: 16bit
			type = LAYERTYPE_GRAY;
		else
		{
			if(coltype == APD3_COLTYPE_16BIT)
				//A16
				type = LAYERTYPE_ALPHA;
			else if(coltype == APD3_COLTYPE_1BIT)
				//A1
				type = LAYERTYPE_ALPHA1BIT;
			else
				return MLKERR_INVALID_VALUE;
		}

		//イメージ作成
		
		img = TileImage_newFromRect_forFile(type, &info->rc);
		if(!img) return MLKERR_ALLOC;

		LayerItem_replaceImage(pi, img, type);

		//線の色

		LayerItem_setLayerColor(pi, MLK_RGB(info->r, info->g, info->b));

		//テクスチャ

		if(info->texpath)
			LayerItem_setTexture(pi, info->texpath);

		//合成モード

		u32 = info->blendid;

		for(j = 0; g_blendmode_id[j]; j++)
		{
			if(u32 == g_blendmode_id[j])
			{
				pi->blendmode = j;
				break;
			}
		}
	}

	return MLKERR_OK;
}

/* レイヤタイルの読み込み開始 */

static mlkerr _begin_layer_tile(_loadv3 *p)
{
	uint16_t tsize;
	uint8_t type;
	mlkerr ret;

	//壊れている場合でも、途中まで読み込む

	ret = _goto_thunk(p, _MAKE_ID('L','Y','T','L'), FALSE);
	if(ret) return ret;

	if(p->thunk_size < 3)
		return MLKERR_DAMAGED;

	mFILEreadBE16(p->fp, &tsize); //タイルのpxサイズ
	mFILEreadByte(p->fp, &type);  //圧縮タイプ

	if(tsize != 64 || type != 0)
		return MLKERR_UNSUPPORTED;

	return MLKERR_OK;
}

/* 各レイヤのタイルイメージを読み込み (フォルダの分も含む) */

static mlkerr _read_layer_tile(_loadv3 *p,LayerItem *item)
{
	TileImage *img;
	uint32_t num,encsize;
	uint8_t buf[4],**pptile;
	int tx,ty;
	mlkerr ret;

	//タイル数、圧縮サイズ

	if(mFILEreadBE32(p->fp, &num)
		|| mFILEreadBE32(p->fp, &encsize))
		return MLKERR_DAMAGED;

	//タイルなし

	if(num == 0)
	{
		mPopupProgressThreadAddPos(p->prog, _PROGRESS_STEP);
		return MLKERR_OK;
	}

	//タイルがあるのにイメージがない場合、エラー

	img = item->img;
	if(!img) return MLKERR_INVALID_VALUE;

	//------ タイル

	mZlibDecReset(p->zlib);
	mZlibDecSetSize(p->zlib, encsize);

	mPopupProgressThreadSubStep_begin(p->prog, _PROGRESS_STEP, num);

	for(; num; num--)
	{
		//タイル位置

		ret = mZlibDecRead(p->zlib, buf, 4);
		if(ret) return ret;

		tx = mGetBufBE16(buf);
		ty = mGetBufBE16(buf + 2);

		if(tx >= img->tilew || ty >= img->tileh)
			return MLKERR_INVALID_VALUE;

		pptile = TILEIMAGE_GETTILE_BUFPT(img, tx, ty);

		//ソースのタイル読み込み

		ret = mZlibDecRead(p->zlib, p->tilebuf, img->tilesize);
		if(ret) return ret;

		//タイル確保

		*pptile = TileImage_allocTile(img);
		if(!(*pptile)) return MLKERR_ALLOC;

		//変換

		TileImage_converTile_APDv3(img, *pptile, p->tilebuf);

		mPopupProgressThreadSubStep_inc(p->prog);
	}

	ret = mZlibDecFinish(p->zlib);

	return ret;
}


//=============================
// main
//=============================


/* 読み込み処理 */

static mlkerr _proc_main(AppDraw *p,_loadv3 *load)
{
	LayerItem *pi;
	mlkerr ret;

	//レイヤヘッダ

	ret = _read_layer_header(load);
	if(ret) return ret;

	//レイヤツリー

	ret = _read_layer_tree(load);
	if(ret) return ret;

	//レイヤ情報

	ret = _read_layer_info(load);
	if(ret) return ret;

	//レイヤ作成

	ret = _create_layers(load);
	if(ret) return ret;

	_free_layerinfo(load);

	//レイヤタイル

	ret = _begin_layer_tile(load);
	if(ret) return ret;

	for(pi = LayerList_getTopItem(p->layerlist); pi; pi = LayerItem_getNext(pi))
	{
		ret = _read_layer_tile(load, pi);
		if(ret) return ret;
	}

	//カレントレイヤ

	p->curlayer = LayerList_getItemAtIndex(p->layerlist, load->layer_curno);

	return MLKERR_OK;
}

/** APD v3 読み込み */

mlkerr drawFile_load_apd_v3(AppDraw *p,const char *filename,mPopupProgress *prog)
{
	_loadv3 *load;
	mlkerr ret;

	ret = _loadv3_open(&load, filename, prog);
	if(ret) return ret;

	ret = _proc_main(p, load);

	_loadv3_close(load);

	return ret;
}

