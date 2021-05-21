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

/***************************
 * パレットリスト
 ***************************/

#include <stdio.h>
#include <string.h>

#include "mlk_gui.h"
#include "mlk_list.h"
#include "mlk_str.h"
#include "mlk_stdio.h"
#include "mlk_util.h"

#include "def_draw.h"

#include "palettelist.h"


//[!] データが変更された (ファイルに保存する必要がある) 時は、フラグを ON にする。

//----------------

#define _CONFIG_FILENAME "colpalette.dat"

#define _SET_MODIFY  APPDRAW->col.pal_fmodify = TRUE

//----------------


//=========================
// PaletteList
//=========================


/** 変更フラグを ON */

void PaletteList_setModify(void)
{
	_SET_MODIFY;
}

/* 色バッファを確保 */

static mlkbool _alloc_palbuf(PaletteListItem *pi,int num)
{
	mFree(pi->palbuf);

	if(num > PALETTE_MAX_COLNUM) num = PALETTE_MAX_COLNUM;

	pi->palbuf = (uint8_t *)mMalloc(num * 3);
	if(!pi->palbuf) return FALSE;

	pi->palnum = num;

	return TRUE;
}

/* バッファに色数追加 (最大数に制限される場合あり) */

static mlkbool _append_palbuf(PaletteListItem *pi,int num)
{
	uint8_t *buf;

	num += pi->palnum;

	if(num > PALETTE_MAX_COLNUM) num = PALETTE_MAX_COLNUM;

	//

	buf = (uint8_t *)mRealloc(pi->palbuf, num * 3);
	if(!buf) return FALSE;

	pi->palbuf = buf;
	pi->palnum = num;

	return TRUE;
}

/* 設定ファイルを開く */

static FILE *_open_configfile(const char *mode)
{
	mStr str = MSTR_INIT;
	FILE *fp;
	
	mGuiGetPath_config(&str, _CONFIG_FILENAME);

	fp = mFILEopen(str.buf, mode);

	mStrFree(&str);

	return fp;
}

/* アイテム破棄ハンドラ */

static void _destroy_item(mList *list,mListItem *item)
{
	PaletteListItem *pi = (PaletteListItem *)item;

	mFree(pi->name);
	mFree(pi->palbuf);
}

/** リスト初期化 */

void PaletteList_init(mList *list)
{
	list->item_destroy = _destroy_item;
}

/** 追加 */

PaletteListItem *PaletteList_add(mList *list,const char *name)
{
	PaletteListItem *pi;

	pi = (PaletteListItem *)mListAppendNew(list, sizeof(PaletteListItem));
	if(!pi) return NULL;

	pi->name = mStrdup(name);
	pi->cellw = pi->cellh = 16;

	_alloc_palbuf(pi, 256);

	PaletteListItem_setAllColor(pi, 255, 255, 255);

	return pi;
}

/** 設定ファイルから読み込み */

void PaletteList_loadConfig(mList *list)
{
	FILE *fp;
	PaletteListItem *pi;
	uint8_t ver,buf[10];
	uint32_t palnum;
	uint16_t num,cellw,cellh,xmaxnum;
	int size;

	fp = _open_configfile("rb");
	if(!fp) return;

	if(mFILEreadStr_compare(fp, "AZPTCONFPAL")
		|| mFILEreadByte(fp, &ver)
		|| ver != 0
		|| mFILEreadBE16(fp, &num))
		goto END;

	for(; num; num--)
	{
		if(fread(buf, 1, 10, fp) != 10) break;

		mGetBuf_format(buf, ">ihhh", &palnum, &cellw, &cellh, &xmaxnum);

		size = palnum * 3;

		//作成
	
		pi = (PaletteListItem *)mListAppendNew(list, sizeof(PaletteListItem));
		if(!pi) break;

		pi->palnum = palnum;
		pi->cellw = cellw;
		pi->cellh = cellh;
		pi->xmaxnum = xmaxnum;

		if(mFILEreadStr_lenBE16(fp, &pi->name) == -1) break;

		pi->palbuf = (uint8_t *)mMalloc(size);

		//パレット

		if(fread(pi->palbuf, 1, size, fp) != size)
			break;
	}

END:
	fclose(fp);
}

/** 設定ファイルに保存 */

void PaletteList_saveConfig(mList *list)
{
	FILE *fp;
	PaletteListItem *pi;
	uint8_t buf[12];
	int len;

	fp = _open_configfile("wb");
	if(!fp) return;

	fputs("AZPTCONFPAL", fp);
	mFILEwriteByte(fp, 0);

	mFILEwriteBE16(fp, list->num);

	MLK_LIST_FOR(*list, pi, PaletteListItem)
	{
		//値
		
		len = mStrlen(pi->name);
		if(len > UINT16_MAX) len = UINT16_MAX;
		
		mSetBuf_format(buf, ">ihhhh",
			pi->palnum, pi->cellw, pi->cellh, pi->xmaxnum, len);

		fwrite(buf, 1, 12, fp);

		//名前

		if(len)
			fwrite(pi->name, 1, len, fp);

		//パレット

		fwrite(pi->palbuf, 1, pi->palnum * 3, fp);
	}

	fclose(fp);
}


//=========================
// PaletteListItem
//=========================


/** 色数を変更 */

void PaletteListItem_resize(PaletteListItem *pi,int num)
{
	uint8_t *buf;
	int srcnum;

	if(pi->palnum == num) return;

	buf = (uint8_t *)mRealloc(pi->palbuf, num * 3);
	if(buf)
	{
		srcnum = pi->palnum;
	
		pi->palbuf = buf;
		pi->palnum = num;

		//拡張部分をクリア

		if(num > srcnum)
			memset(buf + srcnum * 3, 255, (num - srcnum) * 3);
	}
}

/** 指定位置の色を取得 */

uint32_t PaletteListItem_getColor(PaletteListItem *pi,int no)
{
	uint8_t *ps;

	ps = pi->palbuf + no * 3;

	return MLK_RGB(ps[0], ps[1], ps[2]);
}

/** 指定位置に色をセット */

void PaletteListItem_setColor(PaletteListItem *pi,int no,uint32_t col)
{
	uint8_t *pd;

	pd = pi->palbuf + no * 3;

	pd[0] = MLK_RGB_R(col);
	pd[1] = MLK_RGB_G(col);
	pd[2] = MLK_RGB_B(col);

	_SET_MODIFY;
}

/** すべて指定色にセット */

void PaletteListItem_setAllColor(PaletteListItem *pi,uint8_t r,uint8_t g,uint8_t b)
{
	uint8_t *pd;
	int i;

	if(!pi) return;

	pd = pi->palbuf;

	for(i = pi->palnum; i > 0; i--, pd += 3)
	{
		pd[0] = r;
		pd[1] = g;
		pd[2] = b;
	}

	_SET_MODIFY;
}

/** 指定間をグラデーション */

void PaletteListItem_gradation(PaletteListItem *pi,int no1,int no2)
{
	int i,j,len;
	uint8_t *p1,*p2,*pd;
	double d;

	if(no1 > no2)
		i = no1, no1 = no2, no2 = i;

	len = no2 - no1;
	if(len < 2) return;

	//

	p1 = pi->palbuf + no1 * 3;
	p2 = pi->palbuf + no2 * 3;
	pd = p1 + 3;

	for(i = 1; i < len; i++, pd += 3)
	{
		d = (double)i / len;
		
		for(j = 0; j < 3; j++)
			pd[j] = (int)((p2[j] - p1[j]) * d + p1[j] + 0.5);
	}

	_SET_MODIFY;
}

/** 32bit画像からパレット作成 */

void PaletteListItem_setFromImage(PaletteListItem *pi,uint8_t *buf,int w,int h)
{
	uint32_t *palbuf,*pp,col;
	int num,ix,iy,i;

	//作業用に最大数分を確保

	palbuf = (uint32_t *)mMalloc(PALETTE_MAX_COLNUM * 4);
	if(!palbuf) return;

	//取得

	num = 0;

	for(iy = h; iy; iy--)
	{
		for(ix = w; ix; ix--, buf += 4)
		{
			col = MLK_RGB(buf[0], buf[1], buf[2]);

			//すでにあるか

			for(i = num, pp = palbuf; i; i--, pp++)
			{
				if(*pp == col) break;
			}

			//追加

			if(i == 0)
			{
				if(num == PALETTE_MAX_COLNUM) goto END;

				palbuf[num++] = col;
			}
		}
	}

END:
	//パレットセット

	if(_alloc_palbuf(pi, num))
	{
		buf = pi->palbuf;
		pp = palbuf;
	
		for(i = num; i > 0; i--, buf += 3)
		{
			col = *(pp++);

			buf[0] = MLK_RGB_R(col);
			buf[1] = MLK_RGB_G(col);
			buf[2] = MLK_RGB_B(col);
		}

		_SET_MODIFY;
	}

	mFree(palbuf);
}


//===============================
// パレットファイル読み込み
//===============================


/* バッファセット
 *
 * pnum: 色数。実際にセットする色数が入る。
 * return: 色をセットする先頭位置 (NULL でエラー) */

static uint8_t *_load_setbuf(PaletteListItem *pi,int *pnum,mlkbool append)
{
	int srcnum;

	if(append)
	{
		//追加
		
		srcnum = pi->palnum;
	
		if(!_append_palbuf(pi, *pnum)) return FALSE;

		*pnum = pi->palnum - srcnum;

		return pi->palbuf + srcnum * 3;
	}
	else
	{
		if(!_alloc_palbuf(pi, *pnum)) return FALSE;

		*pnum = pi->palnum;

		return pi->palbuf;
	}
}

/* APL 読み込み (LE) */

static mlkbool _loadpal_apl(PaletteListItem *pi,const char *filename,mlkbool append)
{
	FILE *fp;
	uint8_t ver,c[4],*pd;
	uint16_t num16;
	int i,num;
	mlkbool ret = FALSE;

	fp = mFILEopen(filename, "rb");
	if(!fp) return FALSE;

	if(mFILEreadStr_compare(fp, "AZPPAL")
		|| mFILEreadByte(fp, &ver)
		|| ver != 0
		|| mFILEreadLE16(fp, &num16)
		|| num16 == 0)
		goto END;

	//データ

	num = num16;

	pd = _load_setbuf(pi, &num, append);
	if(!pd) goto END;

	for(i = num; i > 0; i--, pd += 3)
	{
		if(fread(c, 1, 4, fp) != 4) break;

		pd[0] = c[2];
		pd[1] = c[1];
		pd[2] = c[0];
	}

	ret = TRUE;
END:
	fclose(fp);

	return ret;
}

/** ACO 読み込み (BE) */

static mlkbool _loadpal_aco(PaletteListItem *pi,const char *filename,mlkbool append)
{
	FILE *fp;
	uint16_t ver,num16,len;
	uint8_t dat[10],*pd;
	mlkbool ret = FALSE;
	int i,num;

	fp = mFILEopen(filename, "rb");
	if(!fp) return FALSE;

	//バージョン

	if(mFILEreadBE16(fp, &ver)
		|| (ver != 1 && ver != 2)
		|| mFILEreadBE16(fp, &num16)
		|| num16 == 0)
		goto END;

	//データ

	num = num16;

	pd = _load_setbuf(pi, &num, append);
	if(!pd) goto END;

	for(i = num; i > 0; i--, pd += 3)
	{
		//[2byte] color space (0:RGB)
		//[2byte x 4] R,G,B,Z

		if(fread(dat, 1, 10, fp) != 10) break;

		//ver2 時、色名をスキップ

		if(ver == 2)
		{
			if(fseek(fp, 2, SEEK_CUR)
				|| mFILEreadBE16(fp, &len)
				|| fseek(fp, len << 1, SEEK_CUR))
				break;
		}

		//RGB (上位バイトのみ)

		pd[0] = dat[2];
		pd[1] = dat[4];
		pd[2] = dat[6];
	}

	ret = TRUE;
END:
	fclose(fp);

	return ret;
}

/** パレットファイル読み込み (APL/ACO)
 *
 * append: TRUE で追加読込 */

mlkbool PaletteListItem_loadFile(PaletteListItem *pi,const char *filename,mlkbool append)
{
	mStr ext = MSTR_INIT;
	mlkbool ret;

	//拡張子から判定

	mStrPathGetExt(&ext, filename);

	if(mStrCompareEq_case(&ext, "apl"))
		ret = _loadpal_apl(pi, filename, append);
	else if(mStrCompareEq_case(&ext, "aco"))
		ret = _loadpal_aco(pi, filename, append);
	else
		ret = FALSE;

	mStrFree(&ext);

	if(ret) _SET_MODIFY;

	return ret;
}


//===============================
// パレットファイル保存
//===============================


/** APL ファイルに保存 */

mlkbool PaletteListItem_saveFile_apl(PaletteListItem *pi,const char *filename)
{
	FILE *fp;
	int i;
	uint8_t d[4],*ps;

	fp = mFILEopen(filename, "wb");
	if(!fp) return FALSE;

	fputs("AZPPAL", fp);

	mFILEwriteByte(fp, 0);
	mFILEwriteLE16(fp, pi->palnum);

	ps = pi->palbuf;
	d[3] = 0;

	for(i = pi->palnum; i > 0; i--, ps += 3)
	{
		d[0] = ps[2];
		d[1] = ps[1];
		d[2] = ps[0];

		fwrite(d, 1, 4, fp);
	}

	fclose(fp);

	return TRUE;
}

/** ACO ファイルに保存 */

mlkbool PaletteListItem_saveFile_aco(PaletteListItem *pi,const char *filename)
{
	FILE *fp;
	int i;
	uint8_t d[10],*ps;

	fp = mFILEopen(filename, "wb");
	if(!fp) return FALSE;

	mFILEwriteBE16(fp, 1); //ver
	mFILEwriteBE16(fp, pi->palnum);

	ps = pi->palbuf;
	memset(d, 0, 10);

	for(i = pi->palnum; i > 0; i--, ps += 3)
	{
		d[2] = ps[0];
		d[4] = ps[1];
		d[6] = ps[2];

		fwrite(d, 1, 10, fp);
	}

	fclose(fp);

	return TRUE;
}

