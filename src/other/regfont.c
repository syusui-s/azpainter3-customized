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

/*******************************
 * 登録フォント
 *******************************/

#include <stdio.h>
#include <string.h>

#include "mlk_gui.h"
#include "mlk_list.h"
#include "mlk_str.h"
#include "mlk_buf.h"
#include "mlk_util.h"
#include "mlk_stdio.h"

#include "regfont.h"


/* データの変更時は fmodify を TRUE にする。 */

//----------------

typedef struct
{
	mListItem i;
	uint32_t size;
	uint8_t dat[1];
}_listitem;

//データ

typedef struct
{
	mList list;
	int fmodify;	//データが変更されたか
}regfont;

static regfont g_dat;

#define _FILENAME_REGFONT  "regfont.dat"

//----------------


//==========================
// sub
//==========================


/* RegFontDat からデータサイズ計算 */

static int _calc_datsize(RegFontDat *p)
{
	int s = 2 + 2 + 20 + 2 * 3;

	s += p->strname.len + 1
		+ p->str_basefont.len
		+ p->str_repfont[0].len
		+ p->str_repfont[1].len
		+ p->charsize[0] + p->charsize[1];

	return s;
}

/* (BE) 文字列データをセット
 *
 * fnull: NULL を含むか */

static uint8_t *_setdat_str(uint8_t *pd,mStr *str,mlkbool fnull)
{
	int len;
	
	//長さ

	len = str->len;
	if(len > 0xffff) len = 0;
	if(fnull) len++;

	mSetBufBE16(pd, len);
	pd += 2;

	//文字列

	if(len)
	{
		memcpy(pd, str->buf, len);
		pd += len;
	}

	return pd;
}

/* (BE) 文字列データを取得 */

static uint8_t *_getdat_str(uint8_t *ps,mStr *str,mlkbool fnull)
{
	int len;

	len = mGetBufBE16(ps);
	ps += 2;

	if(len)
	{
		mStrSetText_len(str, (const char *)ps, (fnull)? len - 1: len);
		ps += len;
	}

	return ps;
}


//==========================
// main
//==========================


/** 初期化 */

void RegFont_init(void)
{
	mMemset0(&g_dat, sizeof(regfont));
}

/** 解放 */

void RegFont_free(void)
{
	mListDeleteAll(&g_dat.list);
}

/** 先頭アイテムを取得 */

mListItem *RegFont_getTopItem(void)
{
	return g_dat.list.top;
}

/** アイテムから名前のポインタ位置を取得 */

const char *RegFont_getNamePtr(mListItem *pi)
{
	return (const char *)(((_listitem *)pi)->dat + 2 + 2);
}

/** アイテムから ID を取得 */

int RegFont_getItemID(mListItem *pi)
{
	return mGetBufBE16(((_listitem *)pi)->dat);
}

/** リストをクリア (リスト編集OK時) */

void RegFont_clear(void)
{
	mListDeleteAll(&g_dat.list);

	g_dat.fmodify = TRUE;
}

/** RegFontDat からアイテム追加 */

mListItem *RegFont_addDat(RegFontDat *p)
{
	_listitem *pi;
	uint8_t *pd;
	int size;

	size = _calc_datsize(p);

	pi = (_listitem *)mListAppendNew(&g_dat.list, sizeof(_listitem) - 1 + size);
	if(!pi) return NULL;

	pi->size = size;

	pd = pi->dat;

	//ID

	mSetBufBE16(pd, p->id);
	pd += 2;

	//名前

	pd = _setdat_str(pd, &p->strname, TRUE);

	//値

	pd += mSetBuf_format(pd, ">iiihhhh",
		p->index_base, p->index_rep[0], p->index_rep[1],
		p->charflags[0], p->charflags[1],
		p->charsize[0], p->charsize[1]);

	//フォントファイル

	pd = _setdat_str(pd, &p->str_basefont, FALSE);
	pd = _setdat_str(pd, &p->str_repfont[0], FALSE);
	pd = _setdat_str(pd, &p->str_repfont[1], FALSE);

	//文字データ (Host -> BE)

	if(p->charbuf)
		mCopyBuf_32bit_BEtoHOST(pd, p->charbuf, (p->charsize[0] + p->charsize[1]) / 4);

	return (mListItem *)pi;
}

/** 指定アイテムのデータを置き換え (直接編集時) */

mListItem *RegFont_replaceDat(mListItem *pi,RegFontDat *dat)
{
	mListItem *ins;

	g_dat.fmodify = TRUE;

	ins = pi->next;

	//削除

	mListDelete(&g_dat.list, pi);

	//追加

	pi = RegFont_addDat(dat);
	if(!pi) return NULL;

	//元の位置に移動

	if(ins)
		mListMove(&g_dat.list, pi, ins);

	return pi;
}

/** ID からアイテムを検索 */

mListItem *RegFont_findID(int id)
{
	mListItem *pi;

	for(pi = g_dat.list.top; pi; pi = pi->next)
	{
		if(RegFont_getItemID(pi) == id)
			return pi;
	}

	return NULL;
}


//============================
// ファイル
//============================


/** 設定ファイル読み込み */

void RegFont_loadConfigFile(void)
{
	mStr str = MSTR_INIT;

	mGuiGetPath_config(&str, _FILENAME_REGFONT);

	RegFont_loadFile(str.buf);

	mStrFree(&str);
}

/** 設定ファイルに保存 (データ変更時のみ) */

void RegFont_saveConfigFile(void)
{
	mStr str = MSTR_INIT;

	if(!g_dat.fmodify) return;

	mGuiGetPath_config(&str, _FILENAME_REGFONT);

	RegFont_saveFile(str.buf);

	mStrFree(&str);
}

/** ファイル読み込み */

mlkerr RegFont_loadFile(const char *filename)
{
	FILE *fp;
	_listitem *pi;
	uint16_t ver,num;
	uint32_t size;
	mlkerr ret;

	fp = mFILEopen(filename, "rb");
	if(!fp) return MLKERR_OPEN;

	if(mFILEreadStr_compare(fp, "AZPRFON")
		|| mFILEreadBE16(fp, &ver)
		|| ver != 0)
	{
		ret = MLKERR_UNSUPPORTED;
		goto ERR;
	}

	//個数
	
	if(mFILEreadBE16(fp, &num))
	{
		ret = MLKERR_DAMAGED;
		goto ERR;
	}

	//データ

	for(; num; num--)
	{
		if(mFILEreadBE32(fp, &size)) break;

		pi = (_listitem *)mListAppendNew(&g_dat.list, sizeof(_listitem) - 1 + size);
		if(!pi)
		{
			ret = MLKERR_ALLOC;
			goto ERR;
		}

		pi->size = size;

		if(mFILEreadOK(fp, pi->dat, size))
		{
			ret = MLKERR_DAMAGED;
			mListDelete(&g_dat.list, MLISTITEM(pi));
			goto ERR;
		}
	}

	ret = MLKERR_OK;

ERR:
	fclose(fp);

	return ret;
}

/** ファイル保存 */

void RegFont_saveFile(const char *filename)
{
	FILE *fp;
	_listitem *pi;

	fp = mFILEopen(filename, "wb");
	if(!fp) return;

	fputs("AZPRFON", fp);

	mFILEwriteBE16(fp, 0);
	mFILEwriteBE16(fp, g_dat.list.num);

	MLK_LIST_FOR(g_dat.list, pi, _listitem)
	{
		mFILEwriteBE32(fp, pi->size);

		fwrite(pi->dat, 1, pi->size, fp);
	}

	fclose(fp);
}


//==========================
// RegFontDat
//==========================


/** RegFontDat 解放 */

void RegFontDat_free(RegFontDat *p)
{
	if(p)
	{
		mStrFree(&p->strname);
		mStrFree(&p->str_basefont);
		mStrFree(&p->str_repfont[0]);
		mStrFree(&p->str_repfont[1]);

		mFree(p->charbuf);
		mFree(p);
	}
}

/** 新規作成 */

RegFontDat *RegFontDat_new(void)
{
	return (RegFontDat *)mMalloc0(sizeof(RegFontDat));
}

/** 複製を作成 (ID はそのままコピー) */

RegFontDat *RegFontDat_dup(const RegFontDat *src)
{
	RegFontDat *p;

	p = (RegFontDat *)mMalloc0(sizeof(RegFontDat));
	if(!p) return NULL;

	*p = *src;

	mStrCopy_init(&p->strname, &src->strname);
	mStrCopy_init(&p->str_basefont, &src->str_basefont);
	mStrCopy_init(&p->str_repfont[0], &src->str_repfont[0]);
	mStrCopy_init(&p->str_repfont[1], &src->str_repfont[1]);

	p->charbuf = (uint8_t *)mMemdup(src->charbuf, src->charsize[0] + src->charsize[1]);

	return p;
}

/** mListItem から RegFontDat を作成 */

RegFontDat *RegFontDat_make(mListItem *item)
{
	RegFontDat *p;
	uint8_t *ps;
	int size;

	p = (RegFontDat *)mMalloc0(sizeof(RegFontDat));
	if(!p) return NULL;

	ps = ((_listitem *)item)->dat;

	//ID

	p->id = mGetBufBE16(ps);
	ps += 2;

	//名前

	ps = _getdat_str(ps, &p->strname, TRUE);

	//値

	ps += mGetBuf_format(ps, ">iiihhhh",
		&p->index_base, &p->index_rep[0], &p->index_rep[1],
		&p->charflags[0], &p->charflags[1],
		&p->charsize[0], &p->charsize[1]);

	//フォントファイル

	ps = _getdat_str(ps, &p->str_basefont, FALSE);
	ps = _getdat_str(ps, &p->str_repfont[0], FALSE);
	ps = _getdat_str(ps, &p->str_repfont[1], FALSE);

	//文字データ (BE -> host, ない場合は NULL)

	size = p->charsize[0] + p->charsize[1];

	if(size)
	{
		p->charbuf = (uint8_t *)mMalloc(size);

		if(p->charbuf)
			mCopyBuf_32bit_BEtoHOST(p->charbuf, ps, size / 4);
	}

	return p;
}

/** (編集時) 置き換え文字データをセット
 *
 * no: 0,1 */

void RegFontDat_setCharBuf(RegFontDat *p,int no,mBuf *buf)
{
	int bksize,newsize,pos;
	uint8_t *newbuf;

	//データサイズ

	newsize = buf->cursize;
	bksize = p->charsize[no];

	p->charsize[no] = newsize;

	//全体が空

	if(p->charsize[0] + p->charsize[1] == 0)
	{
		mFree(p->charbuf);
		p->charbuf = NULL;
		return;
	}

	//データ

	if(bksize == newsize)
	{
		//前回と同じサイズならコピー

		if(newsize)
		{
			pos = (no == 0)? 0: p->charsize[0];

			memcpy(p->charbuf + pos, buf->buf, newsize);
		}
	}
	else
	{
		//サイズが異なる場合、再確保してセット

		newbuf = (uint8_t *)mMalloc(p->charsize[0] + p->charsize[1]);
		if(!newbuf)
		{
			p->charsize[no] = bksize;
			return;
		}

		if(no == 0)
		{
			//rep1 を置き換え
			memcpy(newbuf, buf->buf, newsize);
			memcpy(newbuf + newsize, p->charbuf + bksize, p->charsize[1]);
		}
		else
		{
			//rep2 を置き換え
			memcpy(newbuf, p->charbuf, p->charsize[0]);
			memcpy(newbuf + p->charsize[0], buf->buf, newsize);
		}

		mFree(p->charbuf);

		p->charbuf = newbuf;
	}
}

/** RegFontDat の置換文字データから、編集時用のテキストを取得 */

mlkbool RegFontDat_getCodeText(RegFontDat *p,int no,mStr *str)
{
	uint32_t *ps,*psend,v,v2;
	mlkbool hex;

	if(!p->charbuf || !p->charsize[no]) return FALSE;

	hex = !(p->charflags[no] & REGFONT_CHARF_CODE_CID);

	if(no == 0)
		ps = (uint32_t *)p->charbuf;
	else
		ps = (uint32_t *)(p->charbuf + p->charsize[0]);

	psend = (uint32_t *)((uint8_t *)ps + p->charsize[no]);

	//

	while(ps < psend)
	{
		v = *(ps++);

		if(v & REGFONT_CODE_F_RANGE)
		{
			v &= REGFONT_CODE_MASK;
			v2 = *(ps++);
			
			if(hex)
				mStrAppendFormat(str, "%X-%X,", v, v2);
			else
				mStrAppendFormat(str, "%d-%d,", v, v2);
		}
		else
		{
			if(hex)
				mStrAppendFormat(str, "%X,", v);
			else
				mStrAppendFormat(str, "%d,", v);
		}
	}

	//終端の , を除外

	if(mStrGetLastChar(str) == ',')
		mStrSetLen(str, str->len - 1);

	return TRUE;
}


//===============================
// 文字列から文字データを作成
//===============================


/* バッファ位置から値の範囲を取得 */

static uint32_t *_get_code(uint32_t *ps,uint32_t *val1,uint32_t *val2)
{
	uint32_t v1,v2;

	v1 = *(ps++);

	if(v1 & REGFONT_CODE_F_RANGE)
	{
		v1 &= REGFONT_CODE_MASK;
		v2 = *(ps++);
	}
	else
		v2 = v1;

	*val1 = v1;
	*val2 = v2;

	return ps;
}

/* 値の小さい順に並べ替え & 重複チェック
 *
 * return: 0 で成功、1 で重複 */

static int _sort_charbuf(mBuf *buf)
{
	uint32_t *ps,*psend,*psnext,*ps2,*ps2next,*pmin;
	uint32_t v1,v2,vc1,vc2,vmin1,vmin2;

	ps = (uint32_t *)buf->buf;
	psend = (uint32_t *)(buf->buf + buf->cursize);

	while(ps < psend)
	{
		psnext = _get_code(ps, &v1, &v2);

		pmin = ps;
		vmin1 = v1;
		vmin2 = v2;

		//重複判定と最小値取得

		for(ps2 = psnext; ps2 < psend; ps2 = ps2next)
		{
			ps2next = _get_code(ps2, &vc1, &vc2);

			//v1-v2 が vc1-vc2 の範囲と重複

			if(!(v1 > vc2 || v2 < vc1))
				return 1;

			//最小

			if(vc1 < vmin1)
			{
				pmin = ps2;
				vmin1 = vc1;
				vmin2 = vc2;
			}
		}

		//入れ替え & ps を次の位置へ

		if(pmin == ps)
			ps = psnext;
		else if(v1 == v2 && vmin1 == vmin2)
		{
			//両方単体

			*ps = vmin1;
			*pmin = v1;

			ps = psnext;
		}
		else if(v1 != v2 && vmin1 != vmin2)
		{
			//両方範囲

			ps[0] = vmin1 | REGFONT_CODE_F_RANGE;
			ps[1] = vmin2;
			pmin[0] = v1 | REGFONT_CODE_F_RANGE;
			pmin[1] = v2;

			ps = psnext;
		}
		else if(v1 == v2)
		{
			//単体 <-> 範囲

			memmove(ps + 2, psnext, (pmin - psnext) * 4);

			ps[0] = vmin1 | REGFONT_CODE_F_RANGE;
			ps[1] = vmin2;
			pmin[1] = v1;

			ps += 2;
		}
		else
		{
			//範囲 <-> 単体

			memmove(ps + 1, psnext, (pmin - psnext) * 4);

			*ps = vmin1;
			pmin[-1] = v1 | REGFONT_CODE_F_RANGE;
			pmin[0] = v2;

			ps++;
		}
	}

	return 0;
}

/** 文字列から文字データ値を作成
 *
 * return: 0 で成功、-1 で調整不可のエラー、1 で重複 */

int RegFontDat_text_to_charbuf(mBuf *buf,const char *text,mlkbool hex)
{
	uint32_t val,valst,valtmp;
	char c;
	mlkbool frange = FALSE,fval = FALSE,floop = TRUE;

	val = 0;

	while(floop)
	{
		c = *(text++);

		if(!c) floop = FALSE;

		if(c == ',' || c == 0)
		{
			//---- 値の区切り

			//",," の場合はスキップ

			if(!fval) continue;

			if(val > 0xffffff)
				val = 0xffffff;

			//範囲で N=N の場合は単体へ

			if(frange && val == valst)
				frange = FALSE;

			//追加
			
			if(!frange)
				//単体
				mBufAppend(buf, &val, 4);
			else
			{
				//範囲
				// :左の方が大きい場合、入れ替え
				
				if(valst >= val)
				{
					valtmp = val;
					val = valst;
					valst = valtmp;
				}

				valst |= REGFONT_CODE_F_RANGE;
			
				mBufAppend(buf, &valst, 4);
				mBufAppend(buf, &val, 4);
			}
		
			frange = FALSE;
			fval = FALSE;
			val = 0;
		}
		else if(c == '-')
		{
			//---- 範囲の区切り

			//"N-N-N" の場合エラー

			if(frange) return -1;
			
			if(val > 0xffffff)
				val = 0xffffff;

			valst = val;

			frange = TRUE;
			fval = FALSE;
			val = 0;
		}
		else if(hex)
		{
			//16進数
			
			if(c >= '0' && c <= '9')
				val <<= 4, val |= c - '0';
			else if(c >= 'a' && c <= 'f')
				val <<= 4, val |= c - 'a' + 10;
			else if(c >= 'A' && c <= 'F')
				val <<= 4, val |= c - 'A' + 10;
			else
				continue;

			fval = TRUE;
		}
		else
		{
			//10進数
			
			if(c >= '0' && c <= '9')
			{
				val = val * 10 + (c - '0');
				fval = TRUE;
			}
		}
	}

	//最大サイズ

	if(buf->cursize > 65532)
		mBufReset(buf);

	//調整

	return _sort_charbuf(buf);
}


//==============================
// コード判定処理
//==============================


//漢字
static const uint32_t g_unicode_kanji[] = {
	0x2E80, 0x2FDF, //CJK部首補助,康熙部首
	0x3400, 0x4DBF, //CJK統合漢字拡張A
	0x4E00, 0x9FFF, //CJK統合漢字
	0xF900, 0xFAFF, //CJK互換漢字
	0x20000, 0x2A6DF, //CJK統合漢字拡張B
	0x2A700, 0x2EBEF, //CJK統合漢字拡張C,D,E,F
	0x2F800, 0x2FA1F, //CJK互換漢字追加
	0x30000, 0x3134F,  //CJK統合漢字拡張G
	0
};


/** Unicode が文字種フラグ内のコードかどうか */

mlkbool RegFontCode_checkFlags(uint32_t code,uint16_t flags)
{
	const uint32_t *ps;

	//基本ラテン文字

	if((flags & REGFONT_CHARF_LATIN)
		&& code >= 0 && code <= 0x7f)
		return TRUE;

	//ひらがな

	if(flags & REGFONT_CHARF_HIRA)
	{
		if((code >= 0x3041 && code <= 0x309F)
			|| (code >= 0x1B150 && code <= 0x1B152)) //小書き仮名拡張
			return TRUE;
	}

	//カタカナ

	if(flags & REGFONT_CHARF_KANA)
	{
		if((code >= 0x30A0 && code <= 0x30FF)
			|| (code >= 0x31F0 && code <= 0x31FF)	//片仮名拡張
			|| (code >= 0x1B164 && code <= 0x1B167)) //小書き仮名拡張
			return TRUE;
	}

	//漢字

	if((flags & REGFONT_CHARF_KANJI)
		&& code >= 0x2E80 && code <= 0x3134F)
	{
		for(ps = g_unicode_kanji; *ps; ps += 2)
		{
			if(code >= ps[0] && code <= ps[1])
				return TRUE;
		}
	}

	//句読点など

	if(flags & REGFONT_CHARF_KUDOKUTEN)
	{
		if((code >= 0x2000 && code <= 0x206F)	//一般句読点
			|| (code >= 0x3000 && code <= 0x303F) //CJKの記号及び句読点
			|| (code >= 0xFF00 && code <= 0xFFEF)) //全角半角
			return TRUE;
	}

	//外字

	if(flags & REGFONT_CHARF_GAIJI)
	{
		if((code >= 0xE000 && code <= 0xF8FF)
			|| (code >= 0xF0000 && code <= 0xFFFFD)
			|| (code >= 0x100000 && code <= 0x10FFFD))
			return TRUE;
	}

	return FALSE;
}

/** code が文字コードの範囲内か */

mlkbool RegFontCode_checkBuf(uint32_t *buf,int len,uint32_t code)
{
	uint32_t *end,v1,v2;

	if(!buf) return FALSE;

	end = buf + len;

	while(buf < end)
	{
		v1 = *(buf++);

		if(v1 & REGFONT_CODE_F_RANGE)
		{
			v1 &= REGFONT_CODE_MASK;
			v2 = *(buf++);

			if(code < v1) return FALSE;
			if(code <= v2) return TRUE;
		}
		else
		{
			if(code < v1) return FALSE;
			if(code == v1) return TRUE;
		}
	}

	return FALSE;
}

