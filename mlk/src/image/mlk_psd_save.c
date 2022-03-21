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
 * PSD 書き込み
 *****************************************/

#include <stdio.h>
#include <string.h>

#include "mlk.h"
#include "mlk_rectbox.h"
#include "mlk_list.h"
#include "mlk_stdio.h"
#include "mlk_util.h"
#include "mlk_packbits.h"
#include "mlk_unicode.h"

#include "mlk_psd.h"


//-------------------

/* リソースアイテム */

typedef struct
{
	mListItem i;
	uint16_t id;
	uint32_t size,
		size2;	//2byte 単位でのサイズ

	union
	{
		uint8_t *buf;	//データのバッファ (確保)
		uint8_t dat[4]; //サイズが 4byte 以下の場合に使われる
	};
}_resitem;

/* レイヤ情報 */

typedef struct
{
	mPSDLayer i;

	off_t fpos_chinfo;	//チャンネル情報の位置
	uint8_t *exdata;	//拡張データ (まとめて一つに)
	uint32_t exdata_size;
}_layerinfo;

/* mPSDSave */

struct _mPSDSave
{
	mPSDHeader hd;
	mPackBits packbits;
	
	mList list_res;		//リソースデータ

	FILE *fp;

	uint8_t *outbuf;		//出力用バッファ
	uint16_t *encsizebuf;	//PackBits 圧縮サイズバッファ (BE)

	_layerinfo *layerinfo,	//レイヤ情報
		*curlayer;

	mBox *curimgbox;

	int fp_open,		//ファイルから開いたか
		compress,		//圧縮タイプ: 0=無圧縮 1=PackBits(default)
		rowsize,		//1ch のY1行サイズ
		layer_num,		//レイヤ数
		cur_line,
		cur_layerno,
		cur_chno,
		cur_chid,
		layerimg_maxw,	//レイヤイメージの最大サイズ
		layerimg_maxh;

	uint32_t size1,
		imgencsize;		//イメージの全体サイズ

	off_t fpos1,
		fpos2;
};

//-------------------

#define _OUTBUFSIZE  (8 * 1024)

//-------------------


//=========================
// sub
//=========================


/* PackBits 書き込み関数 */

static mlkerr _packbits_write(mPackBits *p,uint8_t *buf,int size)
{
	if(fwrite(buf, 1, size, (FILE *)p->param) != size)
		return MLKERR_IO;

	return MLKERR_OK;
}

/* 幅から 1ch 分のY1行データサイズ取得 */

static int _get_ch_rowsize(mPSDSave *p,int width)
{
	if(p->hd.bits == 1)
		width = (width + 7) >> 3;
	else if(p->hd.bits == 16)
		width <<= 1;

	return width;
}

/* outbuf のデータを書き込み
 *
 * return: 0 で成功 */

static int _put_outbuf(mPSDSave *p,int size)
{
	return (fwrite(p->outbuf, 1, size, p->fp) != size);
}

/* イメージエンコード準備
 *
 * レイヤ時は、各チャンネルごと */

static int _setup_image_write(mPSDSave *p,int width,int height,mlkbool is_layer)
{
	int size,have_img;

	have_img = (width && height);

	p->imgencsize = 2;

	//Y1行サイズ

	p->rowsize = _get_ch_rowsize(p, width);

	//圧縮タイプ書き込み

	if(mFILEwriteBE16(p->fp, (have_img)? p->compress: 0))
		return MLKERR_IO;

	p->fpos1 = ftello(p->fp);

	//PackBits
	//(空イメージの場合はスキップ)

	if(p->compress && have_img)
	{
		//圧縮サイズバッファ

		if(is_layer)
		{
			//レイヤ

			if(!p->encsizebuf)
			{
				p->encsizebuf = (uint16_t *)mMalloc0(p->layerimg_maxh * 2);
				if(!p->encsizebuf) return MLKERR_ALLOC;
			}

			size = height * 2;

			p->imgencsize += size;
		}
		else
		{
			//一枚絵
			
			mFree(p->encsizebuf);
		
			size = p->hd.height * p->hd.img_channels * 2;
			
			p->encsizebuf = (uint16_t *)mMalloc0(size);
			if(!p->encsizebuf) return MLKERR_ALLOC;

			p->size1 = size;
		}

		//仮書き込み

		if(fwrite(p->encsizebuf, 1, size, p->fp) != size)
			return MLKERR_IO;
	}

	return MLKERR_OK;
}

/* Y1行イメージ書き込み */

static int _write_row_image(mPSDSave *p,uint8_t *buf)
{
	int ret,rowsize;

	rowsize = p->rowsize;

	//16bit 時、BE へ

#if !defined(MLK_BIG_ENDIAN)
	if(p->hd.bits == 16)
		mSwapByte_16bit(buf, rowsize >> 1);
#endif

	//

	if(p->compress == 0)
	{
		//無圧縮

		if(fwrite(buf, 1, rowsize, p->fp) != rowsize)
			return MLKERR_IO;

		p->imgencsize += rowsize;
	}
	else
	{
		//PackBits

		p->packbits.buf = buf;
		p->packbits.bufsize = rowsize;
		p->packbits.param = p->fp;

		ret = mPackBits_encode(&p->packbits);
		if(ret) return ret;

		//圧縮サイズセット

		mSetBufBE16((uint8_t *)(p->encsizebuf + p->cur_line), p->packbits.encsize);

		p->imgencsize += p->packbits.encsize;
	}

	p->cur_line++;

	return MLKERR_OK;
}


//=========================
// main
//=========================


/* レイヤ情報解放 */

static void _free_layerinfo(mPSDSave *p)
{
	_layerinfo *pi = p->layerinfo;
	int i;

	for(i = p->layer_num; i > 0; i--, pi++)
	{
		mFree(pi->exdata);
	}

	mFree(p->layerinfo);
}

/* リソースアイテム解放 */

static void _resitem_destroy(mList *list,mListItem *item)
{
	_resitem *pi = (_resitem *)item;

	if(pi->size2 > 4)
		mFree(pi->buf);
}

/**@ 閉じる */

void mPSDSave_close(mPSDSave *p)
{
	if(p)
	{
		if(p->fp)
		{
			if(p->fp_open)
				fclose(p->fp);
			else
				fflush(p->fp);
		}

		mListDeleteAll(&p->list_res);

		if(p->layerinfo)
			_free_layerinfo(p);

		mFree(p->outbuf);
		mFree(p->encsizebuf);
		
		mFree(p);
	}
}

/**@ PSD 保存の作成
 *
 * @g:保存 */

mPSDSave *mPSDSave_new(void)
{
	mPSDSave *p;

	p = (mPSDSave *)mMalloc0(sizeof(mPSDSave));
	if(!p) return NULL;

	//出力用バッファ

	p->outbuf = (uint8_t *)mMalloc(_OUTBUFSIZE);
	if(!p->outbuf)
	{
		mFree(p);
		return NULL;
	}

	//

	p->compress = 1;
	p->list_res.item_destroy = _resitem_destroy;

	//

	p->packbits.workbuf = p->outbuf;
	p->packbits.worksize = _OUTBUFSIZE;
	p->packbits.readwrite = _packbits_write;

	return p;
}

/**@ 圧縮タイプを無圧縮にする */

void mPSDSave_setCompression_none(mPSDSave *p)
{
	p->compress = 0;
}

/**@ ファイルを開く
 *
 * @r:エラーコード */

mlkerr mPSDSave_openFile(mPSDSave *p,const char *filename)
{
	p->fp = mFILEopen(filename, "wb");
	if(!p->fp) return MLKERR_OPEN;

	p->fp_open = 1;

	return MLKERR_OK;
}

/**@ FILE * から開く */

mlkerr mPSDSave_openFILEptr(mPSDSave *p,void *fp)
{
	p->fp = (FILE *)fp;

	return MLKERR_OK;
}

/**@ ヘッダを書き込み
 *
 * @d:開いた後、常に最初に実行する。 */

mlkerr mPSDSave_writeHeader(mPSDSave *p,mPSDHeader *hd)
{
	int size;

	p->hd = *hd;

	//カラーモードサイズまで

	size = mSetBuf_format(p->outbuf, ">sh6zhiihh4z",
		"8BPS",
		1,	//version
		hd->img_channels,
		hd->height,
		hd->width,
		hd->bits,
		hd->colmode);

	if(_put_outbuf(p, size))
		return MLKERR_IO;

	return MLKERR_OK;
}


//===========================
// リソース
//===========================

/* size1 : リソース全体のサイズ */


/* リソースアイテム追加 */

static _resitem *_add_resitem(mPSDSave *p,uint16_t id,uint32_t size)
{
	_resitem *pi;
	uint32_t size2;

	pi = (_resitem *)mListAppendNew(&p->list_res, sizeof(_resitem));
	if(!pi) return NULL;

	size2 = (size + 1) & (~1);

	pi->id = id;
	pi->size = size;
	pi->size2 = size2;

	//4byte 以上の場合、バッファ確保 (余白含む)

	if(size2 > 4)
	{
		pi->buf = (uint8_t *)mMalloc(size2);
		if(!pi->buf)
		{
			mListDelete(&p->list_res, MLISTITEM(pi));
			return NULL;
		}

		//余白分を 0 に

		if(size != size2)
			pi->buf[size] = 0;
	}

	//リソース全体のサイズ

	p->size1 += 12 + size2;

	return pi;
}

/**@ リソースに解像度データを追加 */

mlkbool mPSDSave_res_setResoDPI(mPSDSave *p,int horz,int vert)
{
	_resitem *pi;

	pi = _add_resitem(p, MPSD_RESID_RESOLUTION, 16);
	if(!pi) return FALSE;

	mSetBuf_format(pi->buf, ">ihhihh",
		horz << 16, 1, 2,
		vert << 16, 1, 2);

	return TRUE;
}

/**@ リソースにカレントレイヤ番号を追加 */

mlkbool mPSDSave_res_setCurLayerNo(mPSDSave *p,int no)
{
	_resitem *pi;

	pi = _add_resitem(p, MPSD_RESID_CURRENT_LAYER, 2);
	if(!pi) return FALSE;

	mSetBufBE16(pi->dat, no);

	return TRUE;
}

/**@ リソースに ICC プロファイルデータを追加
 *
 * @d:データはコピーされる。 */

mlkbool mPSDSave_res_setICCProfile(mPSDSave *p,const void *buf,uint32_t size)
{
	_resitem *pi;

	pi = _add_resitem(p, MPSD_RESID_ICC_PROFILE, size);
	if(!pi) return FALSE;

	memcpy(pi->buf, buf, size);

	return TRUE;
}

/**@ リソースデータを書き込み
 *
 * @d:ヘッダ書き込みの後、常に実行すること。 */

mlkerr mPSDSave_writeResource(mPSDSave *p)
{
	_resitem *pi;
	uint8_t *outbuf = p->outbuf;
	uint32_t size;

	//リソースサイズ

	if(mFILEwriteBE32(p->fp, p->size1))
		return MLKERR_IO;

	//デフォルトデータ

	memcpy(outbuf, "8BIM", 4);

	//各データ

	MLK_LIST_FOR(p->list_res, pi, _resitem)
	{
		mSetBuf_format(outbuf + 4, ">h2zi", pi->id, pi->size);

		if(_put_outbuf(p, 12))
			return MLKERR_IO;

		//データ

		size = pi->size2;

		if(size <= 4)
		{
			if(fwrite(pi->dat, 1, size, p->fp) != size)
				return MLKERR_IO;
		}
		else
		{
			if(fwrite(pi->buf, 1, size, p->fp) != size)
				return MLKERR_IO;
		}
	}

	//解放

	mListDeleteAll(&p->list_res);

	return MLKERR_OK;
}


//===========================
// 一枚絵イメージ
//===========================

/* fpos1 : イメージ位置
 * size1 : PackBits 圧縮サイズリストのサイズ */


/**@ 一枚絵イメージのY1行サイズ取得 */

int mPSDSave_getImageRowSize(mPSDSave *p)
{
	return _get_ch_rowsize(p, p->hd.width);
}

/**@ 一枚絵イメージ書き込み開始
 *
 * @d:レイヤデータの書き込み後に行う。 */

mlkerr mPSDSave_startImage(mPSDSave *p)
{
	int ret;
	
	ret = _setup_image_write(p, p->hd.width, p->hd.height, FALSE);
	if(ret) return ret;

	p->cur_line = 0;

	return MLKERR_OK;
}

/**@ 一枚絵イメージのY1行データ書き込み
 *
 * @d:高さ x チャンネル数分を書き込む。\
 * 16bit 時はホストのバイト順。 */

mlkerr mPSDSave_writeImageRowCh(mPSDSave *p,uint8_t *buf)
{
	return _write_row_image(p, buf);
}

/**@ 一枚絵イメージの書き込み終了 */

mlkerr mPSDSave_endImage(mPSDSave *p)
{
	if(p->compress)
	{
		//圧縮サイズリスト
		
		if(fseeko(p->fp, p->fpos1, SEEK_SET)
			|| fwrite(p->encsizebuf, 1, p->size1, p->fp) != p->size1
			|| fseeko(p->fp, 0, SEEK_END))
			return MLKERR_IO;
	}

	return MLKERR_OK;
}


//=============================
// レイヤ拡張情報
//=============================


/* アイテム解放 */

static void _infoexitem_destroy(mList *list,mListItem *item)
{
	mFree(((mPSDLayerInfoExItem *)item)->buf);
}

/* リスト追加 */

static mPSDLayerInfoExItem *_add_infoex_item(mList *list,uint32_t key,uint32_t size)
{
	mPSDLayerInfoExItem *pi;

	pi = (mPSDLayerInfoExItem *)mListAppendNew(list, sizeof(mPSDLayerInfoExItem));
	if(!pi) return NULL;

	pi->key = key;
	pi->size = size;

	pi->buf = (uint8_t *)mMalloc(size);
	if(!pi->buf)
	{
		mListDelete(list, MLISTITEM(pi));
		return NULL;
	}

	return pi;
}

/* Unicode レイヤ名追加 */

static mlkbool _exinfo_add_unicode_name(mPSDSave *p,mList *list,const char *name)
{
	mPSDLayerInfoExItem *pi;
	uint8_t *buf;
	int len,size;

	if(!name) return TRUE;

	buf = p->outbuf;

	//UTF-16 BE 変換

	len = mUTF8toUTF16(name, -1, (mlkuchar16 *)(buf + 4), (_OUTBUFSIZE - 4) / 2);
	if(len < 0) return TRUE;

#if !defined(MLK_BIG_ENDIAN)
	mSwapByte_16bit(buf + 4, len);
#endif

	//サイズ (4byte 境界)

	size = 4 + ((len + 1) & (~1)) * 2;

	//余白

	if(len & 1)
		memset(buf + size - 2, 0, 2);

	//長さ

	mSetBufBE32(buf, len);

	//アイテム

	pi = _add_infoex_item(list, MLK_MAKE32_4('l','u','n','i'), size);
	if(!pi) return FALSE;

	memcpy(pi->buf, buf, size);

	return TRUE;
}


/**@ レイヤ拡張情報のリストを解放 */

void mPSDSave_exinfo_freeList(mList *list)
{
	list->item_destroy = _infoexitem_destroy;

	mListDeleteAll(list);
}

/**@ セクション情報追加
 *
 * @p:blendmode 負の値でなし */

mlkbool mPSDSave_exinfo_addSection(mList *list,int type,int blendmode)
{
	mPSDLayerInfoExItem *pi;
	int size;

	size = (blendmode < 0)? 4: 12;

	pi = _add_infoex_item(list, MLK_MAKE32_4('l','s','c','t'), size);
	if(!pi) return FALSE;

	mSetBufBE32(pi->buf, type);

	if(size >= 12)
	{
		mSetBuf_format(pi->buf + 4, ">si",
			"8BIM", blendmode);
	}
	
	return TRUE;
}


//=============================
// レイヤ
//=============================

/*
  fpos1 : PackBits 圧縮サイズリストの位置
  fpos2 : レイヤサイズの位置
  size1 : レイヤ全体の内部サイズ
*/


/* イメージの最大サイズを記録 */

static void _set_layerimage_maxsize(mPSDSave *p,mPSDLayer *info)
{
	int n1,n2;

	//最大幅

	n1 = info->box_img.w;
	n2 = info->box_mask.w;

	if(n1 < n2) n1 = n2;
	if(n1 > p->layerimg_maxw) p->layerimg_maxw = n1;

	//最大高さ

	n1 = info->box_img.h;
	n2 = info->box_mask.h;

	if(n1 < n2) n1 = n2;
	if(n1 > p->layerimg_maxh) p->layerimg_maxh = n1;
}

/* レイヤ情報書き込み: 基本データ */

static mlkerr _write_layerinfo_base(mPSDSave *p,_layerinfo *pi)
{
	mPSDLayer *info = &pi->i;
	mBox box;
	uint8_t *outbuf;
	int i,size;

	outbuf = p->outbuf;

	//

	box = info->box_img;

	outbuf += mSetBuf_format(outbuf,
		">iiiih",
		box.y, box.x,	//top, left
		box.y + box.h,	//bottom
		box.x + box.w,	//right
		info->chnum);	//チャンネル数

	//チャンネルデータ

	for(i = info->chnum; i > 0; i--)
	{
		mSetBuf_format(outbuf, ">hi", 0, 2);
		outbuf += 6;
	}

	//

	outbuf += mSetBuf_format(outbuf, ">sibbbb",
		"8BIM",
		info->blendmode,
		info->opacity,
		info->clipping,
		info->flags,
		0);

	//チャンネルデータの位置

	pi->fpos_chinfo = ftello(p->fp) + 18;

	//書き込み

	size = outbuf - p->outbuf;

	if(_put_outbuf(p, size))
		return MLKERR_IO;

	//

	p->size1 += size;

	return MLKERR_OK;
}

/* レイヤ情報書き込み: レイヤマスク */

static int _write_layerinfo_mask(mPSDSave *p,mPSDLayer *info,uint32_t *exsize)
{
	int size,top,left;

	if(!info->box_mask.w || !info->box_mask.h)
		//なし
		size = 0;
	else
	{
		left = info->box_mask.x;
		top = info->box_mask.y;

		if(info->mask_flags & MPSD_LAYER_MASK_FLAGS_RELATIVE)
		{
			left -= info->box_img.x;
			top -= info->box_img.y;
		}
	
		size = mSetBuf_format(p->outbuf + 4,
			">iiiibb2z",
			top, left,
			top + info->box_mask.h,  //bottom
			left + info->box_mask.w, //right
			info->mask_defcol,  //デフォルト値
			info->mask_flags);  //フラグ
	}

	//サイズ

	mSetBufBE32(p->outbuf, size);
	size += 4;

	//

	*exsize += size;

	return _put_outbuf(p, size);
}

/* レイヤ名 (パスカル文字列) 書き込み */

static int _write_layername_pascal(mPSDSave *p,const char *name,uint32_t *exsize)
{
	int len,size,pad;

	//255 文字に制限してコピー

	if(!name)
		len = 0;
	else
	{
		len = strlen(name);
		if(len > 255) len = 255;

		len = mUTF8CopyValidate((char *)p->outbuf + 1, name, len);
	}

	//長さ

	p->outbuf[0] = len;

	//サイズ (長さ含む 4byte 境界)

	size = (len + 4) & (~3);
	pad = size - (1 + len);

	//余白分は 0 に

	if(pad)
		memset(p->outbuf + 1 + len, 0, pad);

	//

	*exsize += size;

	return _put_outbuf(p, size);
}

/* レイヤ情報書き込み: 拡張データ */

static mlkerr _write_layerinfo_ex(mPSDSave *p,_layerinfo *pi)
{
	uint32_t exsize = 0;
	off_t pos_size;

	//サイズ (仮)

	pos_size = ftello(p->fp);

	if(mFILEwrite0(p->fp, 4))
		return MLKERR_IO;

	//レイヤマスク

	if(_write_layerinfo_mask(p, &pi->i, &exsize))
		return MLKERR_IO;

	//合成データ

	if(mFILEwrite0(p->fp, 4))
		return MLKERR_IO;

	exsize += 4;

	//レイヤ名 (パスカル文字列)

	if(_write_layername_pascal(p, pi->i.name, &exsize))
		return MLKERR_IO;

	//他拡張データ

	if(pi->exdata)
	{
		if(fwrite(pi->exdata, 1, pi->exdata_size, p->fp) != pi->exdata_size)
			return MLKERR_IO;
	
		exsize += pi->exdata_size;
	}

	//拡張データサイズ

	if(fseeko(p->fp, pos_size, SEEK_SET)
		|| mFILEwriteBE32(p->fp, exsize)
		|| fseek(p->fp, 0, SEEK_END))
		return MLKERR_IO;

	//

	p->size1 += 4 + exsize;

	return MLKERR_OK;
}


//================


/**@ 全レイヤイメージの最大サイズを取得 */

void mPSDSave_getLayerImageMaxSize(mPSDSave *p,mSize *size)
{
	size->w = p->layerimg_maxw;
	size->h = p->layerimg_maxh;
}

/**@ レイヤ情報の取得
 *
 * @d:レイヤ情報設定後、レイヤイメージ書き込み時に情報が欲しい時に使う。
 * @r:FALSE でレイヤ番号が範囲外 */

mlkbool mPSDSave_getLayerInfo(mPSDSave *p,int layerno,mPSDLayer *info)
{
	if(layerno < 0 || layerno >= p->layer_num)
		return FALSE;
	else
	{
		*info = p->layerinfo[layerno].i;
		return TRUE;
	}
}

/**@ レイヤなしを書き込む
 *
 * @d:レイヤを格納しない場合、リソース書き込み後に実行する。 */

mlkerr mPSDSave_writeLayerNone(mPSDSave *p)
{
	if(mFILEwrite0(p->fp, 4))
		return MLKERR_IO;
	else
		return MLKERR_OK;
}

/**@ レイヤの書き込みを開始
 *
 * @p:layernum レイヤ数 */

mlkerr mPSDSave_startLayer(mPSDSave *p,int layernum)
{
	p->layer_num = layernum;
	p->size1 = 2;

	//レイヤ情報のバッファ確保

	p->layerinfo = (_layerinfo *)mMalloc0(sizeof(_layerinfo) * layernum);
	if(!p->layerinfo) return MLKERR_ALLOC;

	p->curlayer = p->layerinfo;

	//書き込み
	//(4) 全体サイズ (4) 内部サイズ (2) レイヤ数

	p->fpos2 = ftello(p->fp);

	mSetBuf_format(p->outbuf, ">8zh", layernum);

	if(_put_outbuf(p, 10)) return MLKERR_IO;

	return MLKERR_OK;
}

/**@ レイヤ情報をセット
 *
 * @p:info  layerno にレイヤ番号を入れておく。\
 * \
 * レイヤマスクの範囲は、絶対値にする。\
 * RELATIVE のフラグが ON の場合は、相対値で保存される。
 * 
 * @p:exlist レイヤの拡張データのリスト。\
 * データがなくても常に指定すること。\
 * 内部でデータがコピーされるので、セット後は解放して良い。\
 * mPSDLoad で読み込んだリストをそのままセット可能。\
 * 'luni' は自動でセットされる。 */

mlkerr mPSDSave_setLayerInfo(mPSDSave *p,mPSDLayer *info,mList *exlist)
{
	_layerinfo *linfo;
	mPSDLayerInfoExItem *pi;
	uint8_t *exbuf;
	uint32_t size;

	linfo = p->layerinfo + info->layerno;

	//情報コピー

	linfo->i = *info;
	
	//イメージの最大サイズをセット

	_set_layerimage_maxsize(p, info);

	//----- 拡張データをまとめて一つに

	//Unicode レイヤ名を追加

	_exinfo_add_unicode_name(p, exlist, info->name);

	//全体のサイズを計算

	size = 0;

	MLK_LIST_FOR(*exlist, pi, mPSDLayerInfoExItem)
	{
		size += 12 + pi->size;
	}

	//データ

	if(size)
	{
		//確保

		linfo->exdata = (uint8_t *)mMalloc(size);
		if(!linfo->exdata) return MLKERR_ALLOC;

		linfo->exdata_size = size;

		//exdata にまとめてセット

		exbuf = linfo->exdata;

		MLK_LIST_FOR(*exlist, pi, mPSDLayerInfoExItem)
		{
			mSetBuf_format(exbuf, ">sii",
				"8BIM", pi->key, pi->size);

			memcpy(exbuf + 12, pi->buf, pi->size);

			exbuf += 12 + pi->size;
		}
	}

	return MLKERR_OK;
}

/**@ すべてのレイヤ情報を書き込み
 *
 * @d:startLayer → setLayerInfo → writeLayerInfo の順で行う。 */

mlkerr mPSDSave_writeLayerInfo(mPSDSave *p)
{
	_layerinfo *pi;
	int ret,il;

	pi = p->layerinfo;

	for(il = p->layer_num; il > 0; il--, pi++)
	{
		//基本データ

		ret = _write_layerinfo_base(p, pi);
		if(ret) return ret;

		//拡張データ

		ret = _write_layerinfo_ex(p, pi);
		if(ret) return ret;

		//解放

		mFree(pi->exdata);
		pi->exdata = NULL;
	}

	return MLKERR_OK;
}

/**@ レイヤの各イメージの書き込みを開始
 *
 * @d:writeLayerInfo 後に行う。\
 * レイヤ番号 0 の下層のレイヤから順に、各レイヤごとに実行する。\
 * \
 * ※幅・高さが 0 でも、各チャンネルの圧縮タイプの 2byte の書き込みが必要になるので、
 * 実行すること (この場合、圧縮タイプは常に非圧縮として扱われる)。 */

mlkerr mPSDSave_startLayerImage(mPSDSave *p)
{
	p->cur_chno = 0;

	return MLKERR_OK;
}

/**@ レイヤの各チャンネルの書き込みを開始 */

mlkerr mPSDSave_startLayerImageCh(mPSDSave *p,int chid)
{
	p->cur_chid = chid;
	p->cur_line = 0;

	if(chid == MPSD_CHID_MASK)
		p->curimgbox = &p->curlayer->i.box_mask;
	else
		p->curimgbox = &p->curlayer->i.box_img;

	//エンコード準備

	return _setup_image_write(p, p->curimgbox->w, p->curimgbox->h, TRUE);
}

/**@ レイヤイメージの各チャンネルのY1行イメージを書き込み
 *
 * @d:16bit 時はホストのバイト順。\
 *  幅・高さが 0 の場合は必要ない。 */

mlkerr mPSDSave_writeLayerImageRowCh(mPSDSave *p,uint8_t *buf)
{
	return _write_row_image(p, buf);
}

/**@ レイヤイメージの各チャンネルの書き込みを終了 */

mlkerr mPSDSave_endLayerImageCh(mPSDSave *p)
{
	int size;

	//チャンネルデータ書き換え

	mSetBuf_format(p->outbuf, ">hi",
		p->cur_chid, p->imgencsize);

	if(fseeko(p->fp, p->curlayer->fpos_chinfo + p->cur_chno * 6, SEEK_SET)
		|| _put_outbuf(p, 6))
		return MLKERR_IO;

	//PackBits 圧縮サイズリスト

	if(p->compress && p->curimgbox->w && p->curimgbox->h)
	{
		size = p->curimgbox->h * 2;
	
		if(fseeko(p->fp, p->fpos1, SEEK_SET)
			|| fwrite(p->encsizebuf, 1, size, p->fp) != size)
			return MLKERR_IO;
	}

	if(fseek(p->fp, 0, SEEK_END))
		return MLKERR_IO;

	p->cur_chno++;
	p->size1 += p->imgencsize;

	return MLKERR_OK;
}

/**@ 各レイヤのイメージの書き込み終了 */

void mPSDSave_endLayerImage(mPSDSave *p)
{
	p->cur_layerno++;
	p->curlayer++;
}

/**@ レイヤの書き込み終了 */

mlkerr mPSDSave_endLayer(mPSDSave *p)
{
	uint32_t size = p->size1;

	//余白

	if(size & 1)
	{
		if(mFILEwrite0(p->fp, 1))
			return MLKERR_IO;
		
		size++;
	}

	//レイヤサイズ

	mSetBuf_format(p->outbuf, ">ii", size + 4, size);

	if(fseeko(p->fp, p->fpos2, SEEK_SET)
		|| _put_outbuf(p, 8)
		|| fseek(p->fp, 0, SEEK_END))
		return MLKERR_IO;

	return MLKERR_OK;
}
