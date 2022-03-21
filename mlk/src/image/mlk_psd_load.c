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
 * PSD 読み込み
 *****************************************/

#include <stdio.h>
#include <string.h>

#include "mlk.h"
#include "mlk_rectbox.h"
#include "mlk_list.h"
#include "mlk_buf.h"
#include "mlk_stdio.h"
#include "mlk_util.h"
#include "mlk_packbits.h"
#include "mlk_unicode.h"

#include "mlk_psd.h"


//---------------

#define _PUT_DEBUG  0

//レイヤチャンネル最大数
#define _CHANNEL_MAXNUM  7

#define _INBUFSIZE  (8 * 1024)

//---------------

/* リソースリストアイテム */

typedef struct
{
	mListItem i;
	uint16_t id;
	uint32_t size;	//0 の場合あり
	off_t fpos;
}_resitem;

/* レイヤ情報 */

typedef struct
{
	mPSDLayer i;

	uint16_t ch_id[_CHANNEL_MAXNUM];	//各チャンネルID
	uint32_t ch_size[_CHANNEL_MAXNUM],	//各チャンネルデータサイズ
		exinfo_size;		//拡張データのサイズ
	int32_t name_bufpos;	//名前のデータ位置 (-1 でなし)
	off_t ch_imgpos[_CHANNEL_MAXNUM],	//各チャンネルのデータ位置
		exinfo_pos;		//拡張データの位置
}_layerinfo;

//---------------

struct _mPSDLoad
{
	mList list_res;		//リソースデータのリスト
	mBuf strbuf;		//文字列バッファ
	mPackBits packbits;	//PackBits 圧縮

	FILE *fp;

	_layerinfo *layerinfo,	//各レイヤ情報バッファ
		*curlayerinfo;

	uint8_t *inbuf;			//入力作業用 & PackBits 展開用バッファ
	uint16_t *encsizebuf;	//PackBits 圧縮時の圧縮サイズリストバッファ

	off_t fpos_resource,	//リソースデータの先頭位置
		fpos_layertop,		//レイヤデータの先頭位置
		fpos_imgch[5];		//イメージの各チャンネルのデータ位置

	uint32_t width,
		height,
		size_resource,	//リソースデータサイズ
		size_layer;		//レイヤデータサイズ

	int fp_open,		//ファイルから開いた fp か
		layer_num,		//レイヤ数
		compress,		//圧縮タイプ (0:無圧縮、1:PackBits)
		rawlinesize,	//生のラインデータサイズ
		cur_line,		//現在のライン位置
		layerimg_maxw,	//レイヤイメージの最大幅
		layerimg_maxh;	//レイヤイメージの最大高さ

	/* レイヤマスク読み込み用 */
	int	cur_masky,
		layermask_expand_left,
		layermask_expand_right,
		layermask_mask_w;

	uint16_t img_channels,	//一枚絵のチャンネル数
		bits,
		colmode;
};

//---------------

/* 圧縮タイプ */

enum
{
	_COMPRESS_NONE = 0,
	_COMPRESS_PACKBITS = 1,
	_COMPRESS_MAX = 2
};

//---------------


//=========================
// sub
//=========================


/* 幅から 1ch 分のY1行データサイズ取得 */

static int _get_ch_rowsize(mPSDLoad *p,int width)
{
	if(p->bits == 1)
		width = (width + 7) >> 3;
	else if(p->bits == 16)
		width <<= 1;

	return width;
}

/* inbuf に指定サイズ分読み込み
 *
 * return: 0 で成功、1 でエラー */

static int _read_inbuf(mPSDLoad *p,uint32_t size)
{
	if(size > _INBUFSIZE)
		return 1;
	else
		return (fread(p->inbuf, 1, size, p->fp) != size);
}

/* 現在位置からシーク
 *
 * return: 0 で成功、それ以外でエラー */

static int _seek_cur(mPSDLoad *p,uint32_t size)
{
	if(size == 0)
		return 0;
	else
		return fseeko(p->fp, size, SEEK_CUR);
}

/* 先頭位置からシーク */

static int _seek_set(mPSDLoad *p,off_t pos)
{
	return fseeko(p->fp, pos, SEEK_SET);
}

/* パスカル文字列をスキップ (2byte 境界)
 *
 * bytes: NULL 以外で、スキップしたバイト数 */

static int _skip_pascal_string(mPSDLoad *p,int *bytes)
{
	uint8_t len;
	int size;

	if(fread(&len, 1, 1, p->fp) != 1)
		return 1;

	//長さ含む全体のサイズ (2byte 境界)

	size = (len + 2) & (~1);
	if(bytes) *bytes = size;

	return _seek_cur(p, size - 1);
}

/* パスカル文字列を inbuf に読み込み (4byte 境界)
 *
 * 終端はヌル文字が追加される。
 * UTF-8 として扱う。
 *
 * bytes: NULL 以外で、読み込んだバイト数 */

static int _read_pascal_string(mPSDLoad *p,int *bytes)
{
	uint8_t len;
	int size;

	//長さ

	if(fread(&len, 1, 1, p->fp) == 0)
		return 1;

	//長さ含む全体のサイズ (4byte 境界)

	size = (len + 4) & (~3);
	if(bytes) *bytes = size;

	//文字列 (+余白)

	size--;

	if(fread(p->inbuf, 1, size, p->fp) != size)
		return 1;

	//ヌル文字追加

	p->inbuf[len] = 0;

	//UTF-8 検証

	mUTF8Validate((char *)p->inbuf, len);

	return 0;
}

/* バッファから UTF-16 BE 文字列を読み込み、
 * strbuf に UTF-8 文字列を追加。
 *
 * size: データの残りサイズ
 * return: 追加した文字列の位置 (-1 で空文字列) */

static int32_t _add_unicode_string(mPSDLoad *p,uint8_t *buf,uint32_t size)
{
	char *str;
	uint32_t len;
	int32_t retpos;
	int dlen;

	//文字数

	if(size < 4) return -1;

	len = mGetBufBE32(buf);
	if(len == 0) return -1;

	buf += 4;
	size -= 4;

	//文字列
	
	if(size < len * 2) return -1;

#if !defined(MLK_BIG_ENDIAN)
	mSwapByte_16bit(buf, len);
#endif

	str = mUTF16toUTF8_alloc((mlkuchar16 *)buf, len, &dlen);
	if(!str) return -1;

	retpos = mBufAppendUTF8(&p->strbuf, str, dlen);

	mFree(str);

	return retpos;
}

/* リソースリストから ID を検索
 *
 * seek: TRUE でシークする。
 * シークに失敗した場合は NULL が返る。 */

static _resitem *_search_resource(mPSDLoad *p,uint16_t id,mlkbool seek)
{
	_resitem *pi;

	MLK_LIST_FOR(p->list_res, pi, _resitem)
	{
		if(pi->id == id)
		{
			if(seek)
			{
				if(_seek_set(p, pi->fpos))
					return NULL;
			}
			
			return pi;
		}
	}

	return NULL;
}


//=========================
// sub - image
//=========================


/* PackBits 入力関数 */

static mlkerr _packbits_read(mPackBits *p,uint8_t *buf,int size)
{
	if(fread(buf, 1, size, (FILE *)p->param) != size)
		return MLKERR_IO;

	return MLKERR_OK;
}

/* イメージ展開用のバッファ解放 */

static void _free_imagebuf(mPSDLoad *p)
{
	mFree(p->encsizebuf);
	p->encsizebuf = NULL;
}

/* イメージ展開用のバッファ作成
 *
 * encsize: PackBits 圧縮サイズバッファの確保サイズ (0 でなし) */

static int _alloc_imagebuf(mPSDLoad *p,int encsize)
{
	//解放

	_free_imagebuf(p);

	//PackBits 圧縮サイズリスト

	if(encsize)
	{
		p->encsizebuf = (uint16_t *)mMalloc(encsize);
		if(!p->encsizebuf) return 1;
	}

	return 0;
}

/* イメージ読み込みの準備
 *
 * 圧縮タイプ・圧縮サイズリスト読み込み
 *
 * height: レイヤ時、イメージの高さ */

static int _setup_image_read(mPSDLoad *p,int width,int height,mlkbool is_layer)
{
	uint16_t comp;
	int size,ret;

	//圧縮タイプ

	if(mFILEreadBE16(p->fp, &comp))
		return MLKERR_DAMAGED;

	if(comp >= _COMPRESS_MAX)
		return MLKERR_UNSUPPORTED;

	p->compress = comp;

	//Y1行サイズ

	p->rawlinesize = _get_ch_rowsize(p, width);

	//PackBits 圧縮時

	if(comp == _COMPRESS_PACKBITS)
	{
		//バッファ確保

		if(is_layer)
		{
			size = height * 2;

			if(p->encsizebuf)
				ret = 0;
			else
				ret = _alloc_imagebuf(p, p->layerimg_maxh * 2);
		}
		else
		{
			size = p->height * p->img_channels * 2;
			ret = _alloc_imagebuf(p, size);
		}

		if(ret) return MLKERR_ALLOC;

		//圧縮サイズリスト読み込み

		if(fread(p->encsizebuf, 1, size, p->fp) != size)
			return MLKERR_DAMAGED;

	#if !defined(MLK_BIG_ENDIAN)
		mSwapByte_16bit(p->encsizebuf, size >> 1);
	#endif
	}

	return MLKERR_OK;
}

/* 一枚絵イメージ時、各チャンネルのデータ位置セット */

static void _set_chpos_image(mPSDLoad *p)
{
	int i,j,chnum;
	uint32_t size;
	uint16_t *ps;
	off_t pos;

	chnum = p->img_channels;
	if(chnum > 5) chnum = 5;

	pos = ftello(p->fp);

	if(p->compress == _COMPRESS_NONE)
	{
		//無圧縮
		
		size = p->rawlinesize * p->height;
	
		for(i = 0; i < chnum; i++)
		{
			p->fpos_imgch[i] = pos;
			pos += size;
		}
	}
	else
	{
		//PackBits

		ps = p->encsizebuf;
		
		p->fpos_imgch[0] = pos;
		
		for(i = 1; i < chnum; i++)
		{
			for(j = p->height; j > 0; j--, ps++)
				pos += *ps;

			p->fpos_imgch[i] = pos;
		}
	}
}

/* Y1行生イメージを読み込み
 *
 * rawlinesize が生データサイズ */

static int _read_row_image(mPSDLoad *p,uint8_t *buf)
{
	mPackBits *pb;
	int rawsize,ret;

	rawsize = p->rawlinesize;

	if(p->compress == _COMPRESS_NONE)
	{
		//無圧縮

		if(fread(buf, 1, rawsize, p->fp) != rawsize)
			return MLKERR_DAMAGED;
	}
	else
	{
		//PackBits

		pb = &p->packbits;

		pb->buf = buf;
		pb->bufsize = rawsize;
		pb->encsize = p->encsizebuf[p->cur_line];
		pb->param = p->fp;

		ret = mPackBits_decode(pb);
		if(ret) return ret;
	}

	//16bit バイト入れ替え

#if !defined(MLK_BIG_ENDIAN)
	if(p->bits == 16)
		mSwapByte_16bit(buf, rawsize >> 1);
#endif

	//

	p->cur_line++;

	return MLKERR_OK;
}

/* レイヤマスクをイメージの範囲に合わせる場合の情報セット
 *  (各レイヤのイメージ読み込み開始時) */

static void _set_layermask_info(mPSDLoad *p,_layerinfo *pi)
{
	mRect rcimg,rcmask,rc;

	mRectSetBox(&rcimg, &pi->i.box_img);
	mRectSetBox(&rcmask, &pi->i.box_mask);

	//マスクの範囲をイメージの範囲内にクリッピング

	rc = rcmask;

	if(!mRectClipRect(&rc, &rcimg))
		//マスクがイメージの範囲内にない
		p->layermask_mask_w = 0;
	else
	{
		p->layermask_mask_w = rc.x2 - rc.x1 + 1;
		p->layermask_expand_left = rcmask.x1 - rcimg.x1;
		p->layermask_expand_right = rcimg.x2 - rc.x2;
	}

	p->cur_masky = pi->i.box_img.y;
}


//=========================
// sub - レイヤ情報
//=========================


/* レイヤイメージの範囲セット (イメージ or マスク) */

static void _set_layerimage_area(mPSDLoad *p,mBox *dst,
	int left,int top,int right,int bottom)
{
	int w,h;

	w = right - left;
	h = bottom - top;
	if(w < 0) w = 0;
	if(h < 0) h = 0;

	dst->x = left;
	dst->y = top;
	dst->w = w;
	dst->h = h;

	//全レイヤの最大幅、高さ

	if(p->layerimg_maxw < w) p->layerimg_maxw = w;
	if(p->layerimg_maxh < h) p->layerimg_maxh = h;
}

/* レイヤマスク情報データ読み込み */

static int _read_layermaskinfo(mPSDLoad *p,_layerinfo *pi,uint32_t masksize)
{
	int32_t top,left,right,bottom;
	uint8_t def,flags;

	if(masksize == 0)
		return 0;
	else if(masksize < 20 || masksize > _INBUFSIZE)
		//スキップ
		return _seek_cur(p, masksize);
	else
	{
		//読み込み
		
		if(_read_inbuf(p, masksize)) return 1;

		mGetBuf_format(p->inbuf, ">iiiibb",
			&top, &left, &bottom, &right, &def, &flags);

		//セット

		_set_layerimage_area(p, &pi->i.box_mask,
			left, top, right, bottom);

		pi->i.mask_defcol = def;
		pi->i.mask_flags = flags;

		//相対位置

		if(flags & MPSD_LAYER_MASK_FLAGS_RELATIVE)
		{
			pi->i.box_mask.x += pi->i.box_img.x;
			pi->i.box_mask.y += pi->i.box_img.y;
		}

	#if _PUT_DEBUG
		mDebug("[mask] (%d, %d)-(%d, %d) (%d x %d), def:%d, flags:%d\n",
			left, top, right, bottom, right - left, bottom - top,
			def, flags);
	#endif
	}
		
	return 0;
}

/* レイヤ拡張データ (8BIM + key) 読み込み
 *
 * デフォルトで必要なもののみ読み込み。 */

static int _read_layerinfo_ex_keys(mPSDLoad *p,_layerinfo *pi,uint32_t datsize)
{
	uint8_t *allocbuf = NULL,*ps;
	uint32_t sig,key,size;
	int ret;

	//入力バッファ

	if(datsize <= _INBUFSIZE)
		ps = p->inbuf;
	else
	{
		ps = allocbuf = (uint8_t *)mMalloc(datsize);
		if(!allocbuf) return MLKERR_ALLOC;
	}

	//読み込み

	if(fread(ps, 1, datsize, p->fp) != datsize)
	{
		mFree(allocbuf);
		return MLKERR_DAMAGED;
	}

	//--------

	ret = MLKERR_OK;

	while(datsize >= 12)
	{
		mGetBuf_format(ps, ">iii", &sig, &key, &size);

		ps += 12;
		datsize -= 12;

		if(sig != MLK_MAKE32_4('8','B','I','M'))
			break;

		if(key == MLK_MAKE32_4('l','u','n','i'))
		{
			//Unicode レイヤ名

			pi->name_bufpos = _add_unicode_string(p, ps, datsize);
			break;
		}

		ps += size;
		datsize -= size;
	}

	mFree(allocbuf);

	return ret;
}

/* レイヤ拡張データ読み込み */

static int _read_layerinfo_ex(mPSDLoad *p,_layerinfo *pi,uint32_t exsize)
{
	char *name;
	uint32_t size;
	int bytes,ret;

	pi->name_bufpos = -1;

	//レイヤマスクデータ

	if(mFILEreadBE32(p->fp, &size)
		|| _read_layermaskinfo(p, pi, size))
		return MLKERR_DAMAGED;

	exsize -= 4 + size;

	//合成モードデータ

	if(mFILEreadBE32(p->fp, &size)
		|| _seek_cur(p, size))
		return MLKERR_DAMAGED;

	exsize -= 4 + size;

	//レイヤ名 (4byte 境界)
	/* 以下の拡張データで Unicode のレイヤ名が格納されている
	 * 場合があるため、保留しておく */

	if(_read_pascal_string(p, &bytes))
		return MLKERR_DAMAGED;

	if(p->inbuf[0])
		name = mStrdup((char *)p->inbuf);
	else
		name = NULL;

	exsize -= bytes;

	//残り

	if(exsize)
	{
		pi->exinfo_pos = ftello(p->fp);
		pi->exinfo_size = exsize;

		ret = _read_layerinfo_ex_keys(p, pi, exsize);
		if(ret)
		{
			mFree(name);
			return ret;
		}
	}

	//レイヤ名がセットされていない場合、
	//パスカル文字列の名前をセット

	if(pi->name_bufpos == -1 && name)
		pi->name_bufpos = mBufAppendUTF8(&p->strbuf, name, -1);

	mFree(name);

	return MLKERR_OK;
}

/* レイヤ情報読み込み */

static int _read_layerinfo(mPSDLoad *p,_layerinfo *pi)
{
	uint8_t *ps;
	int32_t top,left,bottom,right,size;
	uint16_t chnum,chid;
	int i,ret;

	//範囲とチャンネル数

	if(_read_inbuf(p, 18))
		return MLKERR_DAMAGED;

	mGetBuf_format(p->inbuf, ">iiiih",
		&top, &left, &bottom, &right, &chnum);

	//範囲セット

	_set_layerimage_area(p, &pi->i.box_img,
		left, top, right, bottom);

	//チャンネル数

	if(chnum > _CHANNEL_MAXNUM)
		return MLKERR_UNSUPPORTED;

	pi->i.chnum = chnum;

#if _PUT_DEBUG
	mDebug("(%d, %d)-(%d, %d) (%d x %d)\n"
		"chnum: %d\n",
		left, top, right, bottom, right - left, bottom - top,
		chnum);
#endif

	//チャンネルデータ〜拡張データサイズ まで読み込み

	size = chnum * 6 + 16;

	if(_read_inbuf(p, size))
		return MLKERR_DAMAGED;

	ps = p->inbuf;

	//各チャンネルの ID とデータサイズ

	for(i = 0; i < chnum; i++, ps += 6)
	{
		chid = mGetBufBE16(ps);
		
		pi->ch_id[i] = chid;
		pi->ch_size[i] = mGetBufBE32(ps + 2);

		if(chid == MPSD_CHID_ALPHA)
			pi->i.ch_flags |= MPSD_LAYER_CHFLAGS_ALPHA;
		else if(chid == MPSD_CHID_MASK)
			pi->i.ch_flags |= MPSD_LAYER_CHFLAGS_MASK;
		else if(chid < 16)
			pi->i.ch_flags |= 1 << chid;

	#if _PUT_DEBUG
		mDebug("<ch %d> id:%d, size:%u\n", i, chid, pi->ch_size[i]);
	#endif
	}

	//"8BIM"

	if(ps[0] != '8' || ps[1] != 'B' || ps[2] != 'I' || ps[3] != 'M')
		return MLKERR_INVALID_VALUE;

	//合成モード

	pi->i.blendmode = mGetBufBE32(ps + 4);

	//不透明度

	pi->i.opacity = ps[8];

	//クリッピング

	pi->i.clipping = ps[9];

	//フラグ

	pi->i.flags = ps[10];

#if _PUT_DEBUG
	i = pi->i.blendmode;
	mDebug("blendmode:'%c%c%c%c', opacity:%d, clipping:%d, flags:0x%X\n",
		(uint8_t)(i >> 24), (uint8_t)(i >> 16), (uint8_t)(i >> 8), (uint8_t)i,
		pi->i.opacity, pi->i.clipping, pi->i.flags);
#endif

	//拡張データサイズ

	size = mGetBufBE32(ps + 12);

	//拡張データ

	if(size)
	{
		ret = _read_layerinfo_ex(p, pi, size);
		if(ret) return ret;
	}

	//名前

	if(pi->name_bufpos != -1)
		pi->i.name = (const char *)p->strbuf.buf + pi->name_bufpos;

#if _PUT_DEBUG
	mDebug("name: \"%s\"\n", pi->i.name);
#endif

	return MLKERR_OK;
}


//==============================
// sub - レイヤ情報拡張データ
//==============================


/* 拡張データアイテム解放 */

static void _infoexitem_destroy(mList *list,mListItem *item)
{
	mFree(((mPSDLayerInfoExItem *)item)->buf);
}

/* 拡張データアイテム追加 */

static mlkbool _add_infoex_item(mPSDLoad *p,mList *list,
	uint32_t key,uint32_t size,uint8_t *buf)
{
	mPSDLayerInfoExItem *pi;

	pi = (mPSDLayerInfoExItem *)mListAppendNew(list, sizeof(mPSDLayerInfoExItem));
	if(!pi) return FALSE;

	pi->key = key;
	pi->size = size;

	if(size)
	{
		pi->buf = (uint8_t *)mMalloc(size);
		if(!pi->buf)
		{
			mListDelete(list, MLISTITEM(pi));
			return FALSE;
		}

		memcpy(pi->buf, buf, size);
	}

	return TRUE;
}

/* レイヤ拡張データをリストに読み込み */

static int _read_layerinfoex_list(mPSDLoad *p,_layerinfo *pi,mList *list)
{
	uint8_t *allocbuf = NULL,*ps;
	uint32_t sig,key,size,datsize;
	int ret;

	list->item_destroy = _infoexitem_destroy;

	datsize = pi->exinfo_size;

	//入力バッファ

	if(datsize <= _INBUFSIZE)
		ps = p->inbuf;
	else
	{
		ps = allocbuf = (uint8_t *)mMalloc(datsize);
		if(!allocbuf) return MLKERR_ALLOC;
	}

	//読み込み

	if(fread(ps, 1, datsize, p->fp) != datsize)
	{
		mFree(allocbuf);
		return MLKERR_DAMAGED;
	}

	//--------

#if _PUT_DEBUG
	mDebug("+++ layer [%d] info ex +++\n", pi->i.layerno);
#endif

	ret = MLKERR_OK;

	while(datsize >= 12)
	{
		mGetBuf_format(ps, ">iii", &sig, &key, &size);

		ps += 12;
		datsize -= 12;

		if(sig != MLK_MAKE32_4('8','B','I','M'))
			break;

	#if _PUT_DEBUG
		mDebug("\"%c%c%c%c\", size:%u\n",
			key >> 24, (uint8_t)(key >> 16), (uint8_t)(key >> 8), (uint8_t)key,
			size);
	#endif

		//"luni" は除外

		if(key != MLK_MAKE32_4('l','u','n','i'))
		{
			if(datsize < size)
			{
				ret = MLKERR_DAMAGED;
				break;
			}
		
			if(!_add_infoex_item(p, list, key, size, ps))
			{
				ret = MLKERR_ALLOC;
				break;
			}
		}

		ps += size;
		datsize -= size;
	}

	mFree(allocbuf);

	return ret;
}

/* 拡張データのリストから検索 */

static mPSDLayerInfoExItem *_search_infoexlist(mList *list,uint32_t key)
{
	mPSDLayerInfoExItem *pi;

	MLK_LIST_FOR(*list, pi, mPSDLayerInfoExItem)
	{
		if(pi->key == key)
			return pi;
	}

	return NULL;
}


//=========================
// sub - etc
//=========================


/* レイヤデータの先頭を読み込み */

static int _read_layer_top(mPSDLoad *p)
{
	uint32_t size_in;
	int16_t num;

	//[!] サイズが 0 でレイヤなし

	//サイズ

	if(mFILEreadBE32(p->fp, &p->size_layer))
		return MLKERR_DAMAGED;

	p->fpos_layertop = ftello(p->fp);

	if(p->size_layer == 0) return MLKERR_OK;

	//内側のサイズ

	if(mFILEreadBE32(p->fp, &size_in))
		return MLKERR_DAMAGED;

	if(size_in == 0) return MLKERR_OK;

	//レイヤ数 (負の場合あり)

	if(mFILEreadBE16(p->fp, &num))
		return MLKERR_DAMAGED;

	if(num < 0) num = -num;

	p->layer_num = num;

	return MLKERR_OK;
}

/* 一枚絵イメージのチャンネル数をチェック
 *
 * return: 0 以外で OK */

static int _check_img_chnum(mPSDLoad *p)
{
	int num = p->img_channels;

	switch(p->colmode)
	{
		case MPSD_COLMODE_MONO:
			return (num == 1);
		case MPSD_COLMODE_GRAYSCALE:
			return (num == 1 || num == 2);
		case MPSD_COLMODE_RGB:
			return (num == 3 || num == 4);
		case MPSD_COLMODE_CMYK:
			return (num == 4 || num == 5);
	}

	return 0;
}


//=========================
// main
//=========================


/**@ 閉じる */

void mPSDLoad_close(mPSDLoad *p)
{
	if(p)
	{
		if(p->fp && p->fp_open)
			fclose(p->fp);

		mListDeleteAll(&p->list_res);
		mBufFree(&p->strbuf);

		mFree(p->inbuf);
		mFree(p->layerinfo);
		mFree(p->encsizebuf);
		
		mFree(p);
	}
}

/**@ PSD 読み込み作成
 *
 * @g:読み込み */

mPSDLoad *mPSDLoad_new(void)
{
	mPSDLoad *p;

	p = (mPSDLoad *)mMalloc0(sizeof(mPSDLoad));
	if(!p) return NULL;

	//作業用バッファ

	p->inbuf = (uint8_t *)mMalloc(_INBUFSIZE);
	if(!p->inbuf)
	{
		mFree(p);
		return NULL;
	}

	//文字列バッファ

	if(!mBufAlloc(&p->strbuf, 1024, 1024))
	{
		mPSDLoad_close(p);
		return NULL;
	}

	//

	p->packbits.workbuf = p->inbuf;
	p->packbits.worksize = _INBUFSIZE;
	p->packbits.readwrite = _packbits_read;

	return p;
}

/**@ ファイルを開く
 *
 * @r:エラーコード */

mlkerr mPSDLoad_openFile(mPSDLoad *p,const char *filename)
{
	p->fp = mFILEopen(filename, "rb");
	if(!p->fp) return MLKERR_OPEN;

	p->fp_open = 1;

	return MLKERR_OK;
}

/**@ FILE * を開く
 *
 * @d:シークが出来る状態であることが必須。
 * @r:エラーコード (常に OK) */

mlkerr mPSDLoad_openFILEptr(mPSDLoad *p,void *fp)
{
	p->fp = (FILE *)fp;

	return MLKERR_OK;
}

/**@ ヘッダを読み込み
 *
 * @d:開いた後、常に最初に実行する。
 * @r:エラーコード */

mlkerr mPSDLoad_readHeader(mPSDLoad *p,mPSDHeader *dst)
{
	uint32_t sig,size;
	uint16_t ver;
	int n;

	//------- ヘッダ

	//読み込み

	if(_read_inbuf(p, 26))
		return MLKERR_DAMAGED;

	mGetBuf_format(p->inbuf, ">ih6Shiihh",
		&sig, &ver, &p->img_channels, &p->height, &p->width,
		&p->bits, &p->colmode);

	//識別子

	if(sig != MLK_MAKE32_4('8','B','P','S'))
		return MLKERR_FORMAT_HEADER;

	//バージョン

	if(ver != 1) return MLKERR_UNSUPPORTED;

	//セット

	dst->width = p->width;
	dst->height = p->height;
	dst->bits = p->bits;
	dst->colmode = p->colmode;
	dst->img_channels = p->img_channels;

	//幅・高さ

	if(p->width == 0 || p->width > MPSD_IMAGE_MAXSIZE
		|| p->height == 0 || p->height > MPSD_IMAGE_MAXSIZE)
		return MLKERR_INVALID_VALUE;

	//ビット数

	if(p->bits != 1 && p->bits != 8 && p->bits != 16)
		return MLKERR_UNSUPPORTED;

	//カラーモード

	n = p->colmode;

	if(n != MPSD_COLMODE_MONO
		&& n != MPSD_COLMODE_GRAYSCALE
		&& n != MPSD_COLMODE_RGB
		&& n != MPSD_COLMODE_CMYK)
		return MLKERR_UNSUPPORTED;

	//ビット数とカラーモード
	//(1bit は MONO のみ)

	if((p->bits == 1 && n != MPSD_COLMODE_MONO)
		|| (n == MPSD_COLMODE_MONO && p->bits != 1))
		return MLKERR_INVALID_VALUE;

	//一枚絵のチャンネル数

	if(!_check_img_chnum(p))
		return MLKERR_INVALID_VALUE;

#if _PUT_DEBUG
	mDebug("[PSD] bits:%d, colmode:%d, imgchnum:%d\n",
		p->bits, p->colmode, p->img_channels);
#endif

	//----- カラーモードデータ

	if(mFILEreadBE32(p->fp, &size)
		|| _seek_cur(p, size))
		return MLKERR_DAMAGED;

	//----- リソースデータ

	if(mFILEreadBE32(p->fp, &size))
		return MLKERR_DAMAGED;

	p->size_resource = size;
	p->fpos_resource = ftello(p->fp);

	if(_seek_cur(p, size))
		return MLKERR_DAMAGED;

	//----- レイヤデータ

	n = _read_layer_top(p);
	if(n) return n;

	dst->layer_num = p->layer_num;

	return MLKERR_OK;
}


//===========================
/* リソース */


/**@ リソースデータを読み込み
 *
 * @d:ヘッダの読み込み後なら、どこで行っても良い。\
 * リソースデータが必要なれければ、実行しなくて良い。
 * @r:エラーコード */

mlkerr mPSDLoad_readResource(mPSDLoad *p)
{
	FILE *fp = p->fp;
	_resitem *item;
	uint32_t sig,fullsize,size,size2;
	uint16_t id;
	int skip;

	fullsize = p->size_resource;

	if(_seek_set(p, p->fpos_resource))
		return MLKERR_DAMAGED;

#if _PUT_DEBUG
	mDebug("+++ resource (size:%u) +++\n", fullsize);
#endif

	//読み込み

	while(fullsize > 4)
	{
		if(mFILEreadBE32(fp, &sig)
			|| sig != MLK_MAKE32_4('8','B','I','M')
			|| mFILEreadBE16(fp, &id)
			|| _skip_pascal_string(p, &skip)
			|| mFILEreadBE32(fp, &size))
			return MLKERR_DAMAGED;

	#if _PUT_DEBUG
		mDebug("ID:0x%X, name:%d, size:%u\n", id, skip, size);
	#endif

		//2byte 境界のデータサイズ

		size2 = (size + 1) & (~1);

		//リストに追加

		item = (_resitem *)mListAppendNew(&p->list_res, sizeof(_resitem));
		if(!item) return MLKERR_ALLOC;

		item->id = id;
		item->size = size;
		item->fpos = ftello(fp);

		//データスキップ

		if(_seek_cur(p, size2))
			return MLKERR_DAMAGED;

		//

		fullsize -= 10 + skip + size2;
	}

	return MLKERR_OK;
}

/**@ リソースから、解像度を DPI で取得
 *
 * @r:取得できたか */

mlkbool mPSDLoad_res_getResoDPI(mPSDLoad *p,int *horz,int *vert)
{
	_resitem *pi;
	uint32_t h,v;
	uint16_t unih,univ;

	pi = _search_resource(p, MPSD_RESID_RESOLUTION, TRUE);

	if(!pi
		|| pi->size != 16
		|| _read_inbuf(p, 16))
		return FALSE;

	mGetBuf_format(p->inbuf, ">ih2Sih", &h, &unih, &v, &univ);

	//単位: 1=inch 2=cm

	if(unih == 2)
		h = (uint32_t)((double)h / (1<<16) * 2.54 + 0.5);
	else
		h >>= 16;

	if(univ == 2)
		v = (uint32_t)((double)v / (1<<16) * 2.54 + 0.5);
	else
		v >>= 16;

	*horz = h;
	*vert = v;

	return TRUE;
}

/**@ リソースから、カレントレイヤの番号を取得
 *
 * @r:-1 でデータがない。\
 * 0 以上でレイヤ番号 (一番下層のレイヤが 0 となる) */

int mPSDLoad_res_getCurLayerNo(mPSDLoad *p)
{
	_resitem *pi;
	uint16_t no;

	pi = _search_resource(p, MPSD_RESID_CURRENT_LAYER, TRUE);

	if(!pi || pi->size != 2 || mFILEreadBE16(p->fp, &no))
		return -1;
	else
		return no;
}

/**@ リソースから、ICC プロファイルデータ取得
 *
 * @p:psize データのサイズが格納される
 * @r:確保されたバッファ。NULL でデータがない or 失敗。*/

uint8_t *mPSDLoad_res_getICCProfile(mPSDLoad *p,uint32_t *psize)
{
	_resitem *pi;
	uint8_t *buf;

	pi = _search_resource(p, MPSD_RESID_ICC_PROFILE, TRUE);
	if(!pi || pi->size == 0) return NULL;

	*psize = pi->size;

	buf = (uint8_t *)mMalloc(pi->size);
	if(!buf) return NULL;

	if(fread(buf, 1, pi->size, p->fp) != pi->size)
	{
		mFree(buf);
		return NULL;
	}

	return buf;
}


//======================
/* 一枚絵イメージ
 *
 * cur_line: 0 〜 height x img_channels - 1
 * fpos_imgch: 各チャンネルのイメージデータ位置 */


/**@ 一枚絵イメージの 1ch 分のY1行サイズ取得 */

int mPSDLoad_getImageChRowSize(mPSDLoad *p)
{
	return _get_ch_rowsize(p, p->width);
}

/**@ 一枚絵イメージのチャンネル数取得 */

int mPSDLoad_getImageChNum(mPSDLoad *p)
{
	return p->img_channels;
}

/**@ 一枚絵イメージの読み込みを開始
 *
 * @d:ヘッダ読み込み後なら、どこで行っても良い。
 * @r:エラーコード */

mlkerr mPSDLoad_startImage(mPSDLoad *p)
{
	int ret;

	if(_seek_set(p, p->fpos_layertop + p->size_layer))
		return MLKERR_DAMAGED;

	//準備

	ret = _setup_image_read(p, p->width, p->height, FALSE);
	if(ret) return ret;

	//各チャンネルのデータ位置セット

	_set_chpos_image(p);

	//

	p->cur_line = 0;

	return MLKERR_OK;
}

/**@ 一枚絵イメージの読み込みチャンネルを指定
 *
 * @d:startImage 後に行う。\
 * 実行しなかった場合は、先頭チャンネルから順に読み込むことになる。
 * @p:ch 0 〜 img_channels - 1。\
 * RGBA なら、0:R, 1:G, 2:B, (3:A)。\
 * CMYK なら、0:C, 1:M, 2:Y, 3:K, (4:A) */

mlkerr mPSDLoad_setImageCh(mPSDLoad *p,int ch)
{
	if(ch >= p->img_channels)
		ch = p->img_channels - 1;

	p->cur_line = ch * p->height;

	if(_seek_set(p, p->fpos_imgch[ch]))
		return MLKERR_DAMAGED;
	else
		return MLKERR_OK;
}

/**@ 一枚絵イメージの現在位置のY1行イメージを読み込み
 *
 * @d:各チャンネルごとの生イメージを読み込む。\
 * バイトオーダーはホスト順。\
 * \
 * イメージデータは各チャンネルごとに分かれているため、
 * 「イメージ高さ x チャンネル分 (mPSDHeader::img_channels)」のラインデータがある。\
 * [ch 0: lineimg x height][ch 1: lineimg x height]...\
 * すべてのチャンネルを一続きにして、順番に読み込むことが可能。 */

mlkerr mPSDLoad_readImageRowCh(mPSDLoad *p,uint8_t *buf)
{
	 return _read_row_image(p, buf);
}


//=======================
// レイヤ
//=======================


/**@ 全レイヤのイメージの最大幅と高さを取得 */

void mPSDLoad_getLayerImageMaxSize(mPSDLoad *p,mSize *size)
{
	size->w = p->layerimg_maxw;
	size->h = p->layerimg_maxh;
}

/**@ レイヤイメージの最大幅のY1行バイトサイズを取得
 *
 * @d:1チャンネル分のサイズ */

int mPSDLoad_getLayerImageMaxRowSize(mPSDLoad *p)
{
	return _get_ch_rowsize(p, p->layerimg_maxw);
}

/**@ 指定レイヤの情報を取得
 *
 * @p:no レイヤ番号。一番下層のレイヤが 0。
 * @r:FALSE で no が範囲外 */

mlkbool mPSDLoad_getLayerInfo(mPSDLoad *p,int no,mPSDLayer *info)
{
	if(no < 0 || no >= p->layer_num)
		return FALSE;
	else
	{
		*info = p->layerinfo[no].i;
		return TRUE;
	}
}

/**@ 指定レイヤの情報を取得 (拡張データを含む)
 *
 * @p:list 現在の状態に追加されるため、初期化しておくこと。\
 *  取得時に item_destroy ハンドラがセットされるため、取得後は mListDeleteAll() で解放可能。 */

mlkerr mPSDLoad_getLayerInfoEx(mPSDLoad *p,int no,mPSDLayer *info,mList *list)
{
	_layerinfo *pi;
	off_t pos;
	int ret;

	pi = p->layerinfo + no;

	*info = pi->i;

	//

	pos = ftello(p->fp);

	if(fseeko(p->fp, pi->exinfo_pos, SEEK_SET))
		return MLKERR_IO;

	ret = _read_layerinfoex_list(p, pi, list);
	if(ret) return ret;

	fseeko(p->fp, pos, SEEK_SET);

	return MLKERR_OK;
}

/**@ レイヤ情報の param 値をセット */

void mPSDLoad_setLayerInfo_param(mPSDLoad *p,int no,void *param)
{
	if(no >= 0 && no < p->layer_num)
		p->layerinfo[no].i.param = param;
}

/**@ 拡張データからセレクションの情報取得 */

mlkbool mPSDLoad_exinfo_getSelection(mPSDLoad *p,mList *list,mPSDInfoSelection *info)
{
	mPSDLayerInfoExItem *pi;
	uint8_t *buf;

	info->type = MPSD_INFO_SELECTION_TYPE_NORMAL;
	info->blendmode = MPSD_BLENDMODE_NORMAL;

	pi = _search_infoexlist(list, MLK_MAKE32_4('l','s','c','t'));
	if(!pi) return FALSE;

	buf = pi->buf;

	//タイプ
	
	if(pi->size >= 4)
		info->type = mGetBufBE32(buf);

	//合成モード

	if(pi->size >= 12 && strncmp((char *)buf + 4, "8BIM", 4) == 0)
		info->blendmode = mGetBufBE32(buf + 8);

	return TRUE;
}


//======================


/**@ レイヤの読み込みを開始
 *
 * @d:ヘッダ読み込み後なら、どこで実行しても良い。\
 * すべてのレイヤ情報を読み込む。 */

mlkerr mPSDLoad_startLayer(mPSDLoad *p)
{
	_layerinfo *pi;
	int i,j,lnum,ret;
	off_t pos;

	lnum = p->layer_num;

	if(lnum == 0) return MLKERR_OK;

	//バッファ解放

	_free_imagebuf(p);

	//シーク

	if(_seek_set(p, p->fpos_layertop + 6))
		return MLKERR_DAMAGED;

	//レイヤ情報バッファ確保

	p->layerinfo = (_layerinfo *)mMalloc0(sizeof(_layerinfo) * lnum);
	if(!p->layerinfo) return MLKERR_ALLOC;

	//レイヤ情報読み込み

#if _PUT_DEBUG
	mDebug("+++ layer info +++\n");
#endif

	pi = p->layerinfo;

	for(i = 0; i < lnum; i++, pi++)
	{
	#if _PUT_DEBUG
		mDebug("=== layer [%d] ===\n", i);
	#endif
	
		ret = _read_layerinfo(p, pi);
		if(ret) return ret;

		pi->i.layerno = i;
	}

	//各レイヤ、各チャンネルのデータ位置セット

	pi = p->layerinfo;
	pos = ftello(p->fp);

	for(i = 0; i < lnum; i++, pi++)
	{
		for(j = 0; j < pi->i.chnum; j++)
		{
			pi->ch_imgpos[j] = pos;

			pos += pi->ch_size[j];
		}
	}

	return MLKERR_OK;
}

/* 指定チャンネルの読み込み開始
 *
 * chid: 負の値でチャンネル位置 (-1 = 0番) */

static mlkerr _set_layerimage_ch(mPSDLoad *p,int layerno,int chid,mPSDImageInfo *info)
{
	_layerinfo *pi;
	mBox *box;
	int is_mask,ret,chno;

	//レイヤ

	p->curlayerinfo = NULL;

	if(layerno >= p->layer_num) return -1;

	pi = p->layerinfo + layerno;

	//チャンネル

	if(chid < 0)
	{
		chno = -chid - 1;
		if(chno < 0 || chno >= pi->i.chnum) return -2;
		
		chid = pi->ch_id[chno];
	}
	else
	{
		//ID から検索
		
		for(chno = 0; chno < pi->i.chnum; chno++)
		{
			if(pi->ch_id[chno] == chid) break;
		}

		if(chno == pi->i.chnum) return -2;
	}

	//イメージ or マスク

	is_mask = (chid == MPSD_CHID_MASK);

	box = (is_mask)? &pi->i.box_mask: &pi->i.box_img;

	//シーク

	if(_seek_set(p, pi->ch_imgpos[chno]))
		return MLKERR_DAMAGED;

	//準備

	ret = _setup_image_read(p, box->w, box->h, TRUE);
	if(ret) return ret;

	//レイヤマスク読み込み用の情報

	if(is_mask)
		_set_layermask_info(p, pi);

	//情報

	if(info)
	{
		info->chid = chid;
		info->offx = box->x;
		info->offy = box->y;
		info->width = box->w;
		info->height = box->h;
		info->rowsize = _get_ch_rowsize(p, box->w);

		if(is_mask)
		{
			info->rowsize_mask = info->rowsize;
			info->rowsize = _get_ch_rowsize(p, pi->i.box_img.w);
		}
		else
			info->rowsize_mask = 0;
	}

	//

	p->curlayerinfo = pi;
	p->cur_line = 0;

	return MLKERR_OK;
}

/**@ 指定レイヤ＆チャンネルのイメージ読み込みを開始
 *
 * @p:chid チャンネルID
 * @p:info NULL 以外で、イメージの情報が格納される
 * @r:-1 = レイヤが範囲外、-2 = 指定チャンネルが存在しない */

mlkerr mPSDLoad_setLayerImageCh_id(mPSDLoad *p,int layerno,int chid,mPSDImageInfo *info)
{
	return _set_layerimage_ch(p, layerno, chid, info);
}

/**@ 指定レイヤ＆チャンネルのイメージ読み込みを開始
 *
 * @p:chno チャンネルのインデックス位置 (0〜)
 * @p:info NULL 以外で、イメージの情報が格納される
 * @r:-1 = レイヤが範囲外、-2 = 指定チャンネルが存在しない */

mlkerr mPSDLoad_setLayerImageCh_no(mPSDLoad *p,int layerno,int chno,mPSDImageInfo *info)
{
	return _set_layerimage_ch(p, layerno, -(chno + 1), info);
}

/**@ レイヤイメージの各チャンネルのY1行イメージを読み込み */

mlkerr mPSDLoad_readLayerImageRowCh(mPSDLoad *p,uint8_t *buf)
{
	 return _read_row_image(p, buf);
}

/**@ レイヤマスクイメージのY1行データを読み込み
 *
 * @d:マスクのイメージ範囲は、イメージと同じ範囲であるものとして処理され、
 * 結果のイメージは、反転やデフォルト色などが適用済みの状態となる。\
 * 取得したマスクデータは、イメージのアルファ値に適用して、
 * アルファ値をマスク値で制限する。\
 * \
 * mPSDImageInfo の値は、マスクの元範囲がセットされている。\
 * rowsize はイメージ幅でのサイズ、rowsize_mask はマスクの元幅でのサイズが入っている。
 *
 * @p:buf マスクの幅とイメージの幅で、大きい方のサイズ分のバッファが必要となる。 */

mlkerr mPSDLoad_readLayerMaskImageRow(mPSDLoad *p,uint8_t *buf)
{
	_layerinfo *pi;
	mBox boxmask;
	int imgw,maskw,ret,n,left,right;
	uint8_t defcol,reverse;

	pi = p->curlayerinfo;

	boxmask = pi->i.box_mask;

	n = (p->bits == 16)? 2: 1;

	imgw = pi->i.box_img.w * n;
	maskw = p->layermask_mask_w * n;
	left = p->layermask_expand_left * n;
	right = p->layermask_expand_right * n;

	defcol = pi->i.mask_defcol;
	reverse = (pi->i.mask_flags & MPSD_LAYER_MASK_FLAGS_REVERSE);

	//イメージの Y 位置がマスクデータの範囲外なら、デフォルト色で埋める
	//(マスクよりイメージの方が大きい場合)

	if(p->cur_masky < boxmask.y
		|| p->cur_masky >= boxmask.y + boxmask.h)
	{
		memset(buf, (reverse)? 255 - defcol: defcol, imgw);
		p->cur_masky++;
		
		return MLKERR_OK;
	}

	p->cur_masky++;

	//マスクの上端がイメージより上の場合、
	//イメージ部分の上端に来るまで、マスクイメージをスキップ

	if(boxmask.y < pi->i.box_img.y)
	{
		n = pi->i.box_img.y - boxmask.y;
	
		while(p->cur_line < n)
		{
			ret = _read_row_image(p, buf);
			if(ret) return ret;
		}
	}

	//読み込み
	
	ret = _read_row_image(p, buf);
	if(ret) return ret;

	//データ調整

	if(maskw == 0)
	{
		//マスクがイメージの範囲外なら、デフォルトで埋める

		memset(buf, (reverse)? 255 - defcol: defcol, imgw);
	}
	else
	{
		//---- イメージの範囲内

		if(left < 0)
		{
			//先頭から -left 分を除外

			memmove(buf, buf - left, maskw);
		}
		else if(left > 0)
		{
			//先頭から left 分をデフォルトで埋める

			memmove(buf + left, buf, maskw);
			memset(buf, defcol, left);
		}

		//右に足りない分を埋める

		if(right)
			memset(buf + (imgw - right), defcol, right);

		//反転

		if(reverse)
		{
			if(p->bits == 16)
				mReverseVal_16bit(buf, imgw >> 1);
			else
				mReverseVal_8bit(buf, imgw);
		}
	}

	return MLKERR_OK;
}
