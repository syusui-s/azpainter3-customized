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
 * AppDraw: レイヤ関連
 *****************************************/

#include <string.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_popup_progress.h"
#include "mlk_str.h"
#include "mlk_rectbox.h"

#include "def_config.h"
#include "def_draw.h"
#include "def_draw_sub.h"

#include "fileformat.h"
#include "popup_thread.h"
#include "panel_func.h"

#include "layerlist.h"
#include "layeritem.h"
#include "tileimage.h"
#include "imagecanvas.h"
#include "undo.h"
#include "apd_v4_format.h"

#include "draw_main.h"

#include "trid.h"



/* LayerNewOptInfo 解放 */

static void _free_newoptinfo(LayerNewOptInfo *info)
{
	mStrFree(&info->strname);
	mStrFree(&info->strtexpath);
}

/* LayerNewOptInfo にデフォルト値セット (ダイアログ無しでの新規作成時) */

static void _set_newoptinfo_default(LayerNewOptInfo *info)
{
	mMemset0(info, sizeof(LayerNewOptInfo));

	info->opacity = LAYERITEM_OPACITY_MAX;
	info->tone_lines = APPCONF->tone_lines_default;
	info->tone_angle = 45;
}

/* 新規追加時、LayerNewOptInfo からレイヤに値をセット */

static void _set_item_newoptinfo(LayerItem *pi,LayerNewOptInfo *info)
{
	mStrdup_free(&pi->name, info->strname.buf);

	pi->opacity = info->opacity;

	//フォルダでない時

	if(LAYERITEM_IS_IMAGE(pi))
	{
		pi->blendmode = info->blendmode;

		LayerItem_setLayerColor(pi, info->col);

		LayerItem_setTexture(pi, info->strtexpath.buf);

		//テキストレイヤ

		if(info->type & 0x80)
			pi->flags |= LAYERITEM_F_TEXT;

		//トーン

		if(info->ftone)
			pi->flags |= LAYERITEM_F_TONE;

		if(info->ftone_white)
			pi->flags |= LAYERITEM_F_TONE_WHITE;

		pi->tone_lines = info->tone_lines;
		pi->tone_angle = info->tone_angle;
		pi->tone_density = info->tone_density;
	}
}


//==========================
// 新規レイヤ
//==========================


/** 新規レイヤ作成
 *
 * pi_above: NULL でカレントレイヤの上、またはカレントのフォルダの子として作成。
 *  NULL 以外で、常に pi_above の上の位置に置く。
 * return: 作成されたか */

mlkbool drawLayer_newLayer(AppDraw *p,LayerNewOptInfo *info,LayerItem *pi_above)
{
	LayerItem *pi,*above;

	//レイヤ追加

	above = (pi_above)? pi_above: p->curlayer;

	if(info->type == -1)
		pi = LayerList_addLayer(p->layerlist, above);
	else
		pi = LayerList_addLayer_image(p->layerlist, above, info->type & 3, 1, 1);

	if(!pi) return FALSE;

	//フォルダ時、初期状態で開く

	if(info->type == -1)
		pi->flags |= LAYERITEM_F_FOLDER_OPENED;

	//設定

	_set_item_newoptinfo(pi, info);

	//指定レイヤの上に移動
	// :レイヤ追加時、pi_above がフォルダの場合、フォルダ内に配置される。
	// :pi_above != NULL の場合、pi_above がフォルダでも常に上の位置に置きたいので、作成後に移動。

	if(pi_above)
		LayerList_moveitem(p->layerlist, pi, pi_above, FALSE);

	//親フォルダを開く

	LayerItem_setOpened_parent(pi);

	//

	p->curlayer = pi;

	Undo_addLayerNew(pi);

	PanelLayer_update_all();

	return TRUE;
}

/** タイプを指定して新規レイヤ作成 (トーンレイヤは別)
 *
 * type: 負の値でフォルダ。0x80 が ON でテキスト */

void drawLayer_newLayer_direct(AppDraw *p,int type)
{
	LayerNewOptInfo info;

	_set_newoptinfo_default(&info);

	info.type = type;

	if(type < 0)
		mStrSetText(&info.strname, MLK_TR2(TRGROUP_LAYER_TYPE, TRID_LAYERTYPE_FOLDER));
	else
		mStrSetFormat(&info.strname, "layer%d", LayerList_getNum(p->layerlist));

	drawLayer_newLayer(p, &info, NULL);

	_free_newoptinfo(&info);
}

/** TileImage を指定して、新規レイヤ作成 (イメージ置き換え)
 *
 * 選択範囲:新規レイヤ貼り付け用 */

mlkbool drawLayer_newLayer_image(AppDraw *p,TileImage *img)
{
	LayerItem *pi;

	//作成

	pi = LayerList_addLayer(p->layerlist, p->curlayer);
	if(!pi) return FALSE;

	LayerItem_setImage(pi, img);

	LayerList_setItemName_curlayernum(p->layerlist, pi);

	//カレントレイヤ

	p->curlayer = pi;

	LayerItem_setOpened_parent(pi);

	//undo

	Undo_addLayerNew(pi);

	//更新

	drawUpdateRect_canvas_canvasview_inLayerHave(p, pi);

	PanelLayer_update_all();

	return TRUE;
}


//===============================
// ファイルから読み込み
//===============================


typedef struct
{
	const char *filename;
	TileImage *img;
	LayerItem *item;
	uint32_t format;
}_thdata_loadfile;


/* APD v4 をレイヤとして読み込み */

static mlkerr _loadfile_apd(_thdata_loadfile *p,mPopupProgress *prog)
{
	apd4load *load;
	mlkerr ret;

	ret = apd4load_open(&load, p->filename, prog);
	if(ret) return ret;

	//チャンクスキップ

	ret = apd4load_skipChunk(load);
	if(ret) goto ERR;

	//レイヤ読み込み
	// :失敗時、追加したレイヤが残ることはない。

	ret = apd4load_readLayer_append(load, &p->item);

ERR:
	apd4load_close(load);

	return ret;
}

/* [スレッド] ファイルからタイルイメージに読み込み */

static int _thread_loadfile(mPopupProgress *prog,void *data)
{
	_thdata_loadfile *p = (_thdata_loadfile *)data;

	if(p->format & FILEFORMAT_APD_v4)
	{
		//APD v4

		return _loadfile_apd(p, prog);
	}
	else
	{
		//画像ファイル

		return TileImage_loadFile(&p->img, p->filename, p->format, NULL, prog);
	}
}

/** ファイルから読み込んで新規レイヤ作成
 *
 * rcupdate: NULL で単体の読み込み。更新も行われる。
 *   NULL 以外で、複数ファイル読み込み用。読み込まれたイメージの範囲が追加される。 */

mlkerr drawLayer_newLayer_file(AppDraw *p,const char *filename,mRect *rcupdate)
{
	_thdata_loadfile dat;
	LayerItem *item;
	uint32_t format;
	mlkerr ret;
	mRect rc;
	mStr str = MSTR_INIT;

	//ファイルフォーマット
	// :APDv4 と通常イメージ

	format = FileFormat_getFromFile(filename);

	ret = ((format & FILEFORMAT_NORMAL_IMAGE)
		|| ((format & FILEFORMAT_APD) && (format & FILEFORMAT_APD_v4)));

	if(!ret)
		return MLKERR_UNSUPPORTED;

	//スレッド用データ

	mMemset0(&dat, sizeof(_thdata_loadfile));

	dat.filename = filename;
	dat.format = format;

	//読み込みスレッド

	ret = PopupThread_run(&dat, _thread_loadfile);

	if(ret == -1)
		ret = MLKERR_UNKNOWN;

	if(ret) return ret;

	//レイヤ追加

	if(format & FILEFORMAT_APD_v4)
		item = dat.item;
	else
	{
		//------ 画像ファイル時
	
		item = LayerList_addLayer(p->layerlist, p->curlayer);
		if(!item)
		{
			TileImage_free(dat.img);
			return MLKERR_ALLOC;
		}

		LayerItem_replaceImage(item, dat.img, LAYERTYPE_RGBA);

		//名前 (ファイル名から)

		mStrPathGetBasename_noext(&str, filename);

		mStrdup_free(&item->name, str.buf);

		mStrFree(&str);
	}

	//カレントレイヤ

	p->curlayer = item;

	LayerItem_setOpened_parent(item);

	//undo

	Undo_addLayerNew(item);

	//更新など

	TileImage_getHaveImageRect_pixel(item->img, &rc, NULL);

	if(rcupdate)
		//NULL 以外で、範囲を追加
		mRectUnion(rcupdate, &rc);
	else
	{
		//NULL の場合、単体読み込み
		drawUpdateRect_canvas_canvasview(p, &rc);
		PanelLayer_update_all();
	}

	return MLKERR_OK;
}


//===============================
// コマンド
//===============================


/** カレントレイヤを複製
 *
 * - フォルダは不可。 */

void drawLayer_copy(AppDraw *p)
{
	LayerItem *pi;

	//フォルダ不可

	if(LAYERITEM_IS_FOLDER(p->curlayer))
		return;

	//複製

	pi = LayerList_dupLayer(p->layerlist, p->curlayer);
	if(!pi) return;

	p->curlayer = pi;

	//undo

	Undo_addLayerDup();

	//更新

	drawUpdateRect_canvas_canvasview_inLayerHave(p, pi);
	PanelLayer_update_all();
}

/** カレントレイヤを削除 */

void drawLayer_delete(AppDraw *p,mlkbool update)
{
	mRect rc;
	LayerItem *cur;

	cur = p->curlayer;

	//親がルートで、前後にレイヤがない場合、
	// (カレントレイヤがフォルダで、子がある場合も含む)
	// 削除するとレイヤが一つもなくなるので、除外

	if(!(cur->i.parent) && !(cur->i.prev) && !(cur->i.next))
		return;

	//undo、更新範囲

	if(update)
	{
		Undo_addLayerDelete();

		LayerItem_getVisibleImageRect(cur, &rc);
	}

	//削除

	p->curlayer = LayerList_deleteLayer(p->layerlist, cur);

	//更新

	if(update)
	{
		PanelLayer_update_all();
		drawUpdateRect_canvas_canvasview(p, &rc);
	}
}

/** カレントレイヤを消去 */

void drawLayer_erase(AppDraw *p)
{
	LayerItem *pi;
	mRect rc;

	pi = p->curlayer;

	//フォルダ不可

	if(LAYERITEM_IS_FOLDER(pi))
		return;

	//

	if(LAYERITEM_IS_TEXT(pi))
	{
		//テキストのクリア

		if(!pi->list_text.top) return;

		Undo_addLayerTextClear();

		LayerItem_getVisibleImageRect(pi, &rc);

		TileImage_clear(pi->img);
		LayerItem_clearText(pi);
	}
	else
	{
		//イメージのクリア
	
		Undo_addLayerClearImage();

		LayerItem_getVisibleImageRect(pi, &rc);

		TileImage_clear(pi->img);
	}

	//更新

	PanelLayer_update_curlayer(FALSE);
	drawUpdateRect_canvas_canvasview(p, &rc);
}

/** レイヤタイプ変更 */

void drawLayer_setLayerType(AppDraw *p,LayerItem *item,int type,mlkbool lum_to_alpha)
{
	mRect rc;
	int only_text;

	drawCursor_wait();

	//テキストレイヤで、同じタイプの通常レイヤ変換か

	only_text = (LAYERITEM_IS_TEXT(item) && item->type == type);

	//undo

	Undo_addLayerSetType(item, type, only_text);

	//タイプ変換

	if(!only_text)
	{
		LayerItem_getVisibleImageRect(item, &rc);

		TileImage_convertColorType(item->img, type, lum_to_alpha);

		item->type = type;
	}

	//テキストレイヤの場合、テキストをクリアして通常レイヤに

	LayerItem_clearText(item);

	item->flags &= ~LAYERITEM_F_TEXT;

	//更新

	if(!only_text)
		drawUpdateRect_canvas_canvasview(p, &rc);

	PanelLayer_update_layer(item);

	//

	drawCursor_restore();
}

/** イメージ全体の編集 (反転/回転)
 *
 * - フォルダの場合は下位レイヤもすべて処理される。
 * - テキストレイヤは除外。 */

void drawLayer_editImage_full(AppDraw *p,int type)
{
	mRect rc;

	//対象となるレイヤがあるか

	if(!LayerItem_isHave_editImageFull(p->curlayer))
		return;

	//

	Undo_addLayerEditFull(type);

	LayerItem_editImage_full(p->curlayer, type, &rc);

	//更新

	drawUpdateRect_canvas_canvasview(p, &rc);

	if(LAYERITEM_IS_IMAGE(p->curlayer))
		PanelLayer_update_curlayer(FALSE);
	else
		//フォルダ時はプレビューを更新するため、全更新
		PanelLayer_update_all();
}


//===============================
// 下レイヤ結合/移す
//===============================


typedef struct
{
	LayerItem *item_src,*item_dst;
	int opacity_dst;
	mRect rcarea;
}_thdata_combine;


/* [スレッド] 下レイヤに結合/移す */

static int _thread_combine(mPopupProgress *prog,void *data)
{
	_thdata_combine *p = (_thdata_combine *)data;

	TileImage_combine(p->item_dst->img, p->item_src->img, &p->rcarea,
		p->item_src->opacity, p->opacity_dst, p->item_src->blendmode, prog);

	return 1;
}

/** 下レイヤに結合 or 移す
 *
 * drop: TRUE で移す、FALSE で結合 */

void drawLayer_combine(AppDraw *p,mlkbool drop)
{
	LayerItem *item_src,*item_dst;
	mRect rcsrc,rcdst;
	_thdata_combine dat;

	item_src = p->curlayer;
	item_dst = (LayerItem *)item_src->i.next;

	//除外

	if(drop)
	{
		if(!LayerItem_isEnableUnderDrop(item_src)) return;
	}
	else
	{
		if(!LayerItem_isEnableUnderCombine(item_src)) return;
	}

	//イメージ範囲

	TileImage_getHaveImageRect_pixel(item_src->img, &rcsrc, NULL);
	TileImage_getHaveImageRect_pixel(item_dst->img, &rcdst, NULL);

	//"移す" 時、元イージが空なら何もしない
	// :"結合" 時は、結合先のレイヤ不透明度を適用するため、空でも実行する。

	if(drop && mRectIsEmpty(&rcsrc)) return;

	//処理する px 範囲
	// :"結合" の場合、結合先のレイヤ不透明度を結合後イメージに適用させた後、
	// :レイヤ不透明度を 100% に変更するので、結合先全体も処理範囲に含める。

	if(!drop)
		mRectUnion(&rcsrc, &rcdst);
	
	//undo

	Undo_addLayerDropAndCombine(drop);

	//結合処理

	if(!mRectIsEmpty(&rcsrc))
	{
		dat.item_dst = item_dst;
		dat.item_src = item_src;
		dat.opacity_dst = (drop)? LAYERITEM_OPACITY_MAX: item_dst->opacity;
		dat.rcarea = rcsrc;

		if(PopupThread_run(&dat, _thread_combine) == -1)
			return;
	}

	//

	if(drop)
	{
		//---- "移す": カレントレイヤを消去

		TileImage_clear(item_src->img);

		PanelLayer_update_curlayer(FALSE);
		PanelLayer_update_layer(item_dst);
	}
	else
	{
		//---- "結合"

		item_dst->opacity = LAYERITEM_OPACITY_MAX;

		//テキスト同士の場合、テキストを結合
		LayerItem_appendText_dup(item_dst, item_src);

		//カレントレイヤを削除
		drawLayer_delete(p, FALSE);

		PanelLayer_update_all();
	}

	drawUpdateRect_canvas_canvasview(p, &rcsrc);
}


//===============================
// 複数レイヤ結合
//===============================


typedef struct
{
	TileImage *imgdst; //結合先イメージ
	LayerItem *topitem;
	mRect rcupdate;
}_thdata_combinemulti;


/* [スレッド] 複数レイヤ結合 */

static int _thread_combine_multi(mPopupProgress *prog,void *data)
{
	_thdata_combinemulti *p = (_thdata_combinemulti *)data;
	LayerItem *pi;
	int num;
	mRect rc;

	//結合レイヤ数

	for(pi = p->topitem, num = 0; pi; pi = pi->link, num++);

	mPopupProgressThreadSetMax(prog, num);

	//結合

	for(pi = p->topitem; pi; pi = pi->link)
	{
		if(TileImage_getHaveImageRect_pixel(pi->img, &rc, NULL))
		{
			TileImage_combine(p->imgdst, pi->img, &rc,
				LayerItem_getOpacity_real(pi), LAYERITEM_OPACITY_MAX,
				pi->blendmode, NULL);

			mRectUnion(&p->rcupdate, &rc);
		}

		mPopupProgressThreadIncPos(prog);
	}

	return 1;
}

/** 複数レイヤ結合 */

void drawLayer_combineMulti(AppDraw *p,int target,mlkbool newlayer,int type)
{
	LayerItem *top,*newitem,*pi;
	TileImage *img;
	_thdata_combinemulti dat;

	//対象レイヤのリンクをセット
	// :テキストレイヤ含む。
	// :対象となるレイヤがない場合、top = NULL となるが、実行はする。

	top = LayerList_setLink_combineMulti(p->layerlist, target, p->curlayer);

	//結合用イメージ作成

	img = TileImage_new(type, 1, 1);
	if(!img) return;

	//img にイメージを結合

	mRectEmpty(&dat.rcupdate);

	if(top)
	{
		dat.imgdst = img;
		dat.topitem = top;

		if(PopupThread_run(&dat, _thread_combine_multi) == -1)
		{
			TileImage_free(img);
			return;
		}
	}

	//undo (新規レイヤでない場合)

	if(!newlayer)
	{
		if(target == 0)
			Undo_addLayerCombineAll();
		else
			Undo_addLayerCombineFolder();
	}

	//各レイヤ処理

	switch(target)
	{
		//すべての表示レイヤ
		// :(通常時) レイヤをすべてクリアして、新規レイヤ
		case 0:
			if(!newlayer)
				LayerList_clear(p->layerlist);

			newitem = LayerList_addLayer(p->layerlist, NULL);

			if(newlayer)
				LayerList_moveitem(p->layerlist, newitem, LayerList_getTopItem(p->layerlist), FALSE);

			LayerList_setItemName_curlayernum(p->layerlist, newitem);
			break;

		//フォルダ内
		// :(通常時) 現在のフォルダを削除して、新規レイヤ。レイヤ名はフォルダ名と同じにする。
		case 1:
			newitem = LayerList_addLayer(p->layerlist, NULL);

			LayerList_moveitem(p->layerlist, newitem, p->curlayer, FALSE);

			mStrdup_free(&newitem->name, p->curlayer->name);

			if(!newlayer)
				drawLayer_delete(p, FALSE);
			break;

		//チェック (新規レイヤ)
		// :チェックの一番上のレイヤの上へ
		default:
			for(pi = top; pi && pi->link; pi = pi->link);

			newitem = LayerList_addLayer(p->layerlist, pi);

			LayerList_setItemName_curlayernum(p->layerlist, newitem);
			break;
	}

	LayerItem_replaceImage(newitem, img, type);

	p->curlayer = newitem;

	//undo (新規レイヤ時)

	if(newlayer)
		Undo_addLayerNew(newitem);

	//更新

	drawUpdateRect_canvas_canvasview(p, &dat.rcupdate);
	PanelLayer_update_all();
}


//===============================
// 画像の統合
//===============================


/* [スレッド] 画像の統合 */

static int _thread_blend_all(mPopupProgress *prog,void *data)
{
	TileImage *img = (TileImage *)data;

	mPopupProgressThreadSetMax(prog, 20 + 20);

	//ImageCanvas に合成

	drawImage_blendImageReal_curbits(APPDRAW, prog, 20);

	//アルファ値を最大に

	ImageCanvas_setAlphaMax(APPDRAW->imgcanvas);

	//ImageCanvas から TileImage にセット (RGBA)

	TileImage_convertFromCanvas(img, APPDRAW->imgcanvas, prog, 20);
	
	return 0;
}

/** 画像の統合 (すべて合成) */

void drawLayer_blendAll(AppDraw *p)
{
	TileImage *img;
	LayerItem *item;
	int ret;
	
	//統合後のイメージを作成

	img = TileImage_new(TILEIMAGE_COLTYPE_RGBA, p->imgw, p->imgh);
	if(!img) return;

	//実行

	APPDRAW->in_thread_imgcanvas = TRUE;

	ret = PopupThread_run(img, _thread_blend_all);

	APPDRAW->in_thread_imgcanvas = FALSE;

	if(ret)
	{
		TileImage_free(img);
		return;
	}

	//undo

	Undo_addLayerCombineAll();

	//レイヤクリア

	LayerList_clear(p->layerlist);

	//レイヤ追加

	item = LayerList_addLayer(p->layerlist, NULL);

	LayerList_setItemName_curlayernum(p->layerlist, item);

	LayerItem_replaceImage(item, img, LAYERTYPE_RGBA);

	//

	p->curlayer = item;

	//更新

	drawUpdate_all_layer();
}


//===============================
// 状態など変更
//===============================


/** カレントレイヤ変更
 *
 * レイヤ一覧上で、変更前のレイヤと変更後のレイヤが更新される。
 * 変更前のレイヤを更新させたくない場合は、AppDraw::curlayer を NULL にしておく。
 *
 * return: 変更されたか */

mlkbool drawLayer_setCurrent(AppDraw *p,LayerItem *item)
{
	if(item == p->curlayer)
		return FALSE;
	else
	{
		LayerItem *last = p->curlayer;

		p->curlayer = item;

		PanelLayer_update_layer(last);
		PanelLayer_update_curlayer(TRUE);

		return TRUE;
	}
}

/** カレントレイヤ変更 (一覧上で item が見える状態にする)
 *
 * - item が現在の選択レイヤの場合でも、スクロールは行う。
 * - item がフォルダで閉じて見えない場合は、親を開く。
 * - 一覧上で見えるようにスクロールする。 */

void drawLayer_setCurrent_visibleOnList(AppDraw *p,LayerItem *item)
{
	LayerItem *last;
	mlkbool update_all = FALSE;

	//以前のカレント (変わらないなら NULL)

	last = p->curlayer;
	if(last == item) last = NULL;

	//カレント変更

	p->curlayer = item;

	//親フォルダを開く

	if(!LayerItem_isVisible_onlist(item))
	{
		LayerItem_setOpened_parent(item);
		update_all = TRUE;
	}

	//更新

	PanelLayer_update_changecurrent_visible(last, update_all);
}

/** テクスチャセット */

void drawLayer_setTexture(AppDraw *p,LayerItem *item,const char *path)
{
	if(LayerItem_setTexture(item, path))
		drawUpdateRect_canvas_canvasview_inLayerHave(p, item);
}

/** 線の色変更 */

void drawLayer_setLayerColor(AppDraw *p,LayerItem *item,uint32_t col)
{
	if(LayerItem_isNeedColor(item))
	{
		LayerItem_setLayerColor(item, col);

		drawUpdateRect_canvas_canvasview_inLayerHave(p, item);
		PanelLayer_update_layer(item);
	}
}

/** フォルダの展開状態を反転 */

void drawLayer_toggle_folderOpened(AppDraw *p,LayerItem *item)
{
	item->flags ^= LAYERITEM_F_FOLDER_OPENED;

	PanelLayer_update_all(); //スクロール情報変更のため
}

/** レイヤの表示/非表示を反転 (レイヤ一覧は更新しない) */

void drawLayer_toggle_visible(AppDraw *p,LayerItem *item)
{
	mRect rc;

	if(LAYERITEM_IS_VISIBLE(item))
	{
		//表示 => 非表示
		// :現在イメージがある範囲を取得し、その範囲を更新。

		LayerItem_getVisibleImageRect(item, &rc);

		item->flags ^= LAYERITEM_F_VISIBLE;

		drawUpdateRect_canvas_canvasview(p, &rc);
	}
	else
	{
		//非表示 => 表示

		item->flags ^= LAYERITEM_F_VISIBLE;

		drawUpdateRect_canvas_canvasview_inLayerHave(p, item);
	}
}

/** ロック状態を反転 (レイヤ一覧の更新はしない) */

void drawLayer_toggle_lock(AppDraw *p,LayerItem *item)
{
	item->flags ^= LAYERITEM_F_LOCK;
}

/** マスクレイヤのセット (レイヤ一覧更新あり)
 *
 * type: [0] 通常クリック時 [0以外] 下レイヤマスクのON/OFF */

void drawLayer_setLayerMask(AppDraw *p,LayerItem *item,int type)
{
	int funder,fmask;
	LayerItem *update_item = NULL;

	fmask = (item == p->masklayer);
	funder = item->flags & LAYERITEM_F_MASK_UNDER;

	switch(type)
	{
		//通常クリック
		// :マスクレイヤ/下レイヤマスクを OFF or マスクレイヤ ON
		case 0:
			if(fmask)
				//マスクレイヤOFF
				p->masklayer = NULL;
			else if(funder)
				//下レイヤマスクOFF
				item->flags ^= LAYERITEM_F_MASK_UNDER;
			else
			{
				//マスクレイヤON

				if(p->masklayer) update_item = p->masklayer;
				
				p->masklayer = item;
			}
			break;

		//下レイヤマスクON/OFF
		default:
			if(fmask) p->masklayer = NULL;

			item->flags ^= LAYERITEM_F_MASK_UNDER;
			break;
	}
	
	PanelLayer_update_layer(update_item);
	PanelLayer_update_layer(item);
}


//=========================
// フラグ関連
//=========================


/** すべてのレイヤの表示を変更
 *
 * type: [0] すべて非表示 [1] すべて表示 [2] カレントのみ表示 */

void drawLayer_visibleAll(AppDraw *p,int type)
{
	LayerList_setVisible_all(p->layerlist, (type == 1));

	if(type == 2)
		LayerItem_setVisible(p->curlayer);

	drawUpdate_all_layer();
}

/** チェックレイヤの表示反転 */

void drawLayer_visibleAll_checked_rev(AppDraw *p)
{
	mRect rc;

	LayerList_setVisible_all_checked_rev(p->layerlist, &rc);

	drawUpdateRect_canvas_canvasview(p, &rc);
	PanelLayer_update_all();
}

/** フォルダを除くレイヤの表示反転 */

void drawLayer_visibleAll_img_rev(AppDraw *p)
{
	mRect rc;

	LayerList_setVisible_all_img_rev(p->layerlist, &rc);

	drawUpdateRect_canvas_canvasview(p, &rc);
	PanelLayer_update_all();
}

/** すべてのフォルダを開く */

void drawLayer_folder_open_all(AppDraw *p)
{
	LayerList_folder_open_all(p->layerlist);

	PanelLayer_update_all();
}

/** すべてのフォルダを閉じる */

void drawLayer_folder_close_all(AppDraw *p)
{
	LayerList_folder_close_all(p->layerlist, p->curlayer);

	PanelLayer_update_all();
}

/** すべてのレイヤの指定フラグを OFF */

void drawLayer_setFlags_all_off(AppDraw *p,uint32_t flags)
{
	LayerList_setFlags_all_off(p->layerlist, flags);

	PanelLayer_update_all();
}


//==============================
// ほか
//==============================


/** トーンレイヤをグレイスケール表示の切り替え */

void drawLayer_toggle_tone_to_gray(AppDraw *p)
{
	p->ftonelayer_to_gray ^= 1;

	TileImage_global_setToneToGray(p->ftonelayer_to_gray);

	//

	PanelLayer_update_curparam();

	drawUpdate_all();
}


//==============================
// 移動
//==============================


/** カレントレイヤを上下に移動 (同じフォルダ内で) */

void drawLayer_moveUpDown(AppDraw *p,mlkbool up)
{
	int parent,no;

	LayerList_getItemIndex_forParent(p->layerlist, p->curlayer, &parent, &no);

	if(LayerList_moveitem_updown(p->layerlist, p->curlayer, up))
	{
		//undo

		Undo_addLayerMoveList(p->curlayer, parent, no);

		//更新

		PanelLayer_update_all();
		drawUpdateRect_canvas_canvasview_inLayerHave(p, p->curlayer);
	}
}

/** D&D によるレイヤ移動
 *
 * type: [0] 挿入 [1] フォルダ */

void drawLayer_moveDND(AppDraw *p,LayerItem *dst,int type)
{
	LayerItem *cur;
	mTreeItem *parent,*prev;
	int pno,no;

	cur = p->curlayer;
	parent = cur->i.parent;
	prev = cur->i.prev;

	//移動元の位置 (Undo 用)

	LayerList_getItemIndex_forParent(p->layerlist, cur, &pno, &no);

	//移動

	LayerList_moveitem(p->layerlist, cur, dst, (type == 1));

	//位置が変わっていない

	if(cur->i.parent == parent && cur->i.prev == prev)
		return;

	//undo

	Undo_addLayerMoveList(cur, pno, no);

	//更新

	PanelLayer_update_all();
	drawUpdateRect_canvas_canvasview_inLayerHave(p, cur);
}

/** チェックレイヤを指定フォルダに移動 */

void drawLayer_moveChecked_to_folder(AppDraw *p,LayerItem *dst)
{
	mRect rc;
	int16_t *buf;
	int num;

	if(!LAYERITEM_IS_FOLDER(dst)) return;

	buf = LayerList_moveitem_checked_to_folder(p->layerlist, dst, &num, &rc);

	if(buf)
	{
		//undo

		Undo_addLayerMoveList_multi(num, buf);
		mFree(buf);

		//更新
	
		PanelLayer_update_all();
		drawUpdateRect_canvas_canvasview(p, &rc);
	}
}


//==============================
// アンドゥ実行用
//==============================


/** レイヤ削除 (UNDO 実行用) */

void drawLayer_deleteForUndo(AppDraw *p,LayerItem *item)
{
	LayerItem *cur;

	if(!item) return;

	//削除後のカレントレイヤ
	// :item が "現在のカレントレイヤ" または "カレントレイヤが item の子" の場合、
	// :カレントレイヤが削除されることになるので、item の前後をカレントにする。

	cur = p->curlayer;

	if(item == cur || LayerItem_isInParent(cur, item))
	{
		cur = LayerItem_getNextPass(item);
		if(!cur) cur = LayerItem_getPrevOpened(item);
	}

	//レイヤ削除

	LayerList_deleteLayer(APPDRAW->layerlist, item);

	//カレントレイヤ

	p->curlayer = cur;
}

/** アンドゥ処理でレイヤの順番移動をした後に実行 */

void drawLayer_afterMoveList_forUndo(AppDraw *p)
{
	//現在のカレントが、移動後に、閉じられているフォルダの子になった場合、
	//一覧上に見えないので、カレントの親をすべて開く。

	if(!LayerItem_isVisible_onlist(p->curlayer))
		LayerItem_setOpened_parent(p->curlayer);
}


//==============================
// ほか
//==============================


/** レイヤの選択を一つ上下に移動 */

void drawLayer_selectUpDown(AppDraw *p,mlkbool up)
{
	LayerItem *cur = p->curlayer;

	if(up)
		cur = LayerItem_getPrevOpened(cur);
	else
		cur = LayerItem_getNextOpened(cur);

	if(cur)
		drawLayer_setCurrent_visibleOnList(p, cur);
}

/** 指定位置に点がある最上位のレイヤを選択 */

void drawLayer_selectGrabLayer(AppDraw *p,int x,int y)
{
	LayerItem *item;

	item = LayerList_getItem_topPixelLayer(p->layerlist, x, y);

	if(item)
		drawLayer_setCurrent_visibleOnList(p, item);
}

