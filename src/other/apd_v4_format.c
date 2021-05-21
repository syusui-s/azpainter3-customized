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

#include <stdio.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_popup_progress.h"
#include "mlk_stdio.h"
#include "mlk_zlib.h"
#include "mlk_util.h"
#include "mlk_unicode.h"

#include "def_macro.h"
#include "def_draw.h"

#include "layerlist.h"
#include "layeritem.h"
#include "def_tileimage.h"
#include "tileimage.h"

#include "draw_main.h"

#include "apd_v4_format.h"
#include "pv_apd_format.h"


//-------------------

/* load */

struct _apd4load
{
	FILE *fp;
	mPopupProgress *prog;
	mZlib *zlib;
	uint8_t *workbuf;

	apd4info info;

	off_t fpos;
};

/* save */

struct _apd4save
{
	FILE *fp;
	mPopupProgress *prog;
	mZlib *zlib;
	uint8_t *tilebuf;

	uint32_t tilenum, //総数
		curtsize;
	int curtnum;
	off_t fpos;
};

//-------------------


//********************************
// 読み込み
//********************************


/* workbuf 解放 */

static void _load_free_workbuf(apd4load *p)
{
	mFree(p->workbuf);
	p->workbuf = NULL;
}

/** 閉じる */

void apd4load_close(apd4load *p)
{
	if(p)
	{
		if(p->fp) fclose(p->fp);

		mZlibFree(p->zlib);
		mFree(p->workbuf);
		
		mFree(p);
	}
}

/* 先頭情報の取得 */

static mlkerr _load_headinfo(apd4load *p)
{
	uint32_t width,height,dpi;
	uint16_t size,layernum;
	uint8_t bits,coltype,r,g,b;
	uint8_t buf[21];
	apd4info *info = &p->info;

	//情報

	if(fseek(p->fp, 8, SEEK_SET)	//ヘッダスキップ
		|| mFILEreadOK(p->fp, buf, 21))
		return MLKERR_DAMAGED;

	mGetBuf_format(buf, ">hiiibbbbbh",
		&size, &width, &height, &dpi,
		&bits, &coltype, &r, &g, &b, &layernum);

	if(fseek(p->fp, size - 19, SEEK_CUR))
		return MLKERR_DAMAGED;

	//チェック

	if(width > IMAGE_SIZE_MAX || height > IMAGE_SIZE_MAX)
		return MLKERR_MAX_SIZE;

	if((bits != 8 && bits != 16)
		|| coltype != 0)
		return MLKERR_UNSUPPORTED;

	//

	info->width = width;
	info->height = height;
	info->dpi = dpi;
	info->bits = bits;
	info->coltype = coltype;
	info->layernum = layernum;
	info->bkgndcol = MLK_RGB(r,g,b);

	return MLKERR_OK;
}

/* 初期化 */

static mlkerr _load_init(apd4load *p,const char *filename)
{
	//開く

	p->fp = mFILEopen(filename, "rb");
	if(!p->fp) return MLKERR_OPEN;

	//zlib

	p->zlib = mZlibDecNew(8192, -15);
	if(!p->zlib) return MLKERR_ALLOC;

	mZlibSetIO_stdio(p->zlib, p->fp);

	//先頭情報の取得

	return _load_headinfo(p);
}

/** 開く */

mlkerr apd4load_open(apd4load **ppdst,const char *filename,mPopupProgress *prog)
{
	apd4load *p;
	mlkerr ret;

	*ppdst = NULL;

	p = (apd4load *)mMalloc0(sizeof(apd4load));
	if(!p) return MLKERR_ALLOC;

	p->prog = prog;

	//初期化

	ret = _load_init(p, filename);

	if(ret == MLKERR_OK)
		*ppdst = p;
	else
		apd4load_close(p);

	return ret;
}

/** 情報を取得
 *
 * open 後ならいつでも可。 */

void apd4load_getInfo(apd4load *p,apd4info *info)
{
	*info = p->info;
}


//------- チャンク


/** 次のチャンクを読み込み
 *
 * return: -1 で終了 */

mlkerr apd4load_nextChunk(apd4load *p,uint32_t *pid,uint32_t *psize)
{
	uint32_t id,size;

	//次の位置へ

	if(p->fpos)
	{
		if(fseeko(p->fp, p->fpos, SEEK_SET))
			return MLKERR_DAMAGED;
	}

	//ID とサイズ

	if(mFILEreadBE32(p->fp, &id))
		return MLKERR_DAMAGED;

	if(id == 0) return -1;

	if(mFILEreadBE32(p->fp, &size))
		return MLKERR_DAMAGED;

	*pid = id;
	*psize = size;

	//次の位置

	p->fpos = ftello(p->fp) + size;

	return MLKERR_OK;
}

/** すべてのチャンクをスキップしてレイヤ情報へ */

mlkerr apd4load_skipChunk(apd4load *p)
{
	mlkerr ret;
	uint32_t id,size;

	while(1)
	{
		ret = apd4load_nextChunk(p, &id, &size);

		if(ret == -1)
			break;
		else if(ret)
			return ret;
	}

	return MLKERR_OK;
}

/** (チャンク読み込み) サムネイルの情報取得 & 読み込み開始 */

mlkerr apd4load_readChunk_thumbnail(apd4load *p,mSize *psize)
{
	uint16_t w,h;
	uint32_t size;

	if(mFILEreadBE16(p->fp, &w)
		|| mFILEreadBE16(p->fp, &h)
		|| mFILEreadBE32(p->fp, &size))
		return MLKERR_DAMAGED;

	psize->w = w;
	psize->h = h;

	mZlibDecReset(p->zlib);
	mZlibDecSetSize(p->zlib, size);

	return MLKERR_OK;
}

/** サムネイルイメージの読み込み
 *
 * dstbuf: R,G,B 8bit で読み込まれる
 * pxnum: 読み込む px 数 */

mlkerr apd4load_readChunk_thumbnail_image(apd4load *p,uint8_t *dstbuf,int pxnum)
{
	return mZlibDecRead(p->zlib, dstbuf, pxnum * 3);
}


//------- レイヤ: 拡張データ


/* 拡張データ: 文字列読み込み */

static mlkerr _load_layerex_str(apd4load *p,char **ppdst,uint32_t size)
{
	char *buf;

	buf = (char *)mMalloc(size + 1);
	if(!buf) return MLKERR_ALLOC;

	if(mFILEreadOK(p->fp, buf, size))
	{
		mFree(buf);
		return MLKERR_DAMAGED;
	}

	buf[size] = 0;

	mUTF8Validate(buf, size);

	*ppdst = buf;

	return MLKERR_OK;
}

/* 拡張データ: テキストデータ */

static mlkerr _load_layerex_textdat(apd4load *p,LayerItem *pi,uint32_t size)
{
	LayerTextItem *pt;
	int32_t x,y,datsize;
	mRect rc;

	//情報
	
	if(mFILEreadOK(p->fp, p->workbuf, 28))
		return MLKERR_DAMAGED;

	mGetBuf_format(p->workbuf, ">iiiiiii",
		&x, &y, &rc.x1, &rc.y1, &rc.x2, &rc.y2, &datsize);

	size -= 28;

	//追加

	pt = LayerItem_addText_read(pi, x, y, &rc, datsize);
	if(!pt) return MLKERR_ALLOC;

	//データ

	if(mFILEreadOK(p->fp, pt->dat, datsize))
		return MLKERR_DAMAGED;

	size -= datsize;

	//残り

	if(fseek(p->fp, size, SEEK_CUR))
		return MLKERR_DAMAGED;

	return MLKERR_OK;
}

/* 拡張データ読み込み */

static mlkerr _load_layer_ex(apd4load *p,LayerItem *pi)
{
	uint32_t id,size;
	mlkerr ret;

	while(1)
	{
		if(mFILEreadBE32(p->fp, &id))
			return MLKERR_DAMAGED;

		if(id == 0) break;
		
		if(mFILEreadBE32(p->fp, &size))
			return MLKERR_DAMAGED;

		//

		switch(id)
		{
			//レイヤ名
			case _MAKE_ID('n','a','m','e'):
				ret = _load_layerex_str(p, &pi->name, size);
				break;
			//テクスチャパス
			case _MAKE_ID('t','e','x','p'):
				ret = _load_layerex_str(p, &pi->texture_path, size);

				if(ret == MLKERR_OK)
					LayerItem_loadTextureImage(pi);
				break;
			//テキスト
			case _MAKE_ID('t','e','x','t'):
				ret = _load_layerex_textdat(p, pi, size);
				break;

			//スキップ
			default:
				if(fseeko(p->fp, size, SEEK_CUR))
					ret = MLKERR_DAMAGED;
				else
					ret = MLKERR_OK;
				break;
		}

		if(ret) return ret;
	}

	return MLKERR_OK;
}


//----------- レイヤ


/* タイルイメージ読み込み */

static mlkerr _load_layer_image(apd4load *p,TileImage *img)
{
	FILE *fp = p->fp;
	mZlib *zlib = p->zlib;
	uint8_t *buf = p->workbuf;
	uint32_t size,tilenum;
	uint16_t tnum,tx,ty;
	uint8_t comptype;
	int i,tilesize;
	mlkerr ret;

	//圧縮タイプ, タイル総数

	if(mFILEreadByte(fp, &comptype)
		|| mFILEreadBE32(fp, &tilenum))
		return MLKERR_DAMAGED;

	if(comptype != 0)
		return MLKERR_INVALID_VALUE;

	//空イメージ

	if(tilenum == 0)
	{
		mPopupProgressThreadAddPos(p->prog, 6);
		return MLKERR_OK;
	}

	//--- タイル

	mPopupProgressThreadSubStep_begin(p->prog, 6, tilenum);

	tilesize = img->tilesize;

	while(tilenum)
	{
		//タイル数, 圧縮サイズ
		
		if(mFILEreadBE16(fp, &tnum)
			|| mFILEreadBE32(fp, &size))
			return MLKERR_DAMAGED;

		//ブロック読み込み

		mZlibDecReset(zlib);
		mZlibDecSetSize(zlib, size);

		for(i = tnum; i; i--)
		{
			ret = mZlibDecRead(zlib, buf, tilesize + 4);
			if(ret) return ret;

			tx = mGetBufBE16(buf);
			ty = mGetBufBE16(buf + 2);

			if(!TileImage_setTile_fromSave(img, tx, ty, buf + 4))
				return MLKERR_ALLOC;

			mPopupProgressThreadSubStep_inc(p->prog);
		}

		ret = mZlibDecFinish(zlib);
		if(ret) return ret;

		//

		tilenum -= tnum;
	}

	return MLKERR_OK;
}

/* 一つのレイヤを読み込み
 *
 * ppitem: NULL 以外で、作成されたアイテムが入る
 * fadd: TRUE で、カレントレイヤ上に追加
 * return: -1 で終了 */

static mlkerr _load_layer(apd4load *p,LayerItem **ppitem,mlkbool fadd)
{
	AppDraw *draw = APPDRAW;
	FILE *fp = p->fp;
	LayerItem *pi,*pi_parent;
	TileImage *img;
	uint32_t blendmode,col,flags;
	uint16_t parent_no,tone_lines,tone_angle;
	uint8_t lflags,coltype,opacity,amask,tone_density,fimg;
	int i;
	mlkerr ret;
	mRect rc;

	//親のレイヤ番号

	if(mFILEreadBE16(fp, &parent_no))
		return MLKERR_DAMAGED;

	if(parent_no == 0xffff) return -1;

	//情報

	if(mFILEreadOK(fp, p->workbuf, 37))
		return MLKERR_DAMAGED;

	mGetBuf_format(p->workbuf,
		">bbiiiibbiiihhb",
		&lflags, &coltype,
		&rc.x1, &rc.y1, &rc.x2, &rc.y2,
		&opacity, &amask, &blendmode, &col, &flags,
		&tone_lines, &tone_angle, &tone_density);

	fimg = !(lflags & 1);

	//レイヤ作成

	if(fadd)
	{
		//---- カレント上に追加時

		pi = LayerList_addLayer(draw->layerlist, draw->curlayer);
		if(!pi) return MLKERR_ALLOC;
	}
	else
	{
		//---- 新規キャンバス
		
		if(parent_no == 0xfffe)
			pi_parent = NULL;
		else
			pi_parent = LayerList_getItemAtIndex(draw->layerlist, parent_no);

		pi = LayerList_addLayer_parent(draw->layerlist, pi_parent);
		if(!pi) return MLKERR_ALLOC;

		//カレントレイヤ

		if(lflags & 2)
			draw->curlayer = pi;
	}

	if(ppitem) *ppitem = pi;

	//情報セット

	pi->opacity = (opacity > 128)? 128: opacity;
	pi->alphamask = (amask > 3)? 0: amask;
	pi->col = col & 0xffffff;
	pi->flags = flags;
	pi->tone_lines = tone_lines;
	pi->tone_angle = tone_angle;
	pi->tone_density = tone_density;

	for(i = 0; g_blendmode_id[i]; i++)
	{
		if(blendmode == g_blendmode_id[i])
		{
			pi->blendmode = i;
			break;
		}
	}

	//拡張データ

	ret = _load_layer_ex(p, pi);
	if(ret) return ret;

	//タイルイメージ

	if(!fimg)
		mPopupProgressThreadAddPos(p->prog, 6);
	else
	{
		img = TileImage_newFromRect_forFile(coltype, &rc);
		if(!img) return MLKERR_ALLOC;

		LayerItem_replaceImage(pi, img, coltype);
		LayerItem_setLayerColor(pi, pi->col);

		ret = _load_layer_image(p, img);
		if(ret) return ret;
	}

	return MLKERR_OK;
}

/** レイヤをすべて読み込み */

mlkerr apd4load_readLayers(apd4load *p)
{
	mlkerr ret;

	//タイルバッファ

	p->workbuf = (uint8_t *)mMalloc(64 * 64 * 8 + 4);
	if(!p->workbuf) return MLKERR_ALLOC;

	//

	mPopupProgressThreadSetMax(p->prog, p->info.layernum * 6);

	while(1)
	{
		ret = _load_layer(p, NULL, FALSE);

		if(ret == -1)
			break;
		else if(ret)
			return ret;
	}

	_load_free_workbuf(p);

	return MLKERR_OK;
}

/** 単体レイヤをカレントレイヤ上に追加 */

mlkerr apd4load_readLayer_append(apd4load *p,LayerItem **ppitem)
{
	mlkerr ret;

	*ppitem = NULL;

	//タイルバッファ

	p->workbuf = (uint8_t *)mMalloc(64 * 64 * 8 + 4);
	if(!p->workbuf) return MLKERR_ALLOC;

	//レイヤ

	mPopupProgressThreadSetMax(p->prog, 6);

	ret = _load_layer(p, ppitem, TRUE);

	if(ret)
	{
		if(ret == -1)
			ret = MLKERR_UNFOUND;

		//レイヤ追加後にエラーが出た場合、レイヤ削除

		if(*ppitem)
		{
			LayerList_deleteLayer(APPDRAW->layerlist, *ppitem);
			ppitem = NULL;
		}
	}

	_load_free_workbuf(p);

	return ret;
}


//********************************
// 保存
//********************************


/** APD に単体レイヤ保存 */

mlkerr apd4save_saveSingleLayer(LayerItem *item,const char *filename,mPopupProgress *prog)
{
	apd4save *sav;
	mlkerr ret;

	ret = apd4save_open(&sav, filename, prog);
	if(ret) return ret;

	//ヘッダ

	ret = apd4save_writeHeadInfo(sav, 1);
	if(ret) goto ERR;

	//チャンク終了

	ret = apd4save_writeChunk_end(sav);
	if(ret) goto ERR;

	//レイヤ

	mPopupProgressThreadSetMax(prog, 20);

	ret = apd4save_writeLayer(sav, item, TRUE, 20);
	if(ret) goto ERR;

	ret = apd4save_writeLayer_end(sav);

ERR:
	apd4save_close(sav);

	return ret;
}


/** 閉じる */

void apd4save_close(apd4save *p)
{
	if(p)
	{
		if(p->fp) fclose(p->fp);

		mZlibFree(p->zlib);
		mFree(p->tilebuf);
		
		mFree(p);
	}
}

/* 初期化 */

static mlkerr _save_init(apd4save *p,const char *filename)
{
	//開く

	p->fp = mFILEopen(filename, "wb");
	if(!p->fp) return MLKERR_OPEN;

	//作業用バッファ

	p->tilebuf = (uint8_t *)mMalloc(64 * 64 * 8 + 4); //タイル位置分 +4
	if(!p->tilebuf) return MLKERR_ALLOC;

	//zlib

	p->zlib = mZlibEncNew(8192, 6, -15, 8, 0);
	if(!p->zlib) return MLKERR_ALLOC;

	mZlibSetIO_stdio(p->zlib, p->fp);

	return MLKERR_OK;
}

/** 開く */

mlkerr apd4save_open(apd4save **ppdst,const char *filename,mPopupProgress *prog)
{
	apd4save *p;
	mlkerr ret;

	*ppdst = NULL;

	p = (apd4save *)mMalloc0(sizeof(apd4save));
	if(!p) return MLKERR_ALLOC;

	p->prog = prog;

	//初期化

	ret = _save_init(p, filename);

	if(ret == MLKERR_OK)
		*ppdst = p;
	else
		apd4save_close(p);

	return ret;
}

/** 先頭情報書き込み */

mlkerr apd4save_writeHeadInfo(apd4save *p,int layernum)
{
	AppDraw *pd = APPDRAW;
	FILE *fp = p->fp;
	uint8_t *buf = p->tilebuf;

	//ヘッダ

	fputs("AZPDATA", fp);

	mFILEwriteByte(fp, 3);

	//

	mSetBuf_format(buf, ">hiiibbbbbh",
		19, pd->imgw, pd->imgh, pd->imgdpi,
		pd->imgbits, 0,
		pd->imgbkcol.c8.r, pd->imgbkcol.c8.g, pd->imgbkcol.c8.b,
		layernum);

	mFILEwriteOK(fp, buf, 19 + 2);

	return MLKERR_OK;
}


//------- チャンク


/** (チャンク) サムネイル画像書き込み */

mlkerr apd4save_writeChunk_thumbnail(apd4save *p,uint8_t **ppbuf,int width,int height)
{
	mlkerr ret;
	int size,iy;
	off_t pos;

	pos = ftello(p->fp) + 4;

	//情報

	size = mSetBuf_format(p->tilebuf, ">iihhi",
		_MAKE_ID('t','h','u','m'), 0,
		width, height, 0);

	if(mFILEwriteOK(p->fp, p->tilebuf, size))
		return MLKERR_IO;

	//圧縮

	size = width * 3;

	mZlibEncReset(p->zlib);

	for(iy = height; iy; iy--, ppbuf++)
	{
		ret = mZlibEncSend(p->zlib, *ppbuf, size);
		if(ret) return ret;
	}

	ret = mZlibEncFinish(p->zlib);
	if(ret) return ret;

	//サイズ

	size = mZlibEncGetSize(p->zlib);

	if(fseeko(p->fp, pos, SEEK_SET)
		|| mFILEwriteBE32(p->fp, 8 + size) //チャンクサイズ
		|| fseek(p->fp, 4, SEEK_CUR)
		|| mFILEwriteBE32(p->fp, size) //圧縮サイズ
		|| fseek(p->fp, 0, SEEK_END))
		return MLKERR_IO;

	return MLKERR_OK;
}

/** チャンク: 一枚絵イメージを書き込み */

mlkerr apd4save_writeChunk_picture(apd4save *p,uint8_t **ppbuf,int width,int height,int stepnum)
{
	FILE *fp = p->fp;
	mlkerr ret;
	int pitch,ynum;
	off_t postop,pos;
	uint32_t imgsize,encsize,csize = 0;

	postop = ftello(fp) + 4;

	//チャンク情報

	if(mFILEwriteBE32(fp, _MAKE_ID('p','i','c','t'))
		|| mFILEwrite0(fp, 4))
		return MLKERR_IO;

	//圧縮

	pitch = width * 3;
	ynum = 0;
	imgsize = 0;

	mPopupProgressThreadSubStep_begin(p->prog, stepnum, height);

	while(height)
	{
		//情報

		if(!ynum)
		{
			pos = ftello(fp);

			if(mFILEwrite0(fp, 6))
				return MLKERR_IO;
			
			mZlibEncReset(p->zlib);
		}

		//送る
	
		ret = mZlibEncSend(p->zlib, *ppbuf, pitch);
		if(ret) return ret;

		ppbuf++;
		ynum++;
		height--;
		imgsize += pitch;

		mPopupProgressThreadSubStep_inc(p->prog);

		//ブロック終わり

		if(!height || imgsize >= 0x1000000)
		{
			ret = mZlibEncFinish(p->zlib);
			if(ret) return ret;

			encsize = mZlibEncGetSize(p->zlib);

			if(fseeko(fp, pos, SEEK_SET)
				|| mFILEwriteBE16(fp, ynum)
				|| mFILEwriteBE32(fp, encsize)
				|| fseek(fp, 0, SEEK_END))
				return MLKERR_IO;

			ynum = 0;
			imgsize = 0;
			csize += 6 + encsize;
		}
	}

	//チャンクサイズ

	if(fseeko(fp, postop, SEEK_SET)
		|| mFILEwriteBE32(fp, csize)
		|| fseek(fp, 0, SEEK_END))
		return MLKERR_IO;

	return MLKERR_OK;
}

/** チャンク終端を書き込み */

mlkerr apd4save_writeChunk_end(apd4save *p)
{
	mFILEwrite0(p->fp, 4);

	return MLKERR_OK;
}


//------ 拡張データ

/* 拡張データ:文字列書き込み
 *
 * text: NULL で書き込まない */

static mlkerr _write_layerex_str(apd4save *p,uint32_t id,const char *text)
{
	int len;

	if(text)
	{
		len = mStrlen(text);

		if(mFILEwriteBE32(p->fp, id)
			|| mFILEwriteBE32(p->fp, len)
			|| mFILEwriteOK(p->fp, text, len))
			return MLKERR_IO;
	}

	return MLKERR_OK;
}

/* 拡張データ: テキストデータ書き込み */

static mlkerr _write_layerex_textdata(apd4save *p,mList *list)
{
	LayerTextItem *pi;
	FILE *fp = p->fp;
	uint8_t *buf = p->tilebuf;
	int size;

	for(pi = (LayerTextItem *)list->top; pi; pi = (LayerTextItem *)pi->i.next)
	{
		size = mSetBuf_format(buf, ">iiiiiiiii",
			_MAKE_ID('t','e','x','t'), pi->datsize + 28,
			pi->x, pi->y,
			pi->rcdraw.x1, pi->rcdraw.y1, pi->rcdraw.x2, pi->rcdraw.y2,
			pi->datsize);

		if(mFILEwriteOK(fp, buf, size)
			|| mFILEwriteOK(fp, pi->dat, pi->datsize))
			return MLKERR_IO;
	}

	return MLKERR_OK;
}

/* 拡張データ書き込み */

static mlkerr _write_layerex(apd4save *p,LayerItem *pi)
{
	mlkerr ret;

	//名前

	ret = _write_layerex_str(p, _MAKE_ID('n','a','m','e'), pi->name);
	if(ret) return ret;

	//テクスチャパス

	ret = _write_layerex_str(p, _MAKE_ID('t','e','x','p'), pi->texture_path);
	if(ret) return ret;

	//テキストデータ

	if(pi->list_text.top)
	{
		ret = _write_layerex_textdata(p, &pi->list_text);
		if(ret) return ret;
	}

	//終端

	if(mFILEwrite0(p->fp, 4)) return MLKERR_IO;

	return MLKERR_OK;
}

//------ タイルイメージ

/* タイル書き込み関数 */

static mlkerr _func_savetile(TileImage *img,void *param)
{
	apd4save *p = (apd4save *)param;
	FILE *fp = p->fp;
	mlkerr ret;

	//先頭時、情報を仮書き込み

	if(p->curtnum == 0)
	{
		p->fpos = ftello(fp);

		if(mFILEwrite0(fp, 6))
			return MLKERR_IO;

		mZlibEncReset(p->zlib);
	}

	//圧縮

	ret = mZlibEncSend(p->zlib, p->tilebuf, img->tilesize + 4);
	if(ret) return ret;

	p->tilenum--;
	p->curtnum++;
	p->curtsize += img->tilesize;

	//ブロック終わり

	if(p->tilenum == 0 || p->curtsize >= 0x100000 || p->curtnum == 0xffff)
	{
		ret = mZlibEncFinish(p->zlib);
		if(ret) return ret;
		
		if(fseeko(fp, p->fpos, SEEK_SET)
			|| mFILEwriteBE16(fp, p->curtnum)
			|| mFILEwriteBE32(fp, mZlibEncGetSize(p->zlib))
			|| fseek(fp, 0, SEEK_END))
			return MLKERR_IO;

		p->curtnum = 0;
		p->curtsize = 0;
	}

	//
	
	mPopupProgressThreadSubStep_inc(p->prog);

	return MLKERR_OK;
}

/* タイルイメージ書き込み */

static mlkerr _write_tileimage(apd4save *p,TileImage *img,mRect *rc,uint32_t tilenum,int stepnum)
{
	//圧縮タイプ, タイル総数

	if(mFILEwriteByte(p->fp, 0)
		|| mFILEwriteBE32(p->fp, tilenum))
		return MLKERR_IO;

	//空イメージ

	if(!tilenum)
	{
		mPopupProgressThreadAddPos(p->prog, stepnum);
		return MLKERR_OK;
	}

	//タイル書き込み

	mPopupProgressThreadSubStep_begin(p->prog, stepnum, tilenum);

	p->tilenum = tilenum;
	p->curtsize = 0;
	p->curtnum = 0;

	return TileImage_saveTiles_apd4(img, rc, p->tilebuf, _func_savetile, p);
}

/** レイヤの書き込み
 *
 * parent_root: 親レイヤを常にルートにする */

mlkerr apd4save_writeLayer(apd4save *p,LayerItem *pi,mlkbool parent_root,int stepnum)
{
	uint32_t tilenum;
	uint16_t parent;
	uint8_t lflags;
	int size,fimg;
	mlkerr ret;
	mRect rc;

	fimg = LAYERITEM_IS_IMAGE(pi);

	//親のレイヤ番号

	if(parent_root || !pi->i.parent)
		parent = 0xfffe;
	else
		parent = LayerList_getItemIndex(APPDRAW->layerlist, (LayerItem *)pi->i.parent);

	//フラグ

	lflags = 0;

	if(LAYERITEM_IS_FOLDER(pi)) lflags |= 1;

	if(pi == APPDRAW->curlayer) lflags |= 2;

	//イメージ範囲

	if(fimg && TileImage_getHaveImageRect_pixel(pi->img, &rc, &tilenum))
	{
		rc.x2++;
		rc.y2++;
	}
	else
		mMemset0(&rc, sizeof(mRect));

	//情報

	size = mSetBuf_format(p->tilebuf,
		">hbbiiiibbiiihhb",
		parent, lflags, pi->type,
		rc.x1, rc.y1, rc.x2, rc.y2,
		pi->opacity, pi->alphamask,
		g_blendmode_id[pi->blendmode], pi->col, pi->flags,
		pi->tone_lines, pi->tone_angle, pi->tone_density);

	if(mFILEwriteOK(p->fp, p->tilebuf, size))
		return MLKERR_IO;

	//拡張データ

	ret = _write_layerex(p, pi);
	if(ret) return ret;

	//イメージデータ

	if(!fimg)
		mPopupProgressThreadAddPos(p->prog, stepnum);
	else
	{
		ret = _write_tileimage(p, pi->img, &rc, tilenum, stepnum);
		if(ret) return ret;
	}

	return MLKERR_OK;
}

/** レイヤ終端の書き込み */

mlkerr apd4save_writeLayer_end(apd4save *p)
{
	if(mFILEwriteBE16(p->fp, 0xffff))
		return MLKERR_IO;
	else
		return MLKERR_OK;
}

