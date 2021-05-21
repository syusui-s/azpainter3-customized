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

/************************************
 * APD v4 読み込み/保存
 ************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_popup_progress.h"
#include "mlk_rectbox.h"

#include "def_draw.h"
#include "def_config.h"

#include "layerlist.h"
#include "layeritem.h"
#include "imagecanvas.h"

#include "draw_main.h"

#include "apd_v4_format.h"


//=========================
// 保存
//=========================


/** APD v4 保存 */

mlkerr drawFile_save_apd_v4(AppDraw *p,const char *filename,mPopupProgress *prog)
{
	apd4save *save;
	LayerItem *pi;
	mlkerr ret;
	int layernum,fpict;
	mBox box;

	ret = apd4save_open(&save, filename, prog);
	if(ret) return ret;

	fpict = !(APPCONF->foption & CONFIG_OPTF_SAVE_APD_NOPICT);

	layernum = LayerList_getNum(p->layerlist);

	mPopupProgressThreadSetMax(prog, layernum * 6 + (fpict? 6: 0));

	//ヘッダ

	ret = apd4save_writeHeadInfo(save, layernum);
	if(ret) goto ERR;

	//---- チャンクデータ

	//一枚絵

	if(fpict)
	{
		ret = apd4save_writeChunk_picture(save, p->imgcanvas->ppbuf,
			p->imgw, p->imgh, 6);

		if(ret) goto ERR;
	}

	//サムネイル
	// :[!] imgcanvas のイメージが上書きされる

	box.x = box.y = 0;
	box.w = p->imgw;
	box.h = p->imgh;

	mBoxResize_keepaspect(&box, 100, 100, TRUE); 

	if(ImageCanvas_setThumbnailImage_8bit(p->imgcanvas, box.w, box.h))
	{
		ret = apd4save_writeChunk_thumbnail(save, p->imgcanvas->ppbuf, box.w, box.h);
		if(ret) goto ERR;
	}

	//終了

	ret = apd4save_writeChunk_end(save);
	if(ret) goto ERR;

	//---- レイヤデータ

	for(pi = LayerList_getTopItem(p->layerlist); pi; pi = LayerItem_getNext(pi))
	{
		ret = apd4save_writeLayer(save, pi, FALSE, 6);
		if(ret) goto ERR;
	}

	ret = apd4save_writeLayer_end(save);

ERR:
	apd4save_close(save);

	return ret;
}


//=========================
// 読み込み
//=========================


/** v4 読み込み */

mlkerr drawFile_load_apd_v4(AppDraw *p,const char *filename,mPopupProgress *prog)
{
	apd4load *load;
	apd4info info;
	mlkerr ret;

	//開く

	ret = apd4load_open(&load, filename, prog);
	if(ret) return ret;

	//基本情報

	apd4load_getInfo(load, &info);

	if(!drawImage_newCanvas_openFile_bkcol(p, info.width, info.height,
		info.bits, info.dpi, info.bkgndcol))
	{
		ret = MLKERR_ALLOC;
		goto ERR;
	}

	//チャンク

	ret = apd4load_skipChunk(load);
	if(ret) goto ERR;

	//レイヤ

	ret = apd4load_readLayers(load);

	//
ERR:
	apd4load_close(load);
	
	return ret;
}

