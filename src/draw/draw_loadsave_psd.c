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
 * PSD 読み書き
 *****************************************/

#include <string.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_popup_progress.h"
#include "mlk_psd.h"
#include "mlk_file.h"
#include "mlk_list.h"
#include "mlk_rectbox.h"
#include "mlk_imagebuf.h"

#include "def_macro.h"
#include "def_config.h"
#include "def_draw.h"
#include "def_saveopt.h"
#include "def_tileimage.h"

#include "tileimage.h"
#include "layerlist.h"
#include "layeritem.h"
#include "imagecanvas.h"

#include "draw_main.h"


//--------------------

enum
{
	_TYPE_LAYER,
	_TYPE_RGB,
	_TYPE_GRAY,
	_TYPE_MONO
};

#define _RGB_TO_GRAY(r,g,b)  ((r * 77 + g * 150 + b * 29) >> 8)

//--------------------

//添字:AzPainter 合成モード, 値:PSD 合成モード
static const uint32_t g_blendmode[] = {
	MPSD_BLENDMODE_NORMAL, MPSD_BLENDMODE_MULTIPLY, MPSD_BLENDMODE_LINEAR_DODGE,
	MPSD_BLENDMODE_SUBTRACT, MPSD_BLENDMODE_SCREEN, MPSD_BLENDMODE_OVERLAY,
	MPSD_BLENDMODE_HARD_LIGHT, MPSD_BLENDMODE_SOFT_LIGHT, MPSD_BLENDMODE_DODGE,
	MPSD_BLENDMODE_BURN, MPSD_BLENDMODE_LINEAR_BURN, MPSD_BLENDMODE_VIVID_LIGHT,
	MPSD_BLENDMODE_LINEAR_LIGHT, MPSD_BLENDMODE_PIN_LIGHT, MPSD_BLENDMODE_DARKEN,
	MPSD_BLENDMODE_LIGHTEN, MPSD_BLENDMODE_DIFFERENCE,
	//"加算"と重複する
	MPSD_BLENDMODE_LINEAR_DODGE, MPSD_BLENDMODE_LINEAR_DODGE,
	0
};

//--------------------


//******************************
// 読み込み
//******************************


/* レイヤ読み込み: チャンネルイメージ全体の読み込み */

static mlkerr _load_layer_image_channel(mPSDLoad *psd,int layerno,int chid,
	mBox *boximg,uint8_t **ppimg,mPopupProgress *prog)
{
	int iy,n;
	mlkerr ret;

	//開始

	ret = mPSDLoad_setLayerImageCh_id(psd, layerno, chid, NULL);

	if(ret == -2)
	{
		//チャンネルがない場合

		if(chid == MPSD_CHID_ALPHA)
		{
			//[ALPHA] 範囲内すべて不透明

			n = boximg->w * (APPDRAW->imgbits / 8);

			for(iy = boximg->h; iy; iy--, ppimg++)
				memset(*ppimg, 0xff, n);
		}

		mPopupProgressThreadAddPos(prog, 10);

		return MLKERR_OK;
	}
	else if(ret)
		return ret;

	//読み込み

	mPopupProgressThreadSubStep_begin(prog, 10, boximg->h);

	for(iy = boximg->h; iy; iy--, ppimg++)
	{
		ret = mPSDLoad_readLayerImageRowCh(psd, *ppimg);
		if(ret) return ret;

		mPopupProgressThreadSubStep_inc(prog);
	}

	return MLKERR_OK;
}

/* レイヤマスクをアルファチャンネルイメージに適用 */

static mlkerr _set_layermask_image(mPSDLoad *psd,mPSDImageInfo *info,mBox *boximg,uint8_t **ppimg)
{
	uint8_t *rowbuf,*pd8,*ps8;
	uint16_t *pd16,*ps16;
	int ix,iy,bits;
	mlkerr ret;

	bits = APPDRAW->imgbits;

	//Y1行バッファ
	// [!] イメージ・マスクの大きい方の分のサイズが必要

	ix = info->rowsize;
	if(ix < info->rowsize_mask) ix = info->rowsize_mask;

	rowbuf = (uint8_t *)mMalloc(ix);
	if(!rowbuf) return MLKERR_ALLOC;

	//

	for(iy = boximg->h; iy; iy--, ppimg++)
	{
		//読み込み
		
		ret = mPSDLoad_readLayerMaskImageRow(psd, rowbuf);
		if(ret) break;

		//適用

		if(bits == 8)
		{
			pd8 = *ppimg;
			ps8 = rowbuf;
			
			for(ix = boximg->w; ix; ix--, pd8++, ps8++)
			{
				if(*pd8 > *ps8)
					*pd8 = *ps8;
			}
		}
		else
		{
			pd16 = (uint16_t *)*ppimg;
			ps16 = (uint16_t *)rowbuf;
			
			for(ix = boximg->w; ix; ix--, pd16++, ps16++)
			{
				if(*pd16 > *ps16)
					*pd16 = *ps16;
			}
		}
	}

	mFree(rowbuf);

	return ret;
}

/* レイヤ読み込み: アルファ値読み込み */

static mlkerr _load_layer_image_alpha(mPSDLoad *psd,int layerno,
	mPSDLayer *info,TileImage *img,uint8_t **ppimg,mPopupProgress *prog)
{
	mBox box;
	mPSDImageInfo imginfo;
	mlkerr ret;

	box = info->box_img;

	//アルファチャンネル読み込み

	ret = _load_layer_image_channel(psd, layerno, MPSD_CHID_ALPHA, &box, ppimg, prog);
	if(ret) return ret;

	//レイヤマスクがある場合、アルファ値に適用

	if(!(info->mask_flags & MPSD_LAYER_MASK_FLAGS_DISABLE))
	{
		ret = mPSDLoad_setLayerImageCh_id(psd, layerno, MPSD_CHID_MASK, &imginfo);

		if(ret == MLKERR_OK)
		{
			ret = _set_layermask_image(psd, &imginfo, &box, ppimg);
			if(ret) return ret;
		}
	}

	//アルファ値セット

	if(!TileImage_setChannelImage(img, -1, ppimg, box.w, box.h))
		return MLKERR_ALLOC;

	return MLKERR_OK;
}

/* レイヤイメージ読み込み (RGBA/GRAY) */

static mlkerr _load_layer_image(AppDraw *p,mPSDLoad *psd,mPSDHeader *hd,mPopupProgress *prog)
{
	mPSDLayer info;
	LayerItem *item;
	TileImage *img;
	mImageBuf2 *chimg = NULL;
	uint8_t **ppimg;
	mSize size;
	int layerno,chnum,i;
	mlkerr ret;

	chnum = (hd->colmode == MPSD_COLMODE_GRAYSCALE)? 1: 3;

	//チャンネル用イメージ (全体)
	// :imgcanvas のサイズ内であれば、それを使う

	mPSDLoad_getLayerImageMaxSize(psd, &size);

	if(size.h <= p->imgh && size.w <= p->imgw * 4)
		ppimg = p->imgcanvas->ppbuf;
	else
	{
		chimg = mImageBuf2_new(size.w, size.h, p->imgbits, -4);
		if(!chimg) return MLKERR_ALLOC;

		ppimg = chimg->ppbuf;
	}

	//---- 各レイヤ読み込み
	// ファイルの格納順に読み込むため、下層のレイヤから順に読み込み。

	ret = MLKERR_OK;

	mPopupProgressThreadSetMax(prog, LayerList_getNum(p->layerlist) * (chnum + 1) * 10);

	for(layerno = 0; layerno < hd->layer_num; layerno++)
	{
		mPSDLoad_getLayerInfo(psd, layerno, &info);

		item = (LayerItem *)info.param;
		if(!item) continue;

		//フォルダ or 空イメージ

		if(LAYERITEM_IS_FOLDER(item)
			|| (info.box_img.w == 0 || info.box_img.h == 0))
		{
			mPopupProgressThreadAddPos(prog, (chnum + 1) * 10);
			continue;
		}

		//
	
		img = item->img;

		//A チャンネルを先にセット

		ret = _load_layer_image_alpha(psd, layerno, &info, img, ppimg, prog);
		if(ret) goto ERR;

		//RGB/GRAY チャンネル読み込み

		for(i = 0; i < chnum; i++)
		{
			ret = _load_layer_image_channel(psd, layerno, i, &info.box_img, ppimg, prog);
			if(ret) goto ERR;

			if(!TileImage_setChannelImage(img, i, ppimg, info.box_img.w, info.box_img.h))
			{
				ret = MLKERR_ALLOC;
				goto ERR;
			}
		}
	}

ERR:
	mImageBuf2_free(chimg);

	return ret;
}

/* レイヤ情報を読み込み、レイヤ作成 */

static mlkerr _load_layer(AppDraw *p,mPSDLoad *psd,mPSDHeader *hd,mPopupProgress *prog)
{
	mPSDLayer info;
	mPSDInfoSelection selinfo;
	TileImageInfo tinfo;
	mList list = MLIST_INIT;
	LayerItem *item,*item_parent;
	TileImage *img;
	int layerno,i,imgtype;
	mlkerr ret;

	//[!] list を解放すること

	item_parent = NULL; //現在の親

	imgtype = (hd->colmode == MPSD_COLMODE_GRAYSCALE)? TILEIMAGE_COLTYPE_GRAY: TILEIMAGE_COLTYPE_RGBA;

	//上のレイヤから順に作成 (PSD 上では最後のレイヤから)

	for(layerno = hd->layer_num - 1; layerno >= 0; layerno--)
	{
		//情報取得

		mListDeleteAll(&list);
		
		ret = mPSDLoad_getLayerInfoEx(psd, layerno, &info, &list);
		if(ret) break;

		//selection

		if(!mPSDLoad_exinfo_getSelection(psd, &list, &selinfo))
			selinfo.type = MPSD_INFO_SELECTION_TYPE_NORMAL;

		//

	#if 0
		mDebug("%s : (%d,%d-%dx%d) m(%d,%d-%dx%d)\n",
			info.name,
			info.box_img.x, info.box_img.y, info.box_img.w, info.box_img.h,
			info.box_mask.x, info.box_mask.y, info.box_mask.w, info.box_mask.h);
	#endif

		//フォルダ閉じる -> 親へ移動

		if(selinfo.type == MPSD_INFO_SELECTION_TYPE_END_LAST)
		{
			if(item_parent)
				item_parent = LAYERITEM(item_parent->i.parent);

			continue;
		}
		
		//レイヤ作成 (親の終端へ)

		item = LayerList_addLayer_parent(p->layerlist, item_parent);
		if(!item)
		{
			ret = MLKERR_ALLOC;
			break;
		}

		mPSDLoad_setLayerInfo_param(psd, layerno, item);

		//名前

		item->name = mStrdup(info.name);

		//不透明度

		item->opacity = (int)(info.opacity / 255.0 * 128.0 + 0.5);

		//非表示

		if(info.flags & MPSD_LAYER_FLAGS_HIDE)
			item->flags &= ~LAYERITEM_F_VISIBLE;

		//フォルダ

		if(selinfo.type == MPSD_INFO_SELECTION_TYPE_FOLDER_OPEN
			|| selinfo.type == MPSD_INFO_SELECTION_TYPE_FOLDER_CLOSED)
		{
			item_parent = item;

			if(selinfo.type == MPSD_INFO_SELECTION_TYPE_FOLDER_OPEN)
				item->flags |= LAYERITEM_F_FOLDER_OPENED;
			
			continue;
		}

		//------- イメージ

		//イメージ作成

		if(info.box_img.w == 0 || info.box_img.h == 0)
		{
			//空イメージ
			tinfo.offx = tinfo.offy = 0;
			tinfo.tilew = tinfo.tileh = 1;
		}
		else
		{
			tinfo.offx = info.box_img.x;
			tinfo.offy = info.box_img.y;
			tinfo.tilew = (info.box_img.w + 63) / 64;
			tinfo.tileh = (info.box_img.h + 63) / 64;
		}

		img = TileImage_newFromInfo(imgtype, &tinfo);
		if(!img)
		{
			ret = MLKERR_ALLOC;
			break;
		}

		LayerItem_replaceImage(item, img, imgtype);

		//合成モード

		for(i = 0; g_blendmode[i]; i++)
		{
			if(g_blendmode[i] == info.blendmode)
			{
				item->blendmode = i;
				break;
			}
		}
	}

	mListDeleteAll(&list);

	return ret;
}

/* 一枚絵イメージから読み込み */

static int _load_image(AppDraw *p,mPSDLoad *psd,mPSDHeader *hd,mPopupProgress *prog)
{
	LayerItem *item;
	TileImage *img;
	uint8_t *rowbuf,**ppbuf,*buf,*ps8,*pd8,f;
	uint16_t *ps16,*pd16,*table16 = NULL;
	int type,bits,i,ix,iy,width,height,chnum;
	mlkerr ret;

	width = hd->width;
	height = hd->height;

	//カラータイプ, bits

	bits = hd->bits;

	if(hd->bits == 1)
		type = _TYPE_MONO, bits = 8;
	else if(hd->colmode == MPSD_COLMODE_GRAYSCALE)
		type = _TYPE_GRAY;
	else if(hd->colmode == MPSD_COLMODE_RGB && hd->img_channels >= 3)
		type = _TYPE_RGB;
	else
		return MLKERR_UNSUPPORTED;

	//レイヤ作成

	item = LayerList_addLayer(p->layerlist, NULL);
	if(!item) return MLKERR_ALLOC;

	img = TileImage_new(
		(type == _TYPE_RGB)? TILEIMAGE_COLTYPE_RGBA: TILEIMAGE_COLTYPE_GRAY,
		width, height);

	if(!img) return MLKERR_ALLOC;

	LayerItem_replaceImage(item, img, img->type);
	LayerList_setItemName_curlayernum(p->layerlist, item);

	//------- イメージ

	//開始

	ret = mPSDLoad_startImage(psd);
	if(ret) return ret;

	//バッファ

	rowbuf = (uint8_t *)mMalloc(mPSDLoad_getImageChRowSize(psd));
	if(!rowbuf) return MLKERR_ALLOC;

	//16bit GRAY/RGB

	if(type != _TYPE_MONO && bits == 16)
	{
		table16 = TileImage_create16to16fix_table();
		if(!table16)
		{
			mFree(rowbuf);
			return MLKERR_ALLOC;
		}
	}

	//mono/gray

	if(type != _TYPE_RGB)
	{
		mPopupProgressThreadSetMax(prog, 50);
		mPopupProgressThreadSubStep_begin(prog, 50, height);

		ppbuf = p->imgcanvas->ppbuf;
	}

	//imgcanvas に RGBA としてセット

	switch(type)
	{
		//1bit 白黒 (8bit でセット)
		case _TYPE_MONO:
			for(iy = height; iy; iy--)
			{
				//読み込み
				
				ret = mPSDLoad_readImageRowCh(psd, rowbuf);
				if(ret) goto ERR;

				//変換

				ps8 = rowbuf;
				pd8 = *(ppbuf++);
				
				for(ix = width, f = 0x80; ix; ix--, pd8 += 4)
				{
					pd8[0] = pd8[1] = pd8[2] = (*ps8 & f)? 0: 255;
					pd8[3] = 255;
				
					f >>= 1;
					if(!f) f = 0x80, ps8++;
				}

				mPopupProgressThreadSubStep_inc(prog);
			}
			break;

		//グレイスケール
		case _TYPE_GRAY:
			for(iy = height; iy; iy--)
			{
				//読み込み
				
				ret = mPSDLoad_readImageRowCh(psd, rowbuf);
				if(ret) goto ERR;

				//変換

				buf = *(ppbuf++);

				if(bits == 8)
				{
					//8bit

					ps8 = rowbuf;
					pd8 = buf;
					
					for(ix = width; ix; ix--, pd8 += 4)
					{
						pd8[0] = pd8[1] = pd8[2] = *(ps8++);
						pd8[3] = 255;
					}
				}
				else
				{
					//16bit

					ps16 = (uint16_t *)rowbuf;
					pd16 = (uint16_t *)buf;

					for(ix = width; ix; ix--, pd16 += 4, ps16++)
					{
						pd16[0] = pd16[1] = pd16[2] = table16[*ps16];
						pd16[3] = 0x8000;
					}
				}

				mPopupProgressThreadSubStep_inc(prog);
			}
			break;

		//RGB/RGBA
		case _TYPE_RGB:
			chnum = hd->img_channels;
			if(chnum > 4) chnum = 4;

			mPopupProgressThreadSetMax(prog, chnum * 20);

			//チャンネルごとに読み込み

			for(i = 0; i < chnum; i++)
			{
				mPopupProgressThreadSubStep_begin(prog, 20, height);

				ppbuf = p->imgcanvas->ppbuf;

				for(iy = height; iy; iy--)
				{
					//読み込み
					
					ret = mPSDLoad_readImageRowCh(psd, rowbuf);
					if(ret) goto ERR;

					//変換

					buf = *(ppbuf++);

					if(bits == 8)
					{
						ps8 = rowbuf;
						pd8 = buf + i;
						
						for(ix = width; ix; ix--, pd8 += 4)
							*pd8 = *(ps8++);
					}
					else
					{
						ps16 = (uint16_t *)rowbuf;
						pd16 = (uint16_t *)buf + i;

						for(ix = width; ix; ix--, pd16 += 4, ps16++)
							*pd16 = table16[*ps16];
					}

					mPopupProgressThreadSubStep_inc(prog);
				}
			}

			//A がない場合、最大値セット

			if(chnum != 4)
			{
				mPopupProgressThreadSubStep_begin(prog, 20, height);

				ppbuf = p->imgcanvas->ppbuf;

				for(iy = height; iy; iy--)
				{
					buf = *(ppbuf++);

					if(bits == 8)
					{
						pd8 = buf + 3;

						for(ix = width; ix; ix--, pd8 += 4)
							*pd8 = 255;
					}
					else
					{
						pd16 = (uint16_t *)buf + 3;

						for(ix = width; ix; ix--, pd16 += 4)
							*pd16 = 0x8000;
					}

					mPopupProgressThreadSubStep_inc(prog);
				}
			}
			break;
	}

	//タイルに変換 (img のタイプに変換される)

	TileImage_convertFromCanvas(img, p->imgcanvas, NULL, 0);

ERR:
	mFree(rowbuf);
	mFree(table16);

	return ret;
}

/* メイン処理 */

static mlkerr _load_main(AppDraw *p,mPSDLoad *psd,mPopupProgress *prog)
{
	mPSDHeader hd;
	mlkerr ret;
	int horz,vert;

	//ヘッダ読み込み

	ret = mPSDLoad_readHeader(psd, &hd);
	if(ret) return ret;

	//CMYK

	if(hd.colmode == MPSD_COLMODE_CMYK)
		return MLKERR_UNSUPPORTED;

	//画像サイズ制限

	if(hd.width > IMAGE_SIZE_MAX || hd.height > IMAGE_SIZE_MAX)
		return MLKERR_MAX_SIZE;

	//画像リソース

	ret = mPSDLoad_readResource(psd);
	if(ret) return ret;

	if(!mPSDLoad_res_getResoDPI(psd, &horz, &vert))
		horz = -1;

	//新規イメージ

	if(!drawImage_newCanvas_openFile(p, hd.width, hd.height,
		(hd.bits == 16)? 16: 8, horz))
		return MLKERR_ALLOC;

	//レイヤ or 一枚絵イメージ

	if(hd.layer_num)
	{
		//レイヤ

		ret = mPSDLoad_startLayer(psd);
		if(ret) return ret;

		ret = _load_layer(p, psd, &hd, prog);
		if(ret) return ret;

		ret = _load_layer_image(p, psd, &hd, prog);
	}
	else
	{
		//一枚絵イメージ

		ret = _load_image(p, psd, &hd, prog);
	}

	if(ret) return ret;

	//カレントレイヤセット

	p->curlayer = LayerList_getTopItem(p->layerlist);

	return MLKERR_OK;
}

/** PSD 読み込み
 *
 * PSD 側のビット数で読み込む */

mlkerr drawFile_load_psd(AppDraw *p,const char *filename,mPopupProgress *prog)
{
	mPSDLoad *psd;
	mlkerr ret;

	//作成

	psd = mPSDLoad_new();
	if(!psd) return MLKERR_ALLOC;

	//開く

	ret = mPSDLoad_openFile(psd, filename);
	if(ret)
	{
		mPSDLoad_close(psd);
		return ret;
	}

	//処理

	ret = _load_main(p, psd, prog);

	mPSDLoad_close(psd);

	return ret;
}


//******************************
// 保存
//******************************


/* 書き込むレイヤ数を取得 */

static int _get_write_layernum(AppDraw *p)
{
	LayerItem *pi;
	int num = 0;

	for(pi = LayerList_getTopItem(p->layerlist); pi; pi = LayerItem_getNext(pi))
	{
		if(LAYERITEM_IS_FOLDER(pi))
			//フォルダを閉じる用に1つ必要
			num += 2;
		else
			num++;
	}

	return num;
}

/* フォルダ閉じるレイヤのセット */

static mlkerr _set_layer_folder_close(mPSDSave *psd,mPSDLayer *info,int layerno)
{
	mList list = MLIST_INIT;
	mlkerr ret;

	info->layerno = layerno;
	info->chnum = 4;
	info->flags = MPSD_LAYER_FLAGS_HAVE_BIT4 | MPSD_LAYER_FLAGS_NODISP;
	info->blendmode = MPSD_BLENDMODE_NORMAL;

	mPSDSave_exinfo_addSection(&list,
		MPSD_INFO_SELECTION_TYPE_END_LAST, MPSD_BLENDMODE_NORMAL);

	//
	
	ret = mPSDSave_setLayerInfo(psd, info, &list);

	mPSDSave_exinfo_freeList(&list);

	return ret;
}

/* レイヤ情報の書き込み */

static mlkerr _write_layer_info(AppDraw *p,mPSDSave *psd)
{
	LayerItem *pi;
	mPSDLayer info;
	mList list = MLIST_INIT;
	mRect rc;
	int layerno;
	mlkerr ret;

	//レイヤ情報セット

	layerno = 0;

	for(pi = LayerList_getBottomLastItem(p->layerlist); pi; pi = LayerItem_getPrev(pi))
	{
		mMemset0(&info, sizeof(mPSDLayer));

		//------ フォルダ閉じる
		// :(フォルダに子がある時) 最後のアイテムの前
		// :(フォルダに子がない時) フォルダの前
		// :[!] 子がないフォルダが、親フォルダの最後のアイテムの場合、上記の条件を2つとも満たすため、
		// : 2個続くことになる。

		if(pi->i.parent && !pi->i.next)
		{
			ret = _set_layer_folder_close(psd, &info, layerno++);
			if(ret) return ret;
		}

		if(LAYERITEM_IS_FOLDER(pi) && !pi->i.first)
		{
			ret = _set_layer_folder_close(psd, &info, layerno++);
			if(ret) return ret;
		}

		//------ イメージ or フォルダ

		info.layerno = layerno++;
		info.name = pi->name;
		info.opacity = (int)(pi->opacity / 128.0 * 255 + 0.5);
		info.chnum = 4;
		info.flags = MPSD_LAYER_FLAGS_HAVE_BIT4;

		if(!LAYERITEM_IS_VISIBLE(pi))
			info.flags |= MPSD_LAYER_FLAGS_HIDE;

		if(LAYERITEM_IS_FOLDER(pi))
		{
			//フォルダ
			
			info.flags |= MPSD_LAYER_FLAGS_NODISP;
			info.blendmode = MPSD_BLENDMODE_NORMAL;

			mPSDSave_exinfo_addSection(&list,
				(LAYERITEM_IS_FOLDER_OPENED(pi))?
					MPSD_INFO_SELECTION_TYPE_FOLDER_OPEN: MPSD_INFO_SELECTION_TYPE_FOLDER_CLOSED,
				MPSD_BLENDMODE_NORMAL);
		}
		else
		{
			//イメージ
			
			info.blendmode = g_blendmode[pi->blendmode];
			info.param = pi;

			if(TileImage_getHaveImageRect_pixel(pi->img, &rc, NULL))
				mBoxSetRect(&info.box_img, &rc);
		}

		ret = mPSDSave_setLayerInfo(psd, &info, &list);

		mPSDSave_exinfo_freeList(&list);
		
		if(ret) return ret;
	}

	//レイヤ情報書き込み

	return mPSDSave_writeLayerInfo(psd);
}

/* レイヤイメージ書き込み (8bit->8bit) */

static mlkerr _write_layer_image_8bit(mPSDSave *psd,TileImage *img,uint8_t *rowbuf,int ch,mRect *rcsrc)
{
	mRect rc;
	int ix,iy;
	mlkerr ret;
	uint8_t *pd,col[4];

	rc = *rcsrc;

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		pd = rowbuf;
		
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			TileImage_getPixel(img, ix, iy, col);

			*(pd++) = col[ch];
		}

		ret = mPSDSave_writeLayerImageRowCh(psd, rowbuf);
		if(ret) return ret;
	}

	return MLKERR_OK;
}

/* レイヤイメージ書き込み (16bit->8bit) */

static mlkerr _write_layer_image_16to8bit(mPSDSave *psd,TileImage *img,uint8_t *rowbuf,int ch,mRect *rcsrc)
{
	mRect rc;
	int ix,iy;
	mlkerr ret;
	uint8_t *pd;
	uint16_t col[4];

	rc = *rcsrc;

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		pd = rowbuf;
		
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			TileImage_getPixel(img, ix, iy, col);

			*(pd++) = (int)((double)col[ch] / 0x8000 * 255 + 0.5);
		}

		ret = mPSDSave_writeLayerImageRowCh(psd, rowbuf);
		if(ret) return ret;
	}

	return MLKERR_OK;
}

/* レイヤイメージ書き込み (16bit(fix15)->16bit) */

static mlkerr _write_layer_image_16bit(mPSDSave *psd,TileImage *img,uint8_t *rowbuf,int ch,mRect *rcsrc)
{
	mRect rc;
	int ix,iy;
	mlkerr ret;
	uint16_t *pd,col[4];

	rc = *rcsrc;

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		pd = (uint16_t *)rowbuf;
		
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			TileImage_getPixel(img, ix, iy, col);

			*(pd++) = (int)((double)col[ch] / 0x8000 * 0xffff + 0.5);
		}

		ret = mPSDSave_writeLayerImageRowCh(psd, rowbuf);
		if(ret) return ret;
	}

	return MLKERR_OK;
}

/* レイヤ出力 (RGBA) */

static mlkerr _write_layer(AppDraw *p,mPSDSave *psd,int layernum,int bits,mPopupProgress *prog)
{
	LayerItem *pi;
	uint8_t *rowbuf;
	mPSDLayer info;
	mSize size;
	mRect rc;
	int ch,i,srcbits;
	mlkerr ret;

	//レイヤ開始

	ret = mPSDSave_startLayer(psd, layernum);
	if(ret) return ret;

	//レイヤ情報書き込み

	ret = _write_layer_info(p, psd);
	if(ret) return ret;

	//チャンネルバッファ

	mPSDSave_getLayerImageMaxSize(psd, &size);

	rowbuf = (uint8_t *)mMalloc(size.w * (bits / 8));
	if(!rowbuf) return MLKERR_ALLOC;

	//レイヤイメージ

	srcbits = p->imgbits;

	for(i = 0; i < layernum; i++)
	{
		mPSDSave_getLayerInfo(psd, i, &info);

		//pi = NULL でフォルダ関連
		// [!] 空イメージでも書き込む必要あり

		pi = (LayerItem *)info.param;

		//イメージ開始
		
		ret = mPSDSave_startLayerImage(psd);
		if(ret) goto ERR;

		//イメージ範囲

		mRectSetBox(&rc, &info.box_img);

		//各チャンネル

		for(ch = 0; ch < 4; ch++)
		{
			//チャンネル開始
			
			ret = mPSDSave_startLayerImageCh(psd,
				(ch == 3)? MPSD_CHID_ALPHA: ch);

			if(ret) goto ERR;
			
			//イメージ

			if(pi)
			{
				if(srcbits == 8)
					ret = _write_layer_image_8bit(psd, pi->img, rowbuf, ch, &rc);
				else if(bits == 8)
					//16->8bit
					ret = _write_layer_image_16to8bit(psd, pi->img, rowbuf, ch, &rc);
				else
					//16bit
					ret = _write_layer_image_16bit(psd, pi->img, rowbuf, ch, &rc);

				if(ret) goto ERR;
			}

			//チャンネル終了

			ret = mPSDSave_endLayerImageCh(psd);
			if(ret) goto ERR;

			mPopupProgressThreadIncPos(prog);
		}

		//イメージ終了

		mPSDSave_endLayerImage(psd);
	}

ERR:
	mFree(rowbuf);

	if(ret == MLKERR_OK)
		ret = mPSDSave_endLayer(psd);

	return ret;
}

/* 一枚絵イメージ書き込み */

static mlkerr _write_image(AppDraw *p,mPSDSave *psd,int type,int bits,
	mPopupProgress *prog,int substep)
{
	uint8_t *rowbuf,**ppbuf,*ps8,*pd8,*buf,val,f;
	uint16_t *ps16,*pd16;
	int width,height,ch,ix,iy,i,j;
	mlkerr ret;

	width = p->imgw;
	height = p->imgh;

	switch(type)
	{
		//RGB/Layer
		case _TYPE_LAYER:
		case _TYPE_RGB:
			rowbuf = (uint8_t *)mMalloc(mPSDSave_getImageRowSize(psd));
			if(!rowbuf) return MLKERR_ALLOC;

			mPopupProgressThreadSubStep_begin(prog, substep, height * 3);

			for(ch = 0; ch < 3; ch++)
			{
				ppbuf = p->imgcanvas->ppbuf;

				for(iy = height; iy; iy--)
				{
					buf = *(ppbuf++);
					
					if(bits == 8)
					{
						//8bit
						
						pd8 = rowbuf;
						ps8 = buf + ch;

						for(ix = width; ix; ix--, ps8 += 3)
							*(pd8++) = *ps8;
					}
					else
					{
						//16bit
						
						pd16 = (uint16_t *)rowbuf;
						ps16 = (uint16_t *)buf + ch;

						for(ix = width; ix; ix--, ps16 += 3)
							*(pd16++) = *ps16;
					}

					//書き込み

					ret = mPSDSave_writeImageRowCh(psd, rowbuf);
					if(ret)
					{
						mFree(rowbuf);
						return ret;
					}

					mPopupProgressThreadSubStep_inc(prog);
				}
			}

			mFree(rowbuf);
			break;

		//グレイスケール
		case _TYPE_GRAY:
			mPopupProgressThreadSubStep_begin(prog, substep, height);

			ppbuf = p->imgcanvas->ppbuf;

			for(iy = height; iy; iy--)
			{
				buf = *(ppbuf++);
			
				if(bits == 8)
				{
					//8bit
					
					pd8 = ps8 = buf;

					for(ix = width; ix; ix--, ps8 += 3)
						*(pd8++) = _RGB_TO_GRAY(ps8[0], ps8[1], ps8[2]);
				}
				else
				{
					//16bit
					
					pd16 = ps16 = (uint16_t *)buf;

					for(ix = width; ix; ix--, ps16 += 3)
						*(pd16++) = _RGB_TO_GRAY(ps16[0], ps16[1], ps16[2]);
				}

				ret = mPSDSave_writeImageRowCh(psd, buf);
				if(ret) return ret;

				mPopupProgressThreadSubStep_inc(prog);
			}
			break;

		//1bit MONO (白=0、それ以外は1)
		default:
			mPopupProgressThreadSubStep_begin(prog, substep, height);

			ppbuf = p->imgcanvas->ppbuf;

			for(iy = height; iy; iy--)
			{
				buf = *(ppbuf++);
				ps8 = pd8 = buf;
				ix = width;

				for(i = (width + 7) >> 3; i; i--)
				{
					val = 0;
					f = 0x80;

					for(j = 8; j && ix; j--, ix--, ps8 += 3, f >>= 1)
					{
						if(ps8[0] != 255 || ps8[1] != 255 || ps8[2] != 255)
							val |= f;
					}

					*(pd8++) = val;
				}

				ret = mPSDSave_writeImageRowCh(psd, buf);
				if(ret) return ret;

				mPopupProgressThreadSubStep_inc(prog);
			}
			break;
	}

	return MLKERR_OK;
}

/* ヘッダ書き込み */

static mlkerr _write_header(AppDraw *p,mPSDSave *psd,int type,int bits)
{
	mPSDHeader hd;

	hd.width = p->imgw;
	hd.height = p->imgh;
	hd.bits = bits;
	hd.img_channels = (type == _TYPE_GRAY || type == _TYPE_MONO)? 1: 3;

	if(type == _TYPE_GRAY)
		hd.colmode = MPSD_COLMODE_GRAYSCALE;
	else if(type == _TYPE_MONO)
		hd.colmode = MPSD_COLMODE_MONO;
	else
		//RGB/Layer
		hd.colmode = MPSD_COLMODE_RGB;

	return mPSDSave_writeHeader(psd, &hd);
}

/* 書き込み処理 */

static mlkerr _write_main(AppDraw *p,mPSDSave *psd,mPopupProgress *prog)
{
	mlkerr ret;
	int type,bits,substep,layernum;

	type = SAVEOPT_PSD_GET_TYPE(APPCONF->save.psd);

	if(type == _TYPE_MONO)
		bits = 1;
	else if(p->imgbits == 16 && (APPCONF->save.psd & SAVEOPT_PSD_F_16BIT))
		bits = 16;
	else
		bits = 8;

	//ヘッダ書き込み

	ret = _write_header(p, psd, type, bits);
	if(ret) return ret;

	//リソース書き込み

	mPSDSave_res_setResoDPI(psd, p->imgdpi, p->imgdpi);

	ret = mPSDSave_writeResource(psd);
	if(ret) return ret;
	
	//レイヤ

	if(type == _TYPE_LAYER)
	{
		//レイヤあり
		
		layernum = _get_write_layernum(p);
	
		substep = 20;
		mPopupProgressThreadSetMax(prog, substep + layernum * 4);

		ret = _write_layer(p, psd, layernum, bits, prog);
		if(ret) return ret;
	}
	else
	{
		//一枚絵
		
		substep = 100;
		mPopupProgressThreadSetMax(prog, substep);

		ret = mPSDSave_writeLayerNone(psd);
		if(ret) return ret;
	}

	//一枚絵イメージ

	ret = mPSDSave_startImage(psd);
	if(ret) return ret;

	ret = _write_image(p, psd, type, bits, prog, substep);
	if(ret) return ret;

	return mPSDSave_endImage(psd);
}

/** PSD 保存 */

mlkerr drawFile_save_psd(AppDraw *p,const char *filename,mPopupProgress *prog)
{
	mPSDSave *psd;
	mlkerr ret;

	//作成

	psd = mPSDSave_new();
	if(!psd) return MLKERR_ALLOC;

	//開く

	ret = mPSDSave_openFile(psd, filename);
	if(ret)
	{
		mPSDSave_close(psd);
		return ret;
	}

	//無圧縮

	if(APPCONF->save.psd & SAVEOPT_PSD_F_UNCOMPRESS)
		mPSDSave_setCompression_none(psd);

	//

	ret = _write_main(p, psd, prog);

	mPSDSave_close(psd);

	if(ret) mDeleteFile(filename);

	return ret;
}

