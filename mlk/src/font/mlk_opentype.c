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
 * OpenType、GSUB/GPOS テーブル操作
 *****************************************/

#include <string.h>

#include "mlk.h"
#include "mlk_opentype.h"
#include "mlk_bufio.h"
#include "mlk_util.h"


void __mOpenType_sortData(uint8_t *buf,int datasize);


//===========================
// main
//===========================


/**@ [num(uint16), [GID(uint16) + data], ...] のデータから、GID を検索
 *
 * @d:データは GID の小さい順にソート済み。
 * 
 * @p:datasize 1つのデータのサイズ (GID の 2byte を含む)
 * @r:見つかったら、データ部分の先頭位置。NULL で見つからなかった。 */

void *mOpenType_searchData(uint16_t gid,uint8_t *buf,int datasize)
{
	uint8_t *src;
	uint16_t *ps;
	int low,high,mid;

	if(!buf) return NULL;

	src = buf + 2;
	low = 0;
	high = *((uint16_t *)buf) - 1;

	while(low <= high)
	{
		mid = (low + high) >> 1;
		ps = (uint16_t *)(src + mid * datasize);

		if(*ps == gid)
			return (void *)ps;
		else if(*ps < gid)
			low = mid + 1;
		else
			high = mid - 1;
	}

	return NULL;
}

/**@ VORG テーブル読み込み
 *
 * @d:OpenType(CFF) の場合にオプションとして存在する、縦書き時の原点 Y 位置の情報。
 *
 * @p:buf VORG テーブルバッファ */

mlkerr mOpenType_readVORG(void *buf,uint32_t size,mOT_VORG *dst)
{
	mBufIO io;
	int16_t i16;
	uint16_t num;
	uint32_t u32;
	uint8_t *pd;

	mMemset0(dst, sizeof(mOT_VORG));

	mBufIO_init(&io, buf, size, MBUFIO_ENDIAN_BIG);

	//バージョン

	if(mBufIO_read32(&io, &u32) || u32 != 0x00010000)
		return MLKERR_INVALID_VALUE;

	//デフォルトの y 位置

	if(mBufIO_read16(&io, &i16))
		return MLKERR_DAMAGED;

	dst->default_origin = i16;
	dst->have_vorg = 1;

	//配列の数 (0 の場合あり)

	if(mBufIO_read16(&io, &num)
		|| mBufIO_isRemain(&io, num * 4))
		return MLKERR_DAMAGED;

	//データ

	if(num)
	{
		//確保
		
		dst->buf = pd = (uint8_t *)mMalloc(2 + 4 * num);
		if(!dst->buf) return MLKERR_ALLOC;

		//データ数

		*((uint16_t *)pd) = num;

		//コピー

		mCopyBuf_16bit_BEtoHOST(pd + 2, io.cur, num * 2);
	}

	return MLKERR_OK;
}

/**@ GSUB テーブルデータから、単一置換 (lookup 1) のデータを読み込み
 *
 * @p:buf GSUB テーブルバッファ
 * @p:size GUSB テーブルデータサイズ
 * @p:script_tag Script タグ。\
 *  見つからなかった場合、'DFLT' か先頭タグが使われる。
 * @p:ppdst 確保されたバッファが入る */

mlkerr mOpenType_readGSUB_single(void *buf,uint32_t size,
	uint32_t script_tag,uint32_t lang_tag,uint32_t feature_tag,uint8_t **ppdst)
{
	mOTLayout *p;
	mOT_TABLE tbl;
	mlkerr ret;

	*ppdst = NULL;

	ret = mOTLayout_new(&p, buf, size, FALSE);
	if(ret) return ret;

	ret = mOTLayout_getFeature2(p, script_tag, lang_tag, feature_tag, &tbl);
	if(ret == MLKERR_OK)
		ret = mOTLayout_createGSUB_single(p, &tbl, ppdst);

	mOTLayout_free(p);

	return ret;
}

/**@ GPOS テーブルデータから、単一グリフ調整のデータを読み込み */

mlkerr mOpenType_readGPOS_single(void *buf,uint32_t size,
	uint32_t script_tag,uint32_t lang_tag,uint32_t feature_tag,uint8_t **ppdst)
{
	mOTLayout *p;
	mOT_TABLE tbl;
	mlkerr ret;

	*ppdst = NULL;

	ret = mOTLayout_new(&p, buf, size, TRUE);
	if(ret) return ret;

	ret = mOTLayout_getFeature2(p, script_tag, lang_tag, feature_tag, &tbl);
	if(ret == MLKERR_OK)
		ret = mOTLayout_createGPOS_single(p, &tbl, ppdst);

	mOTLayout_free(p);

	return ret;
}


//====================
// 各データ取得
//====================


/**@ VORG データから、指定 GID の原点Y位置を取得
 *
 * @p:origin 原点Y位置が格納される (フォントデザイン単位)。\
 *  水平レイアウトでの原点位置を基準とした値。
 * @r:FALSE で VORG が存在しない */

mlkbool mOpenType_getVORG_originY(mOT_VORG *p,uint16_t gid,int *origin)
{
	int16_t *ps;

	//VORG がない

	if(!p->have_vorg) return FALSE;

	//data = NULL の場合、デフォルト

	ps = (int16_t *)mOpenType_searchData(gid, p->buf, 4);
	if(ps)
		*origin = ps[1];
	else
		*origin = p->default_origin;
			
	return TRUE;
}

/**@ GSUB 単一置換データから、GID 置換
 *
 * @r:置き換え後の GID。データ内になければ、gid をそのまま返す。 */

uint16_t mOpenType_getGSUB_replaceSingle(uint8_t *buf,uint16_t gid)
{
	uint16_t *ps;

	ps = (uint16_t *)mOpenType_searchData(gid, buf, 4);
	if(!ps)
		return gid;
	else
		return ps[1];
}

/**@ GPOS の単一グリフ調整データから、調整値を取得
 *
 * @p:pos 各値は、フォントデザイン単位。データがなかった場合、すべて 0。
 * @r:指定グリフのデータがあった場合は TRUE */

mlkbool mOpenType_getGPOS_single(uint8_t *buf,uint16_t gid,mOT_POS *pos)
{
	uint8_t *ps,f;
	int16_t *pval;

	mMemset0(pos, sizeof(mOT_POS));

	if(!buf)
		return FALSE;
	else
	{
		ps = (uint8_t *)mOpenType_searchData(gid, buf, 6);
		if(!ps) return FALSE;

		ps = buf + *((uint32_t *)(ps + 2));
		f = *(ps++);

		pval = (int16_t *)ps;

		//

		if(f & 1)
			pos->x = *(pval++);

		if(f & 2)
			pos->y = *(pval++);

		if(f & 4)
			pos->advx = *(pval++);
		
		if(f & 8)
			pos->advy = *(pval++);
	}

	return TRUE;
}


//====================
// データ編集
//====================


/**@ GSUB 単一置き換えデータを結合
 *
 * @d:srcbuf に addbuf のデータを追加する。\
 *  置き換え元 ID が同じデータは追加されない。\
 *  {em:※srcbuf,addbuf が両方とも NULL でない場合、元のバッファは自動的に解放される。:em}
 *
 * @p:srcbuf 追加元のデータ。NULL でなし。
 * @p:addbuf 追加するデータ。NULL でなし。
 * @r:結合されたデータバッファ。NULL で失敗。\
 *  srcbuf,addbuf が両方 NULL なら、NULL が返る。\
 *  いずれかが NULL の場合は、NULL でない方のポインタがそのまま返る。\
 *  追加するデータがない場合は、addbuf を解放して srcbuf を返す。 */

uint8_t *mOpenType_combineData_GSUB_single(uint8_t *srcbuf,uint8_t *addbuf)
{
	int i,srcnum,addnum,add;
	uint16_t *padd,*pd;
	uint8_t *pnew;

	//両方またはいずれかが NULL の時

	if(!srcbuf || !addbuf)
	{
		if(srcbuf) return srcbuf;
		else if(addbuf) return addbuf;
		else return NULL;
	}

	//

	srcnum = *((uint16_t *)srcbuf);
	addnum = *((uint16_t *)addbuf);

	//(addbuf) 同じ GID があれば、0 にする

	padd = (uint16_t *)(addbuf + 2);
	add = 0;

	for(i = addnum; i; i--, padd += 2)
	{
		if(mOpenType_searchData(*padd, srcbuf, 4))
			*padd = 0;
		else
			add++;
	}

	//追加数が 0 の場合、srcbuf を返す

	if(add == 0)
	{
		mFree(addbuf);
		return srcbuf;
	}

	//確保

	pnew = (uint8_t *)mMalloc(2 + (srcnum + add) * 4);
	if(!pnew) goto END;

	pd = (uint16_t *)pnew;
	*(pd++) = srcnum + add;

	//src をコピー

	memcpy(pd, srcbuf + 2, srcnum * 4);

	pd += srcnum * 2;

	//追加分をセット

	padd = (uint16_t *)(addbuf + 2);
	
	for(i = addnum; i; i--, padd += 2)
	{
		if(*padd)
		{
			pd[0] = padd[0];
			pd[1] = padd[1];
			pd += 2;
		}
	}

	//ソート

	__mOpenType_sortData(pnew, 4);

	//終了
END:
	mFree(srcbuf);
	mFree(addbuf);

	return pnew;
}
