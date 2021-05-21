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

/*****************************************
 * mSaveImage
 *****************************************/

#include <string.h>
#include <stdio.h>

#include "mlk.h"
#include "mlk_saveimage.h"
#include "mlk_stdio.h"
#include "mlk_util.h"



/**@ mSaveImage 構造体を初期化
 *
 * @d:ゼロクリアする。 */

void mSaveImage_init(mSaveImage *p)
{
	memset(p, 0, sizeof(mSaveImage));
}


//===========================
// 内部で使われるもの
//===========================


/**@ ファイルを開く
 *
 * @r:FILE *。NULL で失敗。 */

void *mSaveImage_openFile(mSaveImage *p)
{
	switch(p->open.type)
	{
		case MSAVEIMAGE_OPEN_FILENAME:
			return mFILEopen(p->open.filename, "wb");
		case MSAVEIMAGE_OPEN_FP:
			return p->open.fp;
	}

	return NULL;
}

/**@ ファイルを閉じる */

void mSaveImage_closeFile(mSaveImage *p,void *fp)
{
	if(!fp) return;
	
	switch(p->open.type)
	{
		case MSAVEIMAGE_OPEN_FILENAME:
			fclose((FILE *)fp);
			break;
		case MSAVEIMAGE_OPEN_FP:
			fflush((FILE *)fp);
			break;
	}
}

/**@ 解像度を dpi 単位で取得
 *
 * @p:horz,vert 取得できなかった場合、0 にセットされる。
 * @r:FALSE で dpi で取得できなかった */

mlkbool mSaveImage_getDPI(mSaveImage *p,int *horz,int *vert)
{
	int h,v;

	h = p->reso_horz;
	v = p->reso_vert;
	
	switch(p->reso_unit)
	{
		//DPI
		case MSAVEIMAGE_RESOUNIT_DPI:
			*horz = h;
			*vert = v;
			return TRUE;
		//DPM -> DPI
		case MSAVEIMAGE_RESOUNIT_DPM:
			*horz = mDPMtoDPI(h);
			*vert = mDPMtoDPI(v);
			return TRUE;
	}

	*horz = 0;
	*vert = 0;

	return FALSE;
}

/**@ 解像度を dpm 単位で取得
 *
 * @p:horz,vert 取得できなかった場合、0 にセットされる。
 * @r:FALSE で dpm で取得できなかった */

mlkbool mSaveImage_getDPM(mSaveImage *p,int *horz,int *vert)
{
	int h,v;

	h = p->reso_horz;
	v = p->reso_vert;
	
	switch(p->reso_unit)
	{
		//DPI -> DPM
		case MSAVEIMAGE_RESOUNIT_DPI:
			*horz = mDPItoDPM(h);
			*vert = mDPItoDPM(v);
			return TRUE;
		//DPM
		case MSAVEIMAGE_RESOUNIT_DPM:
			*horz = h;
			*vert = v;
			return TRUE;
	}

	*horz = 0;
	*vert = 0;

	return FALSE;
}

/**@ パレットデータを RGBX → RGB (3byte) に変換して作成
 *
 * @d:palette_buf のパレットデータを変換し、新たにバッファを確保してセットする。
 * @r:確保されたバッファ */

uint8_t *mSaveImage_createPaletteRGB(mSaveImage *p)
{
	uint8_t *buf,*ps,*pd;
	int num,i;

	if(!p->palette_buf) return NULL;

	num = p->palette_num;

	buf = (uint8_t *)mMalloc(num * 3);
	if(!buf) return NULL;

	//RGBX => RGB 変換

	ps = p->palette_buf;
	pd = buf;

	for(i = num; i > 0; i--, ps += 4, pd += 3)
	{
		pd[0] = ps[0];
		pd[1] = ps[1];
		pd[2] = ps[2];
	}

	return buf;
}

/**@ 8bit RGB イメージバッファからパレット作成
 *
 * @d:パレットは最大 256 個。
 *
 * @p:ppbuf Y 一列ごとのポインタの配列
 * @p:dst_buf 確保されたパレットバッファのポインタが入る (R-G-B-X)。\
 *   ※257 色以上で、戻り値が FALSE の場合でも、セットされている。
 * @p:dst_palnum パレット数が入る
 * @r:TRUE で 256 色以内。FALSE で、エラーまたは 257 色以上。 */

mlkbool mSaveImage_createPalette_fromRGB8_array(uint8_t **ppbuf,
	int width,int height,uint8_t **dst_buf,int *dst_palnum)
{
	uint8_t *ps,cbuf[4];
	int ix,iy,i,num;
	uint32_t *palbuf,*ppal,c;
	mlkbool ret = TRUE;

	*dst_buf = NULL;
	*dst_palnum = 0;

	//確保

	palbuf = (uint32_t *)mMalloc(256 * 4);
	if(!palbuf) return FALSE;

	//

	num = 0;
	cbuf[3] = 0;

	for(iy = height; iy; iy--)
	{
		ps = *(ppbuf++);
	
		for(ix = width; ix; ix--, ps += 3)
		{
			cbuf[0] = ps[0];
			cbuf[1] = ps[1];
			cbuf[2] = ps[2];

			c = *((uint32_t *)cbuf);
		
			//パレット内に存在するか

			ppal = palbuf;

			for(i = num; i; i--, ppal++)
			{
				if(*ppal == c) break;
			}

			//存在しない場合、追加

			if(!i)
			{
				//256色でさらに追加の場合
				
				if(num == 256)
				{
					ret = FALSE;
					goto END;
				}
			
				palbuf[num] = c;
				num++;
			}
		}
	}

END:
	*dst_buf = (uint8_t *)palbuf;
	*dst_palnum = num;

	return ret;
}

/**@ 8bit RGB イメージバッファを、パレットのイメージデータに変換
 *
 * @d:パレットの 4byte 目は 0 でセットされていること。\
 *  パレット内に色がない場合は、0 でセットされる。 */

void mSaveImage_convertImage_RGB8array_to_palette(uint8_t **ppbuf,int width,int height,
	uint8_t *palbuf,int palnum)
{
	uint8_t *pd,*ps,cbuf[4];
	int ix,iy,i;
	uint32_t *ppal,c;

	cbuf[3] = 0;

	for(iy = height; iy; iy--)
	{
		pd = *(ppbuf++);
		ps = pd;
	
		for(ix = width; ix; ix--, ps += 3)
		{
			cbuf[0] = ps[0];
			cbuf[1] = ps[1];
			cbuf[2] = ps[2];

			c = *((uint32_t *)cbuf);

			//パレットから検索

			ppal = (uint32_t *)palbuf;

			for(i = 0; i < palnum; i++, ppal++)
			{
				if(*ppal == c) break;
			}

			//ない場合は、0

			if(i == palnum) i = 0;

			*(pd++) = i;
		}
	}
}


