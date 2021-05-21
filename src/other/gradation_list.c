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

/*************************************
 * グラデーションデータ
 *************************************/

#include <string.h>
#include <stdio.h>

#include "mlk_gui.h"
#include "mlk_str.h"
#include "mlk_list.h"
#include "mlk_pixbuf.h"
#include "mlk_stdio.h"

#include "colorvalue.h"
#include "gradation_list.h"


/*

 - データが更新された時のみ終了時に保存される。

 - データを更新したら fmodify_grad_list を TRUE にすること。

 [--- グラデーションデータ ---] (BigEndian)

 1byte : フラグ (0bit:繰り返す 1bit:単色)
 1byte : 色の数
 1byte : Aの数
 <色>
  2byte : 位置 (0-1024) [14-15bit:色タイプ (0:描画色 1:背景色 2:指定色)]
  3byte : R,G,B
 <A>
  2byte : 位置
  2byte : A (0-1024)

*/

//------------------

#define _FILENAME_GRAD  "grad.dat"

#define _GETVAL16(p)  ((*(p) << 8) | *((p) + 1))
#define _GETPOS(p)    (((*(p) << 8) | *((p) + 1)) & GRAD_DAT_POS_MASK)

//描画色->背景色 のグラデーションデータ (先頭のフラグは含まない)

static const uint8_t g_data_drawbkcol[] = {
	2,2,
	0,0, 0,0,0,		//描画色
	0x44,0, 0,0,0,	//背景色
	0,0, 4,0, 4,0, 4,0	//アルファ値
};

//黒->白のデータ

static const uint8_t g_data_black_white[] = {
	2,2,
	0x80,0, 0,0,0,	//黒
	0x84,0, 255,255,255,//白
	0,0, 4,0, 4,0, 4,0	//アルファ値
};

//白->黒のデータ

static const uint8_t g_data_white_black[] = {
	2,2,
	0x80,0, 255,255,255,//白
	0x84,0, 0,0,0,	//黒
	0,0, 4,0, 4,0, 4,0	//アルファ値
};

//------------------


//==========================
// 定義データ取得
//==========================


/** 描画色->背景色のグラデーションデータを取得 (先頭のフラグ(1byte)は除く) */

const uint8_t *Gradation_getData_draw_to_bkgnd(void)
{
	return g_data_drawbkcol;
}

/** 黒->白のグラデーションデータ取得 */

const uint8_t *Gradation_getData_black_to_white(void)
{
	return g_data_black_white;
}

/** 白->黒のグラデーションデータ取得 */

const uint8_t *Gradation_getData_white_to_black(void)
{
	return g_data_white_black;
}


//==========================
// 描画用のデータに変換
//==========================


/** グラデーションデータを、描画時用のデータに変換 (8bit)
 *
 * src: 先頭のフラグデータ(1byte)は除く */

uint8_t *Gradation_convertData_draw_8bit(const uint8_t *src,RGBcombo *drawcol,RGBcombo *bkgndcol)
{
	uint8_t *dstbuf,*pd;
	int i,n,colnum,anum;

	colnum = src[0];
	anum = src[1];

	dstbuf = pd = (uint8_t *)mMalloc(2 + colnum * 5 + anum * 3);
	if(!dstbuf) return NULL;

	//色,Aの数

	*(pd++) = colnum;
	*(pd++) = anum;

	//RGBデータ

	src += 2;

	for(i = colnum; i > 0; i--, pd += 5, src += 5)
	{
		//位置 (15bit 固定小数)
		
		n = _GETVAL16(src);
		
		*((uint16_t *)pd) = (n & GRAD_DAT_POS_MASK) << (15 - GRAD_DAT_POS_BIT);

		//RGB (0:描画色,1:背景色,2:指定色)

		n >>= 14;

		switch(n)
		{
			case 0:
				pd[2] = drawcol->c8.r;
				pd[3] = drawcol->c8.g;
				pd[4] = drawcol->c8.b;
				break;
			case 1:
				pd[2] = bkgndcol->c8.r;
				pd[3] = bkgndcol->c8.g;
				pd[4] = bkgndcol->c8.b;
				break;
			default:
				pd[2] = src[2];
				pd[3] = src[3];
				pd[4] = src[4];
				break;
		}
	}

	//Aデータ

	for(i = anum; i > 0; i--, pd += 3, src += 4)
	{
		*((uint16_t *)pd) = _GETVAL16(src) << (15 - GRAD_DAT_POS_BIT);

		pd[2] = _GETVAL16(src + 2) * 255 >> GRAD_DAT_ALPHA_BIT;
	}
	
	return dstbuf;
}

/** グラデーションデータを、描画時用のデータに変換 (16bit)
 *
 * src: 先頭のフラグデータ(1byte)は除く */

uint8_t *Gradation_convertData_draw_16bit(const uint8_t *src,RGBcombo *drawcol,RGBcombo *bkgndcol)
{
	uint8_t *dstbuf;
	uint16_t *pd16;
	int i,n,colnum,anum;

	colnum = src[0];
	anum = src[1];

	dstbuf = (uint8_t *)mMalloc(2 + colnum * 8 + anum * 4);
	if(!dstbuf) return NULL;

	//色,Aの数

	dstbuf[0] = colnum;
	dstbuf[1] = anum;

	//RGBデータ

	pd16 = (uint16_t *)(dstbuf + 2);
	src += 2;

	for(i = colnum; i > 0; i--, pd16 += 4, src += 5)
	{
		//位置 (15bit 固定小数)
		
		n = _GETVAL16(src);
		
		*pd16 = (n & GRAD_DAT_POS_MASK) << (15 - GRAD_DAT_POS_BIT);

		//RGB (0:描画色,1:背景色,2:指定色)

		n >>= 14;

		switch(n)
		{
			case 0:
				pd16[1] = drawcol->c16.r;
				pd16[2] = drawcol->c16.g;
				pd16[3] = drawcol->c16.b;
				break;
			case 1:
				pd16[1] = bkgndcol->c16.r;
				pd16[2] = bkgndcol->c16.g;
				pd16[3] = bkgndcol->c16.b;
				break;
			//8bit -> 16bit
			default:
				pd16[1] = Color8bit_to_16bit(src[2]);
				pd16[2] = Color8bit_to_16bit(src[3]);
				pd16[3] = Color8bit_to_16bit(src[4]);
				break;
		}
	}

	//Aデータ

	for(i = anum; i > 0; i--, pd16 += 2, src += 4)
	{
		pd16[0] = _GETVAL16(src) << (15 - GRAD_DAT_POS_BIT);
		pd16[1] = _GETVAL16(src + 2) << (15 - GRAD_DAT_ALPHA_BIT);
	}
	
	return dstbuf;
}


/**********************************
 * グラデーションリストデータ
 **********************************/


/* リストにアイテム追加 & データバッファ確保 */

static GradItem *_additem_allocbuf(mList *list,int bufsize)
{
	GradItem *pi;

	if(list->num >= GRAD_LIST_MAX_NUM) return NULL;

	pi = (GradItem *)mListAppendNew(list, sizeof(GradItem));
	if(!pi) return NULL;
	
	pi->size = bufsize;

	pi->buf = (uint8_t *)mMalloc(bufsize);
	if(!pi->buf)
	{
		mListDelete(list, MLISTITEM(pi)); 
		pi = NULL;
	}

	return pi;
}

/* デフォルトのグラデーションデータセット (黒->白) */

static mlkbool _item_set_default(GradItem *pi)
{
	uint8_t *buf;
	int size;

	size = 1 + sizeof(g_data_black_white);

	pi->buf = buf = (uint8_t *)mMalloc(size);
	if(!buf) return FALSE;

	buf[0] = 0;

	memcpy(buf + 1, g_data_black_white, size - 1);

	pi->size = size;

	return TRUE;
}

/* 未使用の ID を取得 */

static int _get_unused_id(mList *list)
{
	GradItem *pi;
	int id,f;

	for(id = 0; id < 256; id++)
	{
		f = 0;
		
		MLK_LIST_FOR(*list, pi, GradItem)
		{
			if(pi->id == id) { f = 1; break; }
		}

		if(!f) return id;
	}

	return 0;
}

/* アイテム破棄ハンドラ */

static void _destroy_item(mList *list,mListItem *item)
{
	mFree(GRADITEM(item)->buf);
}


//-----------------


/** 初期化 */

void GradationList_init(mList *list)
{
	list->item_destroy = _destroy_item;
}

/** 新規データ追加 */

GradItem *GradationList_newItem(mList *list)
{
	GradItem *pi;
	int id;

	if(list->num >= GRAD_LIST_MAX_NUM) return NULL;

	//ID 取得

	id = _get_unused_id(list);

	//追加

	pi = (GradItem *)mListAppendNew(list, sizeof(GradItem));
	if(!pi) return NULL;

	pi->id = id;

	if(!_item_set_default(pi))
	{
		mListDelete(list, MLISTITEM(pi));
		return NULL;
	}

	return pi;
}

/** アイテム複製 */

GradItem *GradationList_copyItem(mList *list,GradItem *item)
{
	GradItem *pi;
	int id;

	id = _get_unused_id(list);

	pi = _additem_allocbuf(list, item->size);
	if(!pi) return NULL;

	pi->id = id;

	memcpy(pi->buf, item->buf, item->size);

	return pi;
}

/** ID から検索 */

GradItem *GradationList_getItem_id(mList *list,int id)
{
	GradItem *pi;

	MLK_LIST_FOR(*list, pi, GradItem)
	{
		if(pi->id == id) return pi;
	}

	return NULL;
}

/** ID から検索して、データバッファを取得 */

uint8_t *GradationList_getBuf_fromID(mList *list,int id)
{
	GradItem *pi;

	pi = GradationList_getItem_id(list, id);

	return (pi)? pi->buf: NULL;
}


//==========================
// GradItem
//==========================


/** グラデーションデータをセット (編集完了時) */

void GradItem_setData(GradItem *p,
	uint8_t flags,int colnum,int anum,uint8_t *colbuf,uint8_t *abuf)
{
	uint8_t *buf;
	int colsize,asize;

	mFree(p->buf);

	colsize = colnum * 5;
	asize = anum * 4;

	//確保

	p->size = 3 + colsize + asize;
	
	p->buf = (uint8_t *)mMalloc(p->size);
	if(!p->buf) return;

	//データセット

	buf = p->buf;

	buf[0] = flags;
	buf[1] = colnum;
	buf[2] = anum;
	buf += 3;

	memcpy(buf, colbuf, colsize);
	memcpy(buf + colsize, abuf, asize);
}


//==========================
// mPixbuf 描画
//==========================


/* 色データから 8bit RGB 色と位置の値を取得
 *
 * return: 位置の値 */

static int _get_rgb_dat(const uint8_t *buf,RGB8 *dst)
{
	int type,val;

	val = _GETVAL16(buf);
	type = val >> 14;

	if(type == 0)
		//描画色
		dst->r = dst->g = dst->b = 0;
	else if(type == 1)
		//背景色
		dst->r = dst->g = dst->b = 255;
	else
	{
		//指定色
		
		dst->r = buf[2];
		dst->g = buf[3];
		dst->b = buf[4];
	}

	return val & GRAD_DAT_POS_MASK;
}

/* mPixbuf にグラデーション描画
 *
 * 描画色は黒、背景色は白とする。
 * 4x4 のチェック柄を背景にしてアルファ合成。 */

void GradItem_drawPixbuf(mPixbuf *pixbuf,
	int x,int y,int w,int h,uint8_t *colbuf,uint8_t *abuf,int single_col)
{
	int ix,iy,bpp,pos,n1,n2,a,xf,yy,i,colpos_l,colpos_r,apos_l,apos_r,alpha_l,alpha_r;
	mBox box;
	uint8_t *pd,*pdY;
	RGB8 *pcol_bk;
	RGB8 coll,colr,colsrc,coldst,
		col_bkgnd[2] = {{{0xff,0xe8,0xeb}},{{0xff,0xc8,0xce}}};
	mFuncPixbufSetBuf setbuf;

	if(!mPixbufClip_getBox_d(pixbuf, &box, x, y, w, h))
		return;

	pd = mPixbufGetBufPtFast(pixbuf, box.x, box.y);
	x = box.x - x;
	y = box.y - y;

	bpp = pixbuf->pixel_bytes;

	mPixbufGetFunc_setbuf(pixbuf, &setbuf);

	//最初の値

	colpos_l = _get_rgb_dat(colbuf, &coll);
	colpos_r = _get_rgb_dat(colbuf + 5, &colr);

	apos_l = 0;
	alpha_l = _GETVAL16(abuf + 2);

	apos_r = _GETVAL16(abuf + 4);
	alpha_r = _GETVAL16(abuf + 6);

	//高さ最大 8px 分を描画
	// :チェック柄が 4x4 のため、8px 分の描画があれば、後は繰り返しになる。

	h = box.h;
	if(h > 8) h = 8;

	for(ix = box.w; ix; ix--, x++, pd += bpp)
	{
		pos = (x << GRAD_DAT_POS_BIT) / (w - 1);

		//------ RGB

		while(pos > colpos_r)
		{
			colbuf += 5;
			
			colpos_l = colpos_r;
			coll = colr;

			colpos_r = _get_rgb_dat(colbuf, &colr);
		}

		if(single_col)
			colsrc = coll;
		else
		{
			n1 = pos - colpos_l;
			n2 = colpos_r - colpos_l;

			for(i = 0; i < 3; i++)
				colsrc.ar[i] = (colr.ar[i] - coll.ar[i]) * n1 / n2 + coll.ar[i];
		}

		//----- Alpha

		while(pos > apos_r)
		{
			abuf += 4;

			apos_l = apos_r;
			alpha_l = alpha_r;

			apos_r = _GETVAL16(abuf);
			alpha_r = _GETVAL16(abuf + 2);
		}

		a = (alpha_r - alpha_l) * (pos - apos_l) / (apos_r - apos_l) + alpha_l;

		//----- 縦に描画

		pdY = pd;
		xf = (x & 7) >> 2;
		yy = y;

		for(iy = h; iy; iy--, yy++, pdY += pixbuf->line_bytes)
		{
			pcol_bk = col_bkgnd + (xf ^ ((yy & 7) >> 2));

			for(i = 0; i < 3; i++)
				coldst.ar[i] = (((colsrc.ar[i] - pcol_bk->ar[i]) * a) >> GRAD_DAT_ALPHA_BIT) + pcol_bk->ar[i];

			(setbuf)(pdY, mRGBtoPix_sep(coldst.r, coldst.g, coldst.b));
		}
	}

	//残りの Y はコピー

	n1 = pixbuf->line_bytes;
	n2 = box.w * bpp;
	pdY = mPixbufGetBufPtFast(pixbuf, box.x, box.y);
	pd  = pdY + n1 * h;

	for(h = box.h - h; h > 0; h--)
	{
		memcpy(pd, pdY, n2);

		pd += n1;
		pdY += n1;
	}
}


//============================
// 読み書き
//============================


/** 設定ファイルから読み込み */

void GradationList_loadConfig(mList *list)
{
	mStr str = MSTR_INIT;

	mGuiGetPath_config(&str, _FILENAME_GRAD);

	GradationList_loadFile(list, str.buf);

	mStrFree(&str);
}

/** 設定ファイルに保存 */

void GradationList_saveConfig(mList *list)
{
	mStr str = MSTR_INIT;

	mGuiGetPath_config(&str, _FILENAME_GRAD);

	GradationList_saveFile(list, str.buf);

	mStrFree(&str);
}

/** ファイルから読み込み */

void GradationList_loadFile(mList *list,const char *filename)
{
	FILE *fp;
	GradItem *pi;
	uint8_t ver,num,id,head[3];
	int size;

	fp = mFILEopen(filename, "rb");
	if(!fp) return;

	//

	if(mFILEreadStr_compare(fp, "AZPLGRAD")
		|| mFILEreadByte(fp, &ver)
		|| ver > 1
		|| mFILEreadByte(fp, &num))
	{
		fclose(fp);
		return;
	}

	if(num > GRAD_LIST_MAX_NUM)
		num = GRAD_LIST_MAX_NUM;

	//データ

	id = 0;

	for(; num; num--)
	{
		//ID

		if(ver == 1)
		{
			if(mFILEreadByte(fp, &id))
				break;
		}
	
		//3byte

		if(mFILEreadOK(fp, head, 3))
			break;

		size = head[1] * 5 + head[2] * 4;
	
		//アイテム追加 & 確保

		pi = _additem_allocbuf(list, 3 + size);
		if(!pi) break;

		pi->id = id;

		memcpy(pi->buf, head, 3);

		//データ読み込み

		if(mFILEreadOK(fp, pi->buf + 3, size))
			break;

		//

		if(ver == 0) id++;
	}

	fclose(fp);
}

/** ファイルに保存 */

void GradationList_saveFile(mList *list,const char *filename)
{
	FILE *fp;
	GradItem *pi;

	fp = mFILEopen(filename, "wb");
	if(!fp) return;

	fputs("AZPLGRAD", fp);
	
	mFILEwriteByte(fp, 1);

	mFILEwriteByte(fp, list->num);

	MLK_LIST_FOR(*list, pi, GradItem)
	{
		mFILEwriteByte(fp, pi->id);
	
		fwrite(pi->buf, 1, pi->size, fp);
	}

	fclose(fp);
}

