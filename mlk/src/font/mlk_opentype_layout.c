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

/***********************************************
 * OpenType: GSUB/GPOS テーブル処理
 ***********************************************/

#include <string.h>
#include <stdlib.h>

#include "mlk.h"
#include "mlk_opentype.h"
#include "mlk_bufio.h"
#include "mlk_buf.h"
#include "mlk_util.h"


//----------------

struct _mOTLayout
{
	mBufIO io;

	uint16_t offset_script,
		offset_feature,
		offset_lookup,
		is_gpos;

	int lookup_cnt;	//[LookupList] lookupCount
};

typedef struct
{
	uint16_t format,num;
}coverage_dat;

//----------------

//mOT_POS のフラグ
enum
{
	_POSVF_XPOS = 1<<0,
	_POSVF_YPOS = 1<<1,
	_POSVF_XADVANCE = 1<<2,
	_POSVF_YADVANCE = 1<<3
};

//----------------


/* ソート関数 */

static int _compfunc(const void *p1,const void *p2)
{
	return (*((uint16_t *)p1) > *((uint16_t *)p2))? 1: -1;
}

/* (uint16 + datasize) x cnt のデータを、先頭の数値順にソート
 *
 * 先頭 2byte は、データ個数。
 *
 * datasize: 1個のデータサイズ */

void __mOpenType_sortData(uint8_t *buf,int datasize)
{
	qsort(buf + 2, *((uint16_t *)buf), datasize, _compfunc);
}

/* GSUB/GPOS テーブルヘッダ読み込み */

static mlkerr _read_table_header(mOTLayout *p)
{
	mBufIO *io = &p->io;
	uint16_t majver,minver,cnt;

	//バージョン

	if(mBufIO_read16(io, &majver)
		|| mBufIO_read16(io, &minver)
		|| majver != 1)
		return MLKERR_UNSUPPORTED;

	//各テーブルの位置

	if(mBufIO_read16(io, &p->offset_script)
		|| mBufIO_read16(io, &p->offset_feature)
		|| mBufIO_read16(io, &p->offset_lookup))
		return MLKERR_DAMAGED;

	//lookup count

	if(mBufIO_setPos(io, p->offset_lookup)
		|| mBufIO_read16(io, &cnt)
		|| mBufIO_isRemain(io, cnt * 2))
		return MLKERR_DAMAGED;

	p->lookup_cnt = cnt;

	return MLKERR_OK;
}

/* LookupList から Lookup テーブル取得
 *
 * num にサブテーブルの数、buf にサブテーブルのオフセットの配列、
 * offset に Lookup のオフセット位置。
 *
 * index: lookup インデックス
 * return: lookup type。-1 でエラー。 */

static int _get_lookup(mOTLayout *p,int index,mOT_TABLE *dst)
{
	mBufIO *io = &p->io;
	uint16_t pos,type,cnt,flag;
	uint32_t offset;

	if(index >= p->lookup_cnt) return -1;

	//LookupList

	mBufIO_setPos(io, p->offset_lookup + 2 + index * 2);
	mBufIO_read16(io, &pos);

	//Lookup

	offset = p->offset_lookup + pos;

	if(mBufIO_setPos(io, offset)
		|| mBufIO_read16(io, &type)
		|| mBufIO_read16(io, &flag)
		|| mBufIO_read16(io, &cnt))
		return -1;

	//

	dst->buf = io->cur;
	dst->num = cnt;
	dst->offset = offset;

	return type;
}

/* Lookup サブテーブルの位置へ移動
 *
 * type: lookup タイプを入れておく。オフセット拡張タイプの場合、実際の lookup タイプが入る。
 * return: 0 で成功 */

static int _move_lookup_subtable(mOTLayout *p,int *type,uint16_t offset_subst,const mOT_TABLE *lookup)
{
	mBufIO *io = &p->io;
	uint32_t pos,offset32;
	uint16_t format,type16;
	int extype;

	pos = lookup->offset + offset_subst;

	//オフセット拡張 (32bit)

	extype = (p->is_gpos)? 9: 7;

	if(*type == extype)
	{
		if(mBufIO_setPos(io, pos)
			|| mBufIO_read16(io, &format)
			|| format != 1
			|| mBufIO_read16(io, &type16)
			|| mBufIO_read32(io, &offset32))	//このサブテーブルの先頭を 0 とする
			return 1;

		*type = type16;

		pos += offset32;
	}

	return (mBufIO_setPos(io, pos) != 0);
}

/* Coverage テーブル読み込み
 *
 * return: 0 で成功 */

static int _read_coverage(mBufIO *io,int offset,mOT_LOOKUP_SUBTABLE *dat,coverage_dat *dst)
{
	uint16_t format,cnt;
	int size;

	if(mBufIO_setPos(io, dat->offset_subst + offset)
		|| mBufIO_read16(io, &format)
		|| (format != 1 && format != 2)
		|| mBufIO_read16(io, &cnt))
		return 1;

	//データサイズ確認

	size = (format == 1)? cnt * 2: cnt * 6;

	if(mBufIO_isRemain(io, size))
		return 1;

	//

	dst->format = format;
	dst->num = cnt;

	return 0;
}


//=========================
// GSUB: lookup 1
//=========================


/* lookup 関数 (GSUB lookup 1 データ作成) */

static mlkerr _func_gsub_lookup1(mOTLayout *p,mOT_LOOKUP_SUBTABLE *dat,void *param)
{
	mBufIO *io;
	mBuf *pbuf;
	coverage_dat cvg;
	uint16_t i,format,pos,cnt,gid,dstid,range[3];
	uint16_t *arrayid;
	int16_t delta;

	if(dat->lookup_type != 1) return MLKERR_OK;

	io = dat->io;
	pbuf = (mBuf *)param;

	//------ サブテーブル

	if(mBufIO_read16(io, &format)
		|| (format != 1 && format != 2)
		|| mBufIO_read16(io, &pos))	//coverageOffset
		return MLKERR_DAMAGED;

	if(format == 1)
	{
		//元の GID から増減
		
		if(mBufIO_read16(io, &delta))
			return MLKERR_DAMAGED;
	}
	else
	{
		//配列
	
		if(mBufIO_read16(io, &cnt)
			|| mBufIO_isRemain(io, cnt * 2))
			return MLKERR_DAMAGED;

		arrayid = (uint16_t *)io->cur;
	}

	//----- Coverage

	if(_read_coverage(io, pos, dat, &cvg))
		return MLKERR_DAMAGED;

	if(cvg.format == 1)
	{
		//GID 配列

		for(i = 0; i < cvg.num; i++)
		{
			mBufIO_read16(io, &gid);

			if(format == 1)
				dstid = (gid + delta) & 0xffff;
			else
				dstid = mGetBufBE16(arrayid + i);

			if(!mBufAppend(pbuf, &gid, 2)
				|| !mBufAppend(pbuf, &dstid, 2))
				return MLKERR_ALLOC;
		}
	}
	else
	{
		//GID 範囲

		for(i = 0; i < cvg.num; i++)
		{
			//[0]start [1]end [2]startindex
			mBufIO_read16_array(io, range, 3);

			for(gid = range[0]; gid <= range[1]; gid++)
			{
				if(format == 1)
					dstid = (gid + delta) & 0xffff;
				else
					dstid = mGetBufBE16(arrayid + (gid - range[0] + range[2]));

				if(!mBufAppend(pbuf, &gid, 2)
					|| !mBufAppend(pbuf, &dstid, 2))
					return MLKERR_ALLOC;
			}
		}
	}

	return MLKERR_OK;
}


//=========================
// GPOS: lookup 1
//=========================


/* [GPOS] ValueRecord から値を取得してバッファに追加
 *
 * return: -1 でエラー */

static int32_t _set_valuerecord(void *data,int flags,int size,mBuf *buf)
{
	int16_t val[4];
	uint32_t pos;

	mCopyBuf_16bit_BEtoHOST(val, data, size >> 1);

	pos = buf->cursize;

	if(!mBufAppendByte(buf, flags & 0xf)
		|| !mBufAppend(buf, val, size))
		return -1;

	return pos;
}

/* lookup 関数 (GPOS lookup 1 データ作成) */

static mlkerr _func_gpos_lookup1(mOTLayout *p,mOT_LOOKUP_SUBTABLE *dat,void *param)
{
	mBufIO *io;
	mBuf *buf1,*buf2;
	coverage_dat cvg;
	uint16_t i,format,pos,vflag,vcnt,gid,range[3];
	int32_t valpos,valsize,valsize_set;
	uint8_t *valdata = NULL;

	if(dat->lookup_type != 1) return MLKERR_OK;

	io = dat->io;
	buf1 = (mBuf *)*((void **)param);
	buf2 = (mBuf *)*((void **)param + 1);

	//------ サブテーブル

	if(mBufIO_read16(io, &format)
		|| (format != 1 && format != 2)
		|| mBufIO_read16(io, &pos)		//coverageOffset
		|| mBufIO_read16(io, &vflag))	//valueFormat
		return MLKERR_DAMAGED;

	valsize = mGetOnBitCount(vflag) * 2;	//実際のデータサイズ

	vflag &= 0xf;
	valsize_set = mGetOnBitCount(vflag) * 2;	//バッファにセットするデータサイズ

	if(format == 1)
	{
		//共通

		if(mBufIO_isRemain(io, valsize))
			return MLKERR_DAMAGED;

		valpos = _set_valuerecord(io->cur, vflag, valsize_set, buf2);
		if(valpos == -1) return MLKERR_ALLOC;
	}
	else
	{
		//各グリフごと

		if(mBufIO_read16(io, &vcnt)
			|| mBufIO_isRemain(io, vcnt * valsize))
			return MLKERR_DAMAGED;

		valdata = io->cur;
	}

	//----- Coverage

	if(_read_coverage(io, pos, dat, &cvg))
		return MLKERR_DAMAGED;

	if(cvg.format == 1)
	{
		//GID 配列

		for(i = 0; i < cvg.num; i++)
		{
			mBufIO_read16(io, &gid);

			if(format == 2)
			{
				valpos = _set_valuerecord(valdata, vflag, valsize_set, buf2);
				if(valpos == -1) return MLKERR_ALLOC;
				
				valdata += valsize;
			}

			if(!mBufAppend(buf1, &gid, 2)
				|| !mBufAppend(buf1, &valpos, 4))
				return MLKERR_ALLOC;
		}
	}
	else
	{
		//GID 範囲

		for(i = 0; i < cvg.num; i++)
		{
			//[0]start [1]end [2]startindex
			mBufIO_read16_array(io, range, 3);

			for(gid = range[0]; gid <= range[1]; gid++)
			{
				if(format == 2)
				{
					valpos = _set_valuerecord(
						valdata + (gid - range[0] + range[2]) * valsize,
						vflag, valsize_set, buf2);
					
					if(valpos == -1) return MLKERR_ALLOC;
				}

				if(!mBufAppend(buf1, &gid, 2)
					|| !mBufAppend(buf1, &valpos, 4))
					return MLKERR_ALLOC;
			}
		}
	}

	return MLKERR_OK;
}


//===========================
// main
//===========================


/**@ 解放 */

void mOTLayout_free(mOTLayout *p)
{
	mFree(p);
}

/**@ mOTLayout 作成
 *
 * @g:mOTLayout
 *
 * @d:GPOS/GSUB のテーブルデータを処理するためのもの
 *
 * @p:dst 作成された mOTLayout のポインタが入る
 * @p:buf GPOS/GSUB テーブルのバッファ
 * @p:size テーブルバッファサイズ
 * @p:is_gpos GSUB の場合 FALSE、GPOS の場合 TRUE */

mlkerr mOTLayout_new(mOTLayout **dst,void *buf,uint32_t size,mlkbool is_gpos)
{
	mOTLayout *p;
	mlkerr ret;

	p = (mOTLayout *)mMalloc0(sizeof(mOTLayout));
	if(!p) return MLKERR_ALLOC;

	mBufIO_init(&p->io, buf, size, MBUFIO_ENDIAN_BIG);

	p->is_gpos = is_gpos;

	//ヘッダ

	ret = _read_table_header(p);

	if(ret)
	{
		mFree(p);
		p = NULL;
	}
	
	*dst = p;

	return ret;
}

/**@ ScriptList テーブルの位置を取得
 *
 * @d:タグの一覧を取得して選択したい場合に使う。 */

mlkerr mOTLayout_getScriptList(mOTLayout *p,mOT_TABLE *dst)
{
	mBufIO *io = &p->io;
	uint16_t cnt;

	if(mBufIO_setPos(io, p->offset_script)
		|| mBufIO_read16(io, &cnt)
		|| mBufIO_isRemain(io, cnt * 6)
		|| cnt == 0)
		return MLKERR_DAMAGED;

	dst->buf = io->cur;
	dst->num = cnt;

	return MLKERR_OK;
}

/**@ ScriptList/Script テーブルから、各タグを読み込み
 *
 * @p:index タグのインデックス位置
 * @r:ScriptList テーブルの場合は、Script タグ。\
 *  Script テーブルの場合は、言語タグ。*/

uint32_t mOTLayout_readScriptLangTag(const mOT_TABLE *tbl,int index)
{
	if(index < 0 || index >= tbl->num)
		return 0;
	else
		return mGetBufBE32(tbl->buf + index * 6);
}

/**@ 指定 Script タグを検索し、最初に見つかったものを返す
 *
 * @p:tags 検索対象のタグの配列。先頭から順に検索される。値が 0 で終了。\
 *  'DFLT' = デフォルト、'latn' = ローマ字、'kana' = ひらかな、'hani' = CJK
 * @p:dsttag 見つかったタグが入る。エラー時や、見つからなかった場合は 0。
 * @r:見つからなかった場合、MLKERR_UNFOUND。 */

mlkerr mOTLayout_searchScriptList(mOTLayout *p,const uint32_t *tags,uint32_t *dsttag)
{
	mOT_TABLE tbl;
	int i;
	uint32_t t;
	mlkerr ret;

	*dsttag = 0;

	//ScriptList

	ret = mOTLayout_getScriptList(p, &tbl);
	if(ret) return ret;

	//検索

	while(1)
	{
		t = *(tags++);
		if(!t) break;

		for(i = 0; i < tbl.num; i++)
		{
			if(t == mOTLayout_readScriptLangTag(&tbl, i))
			{
				*dsttag = t;
				return MLKERR_OK;
			}
		}
	}

	return MLKERR_UNFOUND;
}

/**@ Script タグを指定し、Script テーブルの位置を取得
 *
 * @d:Script テーブルからは、言語タグの一覧が取得できる。\
 *  ※デフォルトの言語指定しかない場合、一つもタグがない場合あり。
 *
 * @p:dst offset には、Script テーブルのオフセット位置、\
 *  value には、デフォルトの LangSys へのオフセット位置、\
 *  tag には、実際に使われた Script タグが入っている。 */

mlkerr mOTLayout_getScript(mOTLayout *p,uint32_t script_tag,mOT_TABLE *dst)
{
	mBufIO *io = &p->io;
	mOT_TABLE tbl;
	uint8_t *ps,*pdef;
	uint32_t tag,offset;
	uint16_t cnt,def;
	int i;
	mlkerr ret;

	//ScriptList

	ret = mOTLayout_getScriptList(p, &tbl);
	if(ret) return ret;

	//検索

	ps = tbl.buf;
	pdef = NULL;

	for(i = tbl.num; i > 0; i--, ps += 6)
	{
		tag = mGetBufBE32(ps);
	
		if(tag == script_tag)
			break;
		else if(tag == MLK_MAKE32_4('D','F','L','T'))
			pdef = ps;
	}

	//見つからなかった場合、'DFLT' か先頭タグの位置

	if(i == 0)
		ps = (pdef)? pdef: tbl.buf;

	//Script テーブル

	tag = mGetBufBE32(ps);
	offset = p->offset_script + mGetBufBE16(ps + 4);

	if(mBufIO_setPos(io, offset)
		|| mBufIO_read16(io, &def)		//デフォルトの LangSys へのオフセット
		|| mBufIO_read16(io, &cnt)		//0 の場合あり
		|| mBufIO_isRemain(io, cnt * 6))
		return MLKERR_DAMAGED;

	//

	dst->buf = io->cur;
	dst->num = cnt;
	dst->tag = tag;
	dst->offset = offset;
	dst->value = def;

	return MLKERR_OK;
}

/**@ Script テーブルから、LangSys テーブルの位置を取得
 *
 * @p:lang_tag  言語タグ。0 でデフォルト (なければ先頭)。\
 *  指定タグが見つからなかった場合、デフォルトか、最初に格納されている言語が使われる。
 * @p:script Script テーブルの情報
 * @p:dst 結果が入る。script と同じポインタを指定しても良い。\
 *  value には、必須となる FeatureList インデックスが入っている (0xffff でなし)。\
 *  tag には、実際に使われた Lang タグが入っている (0 でデフォルト)。 */

mlkerr mOTLayout_getLang(mOTLayout *p,uint32_t lang_tag,const mOT_TABLE *script,mOT_TABLE *dst)
{
	mBufIO *io = &p->io;
	uint8_t *ps;
	uint16_t v16,pos;
	uint32_t tag,offset;
	int i;

	//LangSysRecord 配列の先頭

	if(mBufIO_setPos(io, script->offset + 4))
		return MLKERR_DAMAGED;

	//言語タグ検索

	ps = io->cur;

	for(i = script->num; i > 0; i--, ps += 6)
	{
		tag = mGetBufBE32(ps);

		if(tag == lang_tag)
		{
			pos = mGetBufBE16(ps + 4);
			break;
		}
	}

	//見つからなかった場合

	if(i == 0)
	{
		if(script->value)
		{
			//defaultLangSys
			
			tag = 0;
			pos = script->value;
		}
		else
		{
			//先頭のタグ
			
			if(script->num == 0) return MLKERR_DAMAGED;

			ps = io->cur;
			tag = mGetBufBE32(ps);
			pos = mGetBufBE16(ps + 4);
		}
	}

	//LangSys のオフセット位置

	offset = script->offset + pos;

	//LangSys

	if(mBufIO_setPos(io, offset)
		|| mBufIO_isRemain(io, 6))
		return MLKERR_DAMAGED;

	mBufIO_seek(io, 2);			//lookupOrder

	mBufIO_read16(io, &v16);	//requiredFeatureIndex
	dst->value = v16;
	
	mBufIO_read16(io, &v16);	//featureIndexCount

	if(mBufIO_isRemain(io, v16 * 2))
		return MLKERR_DAMAGED;

	//

	dst->buf = io->cur;
	dst->num = v16;
	dst->tag = tag;
	dst->offset = offset;

	return MLKERR_OK;
}

/**@ LangSys テーブルを元に、Feature タグを検索して、Feature テーブルの位置を取得
 *
 * @p:langsys LangSys テーブルの情報
 * @p:dst 結果が入る。langsys と同じポインタを指定しても良い。\
 *  tag には、feature_tag の値が入っている。
 * @r:タグが見つからなかった場合、MLKERR_UNFOUND。 */

mlkerr mOTLayout_getFeature(mOTLayout *p,uint32_t feature_tag,const mOT_TABLE *langsys,mOT_TABLE *dst)
{
	mBufIO *io = &p->io;
	const uint16_t *pfi;
	uint16_t cnt,index,pos;
	uint32_t offset,tag;
	int i;

	//FeatureList

	if(mBufIO_setPos(io, p->offset_feature)
		|| mBufIO_read16(io, &cnt)
		|| mBufIO_isRemain(io, cnt * 6))
		return MLKERR_DAMAGED;

	//LangSys の featureIndices からインデックスを取得し、
	//FeatureRecord からタグを検索

	pfi = (const uint16_t *)langsys->buf;
	offset = p->offset_feature + 2;	//FeatureRecord の配列の先頭

	for(i = langsys->num; i > 0; i--, pfi++)
	{
		index = mGetBufBE16(pfi);
		if(index >= cnt) continue;

		mBufIO_setPos(io, offset + index * 6);
		mBufIO_read32(io, &tag);

		if(tag == feature_tag)
		{
			mBufIO_read16(io, &pos);
			break;
		}
	}

	if(i == 0) return MLKERR_UNFOUND;

	//Feature

	offset = p->offset_feature + pos;

	if(mBufIO_setPos(io, offset)
		|| mBufIO_seek(io, 2)
		|| mBufIO_read16(io, &cnt))
		return MLKERR_DAMAGED;

	//

	dst->buf = io->cur;
	dst->num = cnt;
	dst->tag = feature_tag;
	dst->offset = offset;

	return MLKERR_OK;
}

/**@ Script/Lang/Feature タグを指定し、Feature テーブル位置を取得 */

mlkerr mOTLayout_getFeature2(mOTLayout *p,uint32_t script_tag,uint32_t lang_tag,uint32_t feature_tag,mOT_TABLE *dst)
{
	mlkerr ret;

	ret = mOTLayout_getScript(p, script_tag, dst);
	if(ret) return ret;

	ret = mOTLayout_getLang(p, lang_tag, dst, dst);
	if(ret) return ret;

	return mOTLayout_getFeature(p, feature_tag, dst, dst);
}

/**@ Feature テーブルから、Lookup サブテーブルを列挙
 *
 * @p:func サブテーブルごとに呼ばれる関数
 * @p:param 関数に渡されるパラメータ値 */

mlkerr mOTLayout_getLookup(mOTLayout *p,const mOT_TABLE *feature,mFuncOTLayoutLookupSubtable func,void *param)
{
	mOT_TABLE lookup;
	mOT_LOOKUP_SUBTABLE dat;
	const uint16_t *plookup,*psubtbl;
	int i,j,type;
	mlkerr ret;

	plookup = (const uint16_t *)feature->buf;

	for(i = feature->num; i > 0; i--, plookup++)
	{
		//Lookup テーブル (戻り値: lookup type)
	
		type = _get_lookup(p, mGetBufBE16(plookup), &lookup);
		if(type == -1) return MLKERR_DAMAGED;

		//各サブテーブル処理

		psubtbl = (const uint16_t *)lookup.buf;

		for(j = lookup.num; j > 0; j--, psubtbl++)
		{
			//サブテーブルの位置へ
			
			if(_move_lookup_subtable(p, &type, mGetBufBE16(psubtbl), &lookup))
				return MLKERR_DAMAGED;

			//関数

			dat.io = &p->io;
			dat.lookup_type = type;
			dat.offset_subst = mBufIO_getPos(&p->io);

			ret = (func)(p, &dat, param);
			if(ret) return ret;
		}
	}

	return MLKERR_OK;
}

/**@ Feature テーブルから、GSUB の単一置換データを作成
 *
 * @d:lookup type 1 のサブテーブルのみ処理。
 * 
 * @p:ppdst 確保されたバッファが入る。\
 *  uint16: データ数\
 *  4byte x num: [置換元GID (uint16)] + [置換先GID (uint16)] */

mlkerr mOTLayout_createGSUB_single(mOTLayout *p,const mOT_TABLE *feature,uint8_t **ppdst)
{
	mBuf buf;
	int cnt;
	mlkerr ret;

	//バッファ

	mBufInit(&buf);

	if(!mBufAlloc(&buf, 2048, 2048))
		return MLKERR_ALLOC;

	mBufAppend0(&buf, 2); //データ個数

	//サブテーブル

	ret = mOTLayout_getLookup(p, feature, _func_gsub_lookup1, &buf);
	if(ret)
	{
		mBufFree(&buf);
		return ret;
	}

	//データ個数

	cnt = (buf.cursize - 2) / 4;

	*((uint16_t *)buf.buf) = cnt;

	//GID 順にソート

	__mOpenType_sortData(buf.buf, 4);

	//

	mBufCutCurrent(&buf);

	*ppdst = buf.buf;

	return MLKERR_OK;
}

/**@ Feature テーブルから、GPOS の単一グリフ調整データを作成
 *
 * @p:ppdst 確保されたバッファが入る。\
 *  uint16: データ数\
 *  6byte x num: [GID (uint16)] + [調整データのオフセット位置 (uint32)。データの先頭 (データ数の位置) を 0 とする]\
 *  N byte: 各調整データ\
 *  \
 *  調整データは、先頭が 1byte のフラグ。その後に int16 のデータが続く。\
 *  (bit0=X位置, bit1=Y位置, bit2=advance X, bit3=advance Y) */

mlkerr mOTLayout_createGPOS_single(mOTLayout *p,const mOT_TABLE *feature,uint8_t **ppdst)
{
	mBuf buf,buf2;
	void *buf_array[2];
	uint8_t *pd;
	int i,cnt;
	mlksize pos;
	mlkerr ret;

	//バッファ (buf = IDとオフセット、buf2 = 調整データ)

	mBufInit(&buf);
	mBufInit(&buf2);

	if(!mBufAlloc(&buf, 4096, 4096))
		return MLKERR_ALLOC;

	if(!mBufAlloc(&buf2, 2048, 2048))
	{
		mBufFree(&buf);
		return MLKERR_ALLOC;
	}

	mBufAppend0(&buf, 2); //データ数

	//サブテーブル

	buf_array[0] = &buf;
	buf_array[1] = &buf2;

	ret = mOTLayout_getLookup(p, feature, _func_gpos_lookup1, (void *)buf_array);
	if(ret) goto ERR;

	//データ数

	cnt = (buf.cursize - 2) / 6;

	*((uint16_t *)buf.buf) = cnt;

	//GID 順にソート

	__mOpenType_sortData(buf.buf, 6);

	//データを結合

	pos = buf.cursize;

	if(!mBufAppend(&buf, buf2.buf, buf2.cursize))
	{
		ret = MLKERR_ALLOC;
		goto ERR;
	}

	mBufFree(&buf2);

	//オフセット位置を調整

	pd = buf.buf + 4;

	for(i = 0; i < cnt; i++, pd += 6)
		*((uint32_t *)pd) += pos;

	//

	mBufCutCurrent(&buf);

	*ppdst = buf.buf;

	return MLKERR_OK;

ERR:
	mBufFree(&buf);
	mBufFree(&buf2);
	return ret;
}
