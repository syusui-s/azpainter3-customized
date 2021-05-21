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
 * MainWindow: レイヤ関連
 *****************************************/

#include "mlk_gui.h"
#include "mlk_str.h"
#include "mlk_sysdlg.h"

#include "def_widget.h"
#include "def_config.h"
#include "def_draw.h"
#include "def_draw_sub.h"

#include "mainwindow.h"
#include "popup_thread.h"
#include "filedialog.h"
#include "dialogs.h"
#include "panel_func.h"

#include "layerlist.h"
#include "layeritem.h"
#include "tileimage.h"
#include "appresource.h"
#include "apd_v4_format.h"

#include "draw_main.h"
#include "draw_layer.h"

#include "trid.h"
#include "trid_mainmenu.h"



/* レイヤ新規作成 (ダイアログ)
 *
 * pi_above: NULL でカレントレイヤのタイプによって挿入位置が変わる。NULL 以外で、常にこのレイヤの上に作成。
 * tone_type: [0] 通常時 [1] トーンレイヤGRAY [2] トーンレイヤA1 (ツールバーでのドロップメニュー時) */

static void _new_layer(MainWindow *p,LayerItem *pi_above,int tone_type)
{
	LayerNewOptInfo info;

	mMemset0(&info, sizeof(info));

	//デフォルト

	mStrSetFormat(&info.strname, "layer%d", LayerList_getNum(APPDRAW->layerlist));

	info.opacity = LAYERITEM_OPACITY_MAX;
	info.tone_lines = APPCONF->tone_lines_default;
	info.tone_angle = 45;

	if(tone_type)
	{
		info.ftone = 1;
		info.type = (tone_type == 1)? LAYERTYPE_GRAY: LAYERTYPE_ALPHA1BIT;

		//A1の場合は、デフォルトで固定濃度
		if(tone_type == 2)
			info.tone_density = 10;
	}

	//ダイアログ

	if(LayerDialog_newopt(MLK_WINDOW(p), &info, TRUE) != -1)
	{
		//追加

		drawLayer_newLayer(APPDRAW, &info, pi_above);
	}

	mStrFree(&info.strname);
	mStrFree(&info.strtexpath);
}

/* ファイルからレイヤ新規作成 */

static void _newlayer_file(MainWindow *p)
{
	mStr str = MSTR_INIT;
	mlkerr ret;

	if(FileDialog_openImage(MLK_WINDOW(p),
		"Image Files\t*.apd;*.psd;*.png;*.bmp;*.jpg;*.jpeg;*.gif;*.tif;*.tiff;*.webp\t"
		"AzPainter v4 (*.apd)\t*.apd\t"
		"All Files\t*", 0,
		APPCONF->strLayerFileDir.buf, &str))
	{
		//フォルダ記録

		mStrPathGetDir(&APPCONF->strLayerFileDir, str.buf);

		//読み込み

		ret = drawLayer_newLayer_file(APPDRAW, str.buf, NULL);

		if(ret)
			MainWindow_errmes(ret, NULL);
	}

	mStrFree(&str);
}

/** レイヤ新規作成: トーンレイヤ */

void MainWindow_layer_new_tone(MainWindow *p,mlkbool type_a1)
{
	_new_layer(p, NULL, (type_a1)? 2: 1);
}

/** レイヤ設定 */

void MainWindow_layer_setOption(MainWindow *p,LayerItem *pi)
{
	LayerNewOptInfo info;
	int ret;

	//値のセット

	mMemset0(&info, sizeof(LayerNewOptInfo));

	mStrSetText(&info.strname, pi->name);
	mStrSetText(&info.strtexpath, pi->texture_path);

	if(LAYERITEM_IS_FOLDER(pi))
		info.type = -1;
	else
	{
		info.type = pi->type;

		if(LAYERITEM_IS_TEXT(pi))
			info.type |= 0x80;
	}
	
	info.blendmode = pi->blendmode;
	info.opacity = pi->opacity;
	info.col = pi->col;
	info.tone_lines = pi->tone_lines;
	info.tone_angle = pi->tone_angle;
	info.tone_density = pi->tone_density;
	info.ftone = (LAYERITEM_IS_TONE(pi) != 0);
	info.ftone_white = (LAYERITEM_IS_TONE_WHITE(pi) != 0);

	//ダイアログ

	ret = LayerDialog_newopt(MLK_WINDOW(p), &info, FALSE);

	if(ret != -1)
	{
		//セット

		if(ret & LAYER_NEWOPTDLG_F_NAME)
			mStrdup_free(&pi->name, info.strname.buf);

		if(ret & LAYER_NEWOPTDLG_F_TEXPATH)
			LayerItem_setTexture(pi, info.strtexpath.buf);

		pi->blendmode = info.blendmode;
		pi->opacity = info.opacity;

		LayerItem_setLayerColor(pi, info.col);

		if(info.ftone)
			pi->flags |= LAYERITEM_F_TONE;
		else
			pi->flags &= ~LAYERITEM_F_TONE;

		if(info.ftone_white)
			pi->flags |= LAYERITEM_F_TONE_WHITE;
		else
			pi->flags &= ~LAYERITEM_F_TONE_WHITE;

		pi->tone_lines = info.tone_lines;
		pi->tone_angle = info.tone_angle;
		pi->tone_density = info.tone_density;

		//更新

		if(ret & LAYER_NEWOPTDLG_F_UPDATE_CANVAS)
		{
			//キャンバス更新
			
			PanelLayer_update_layer_curparam(pi);
			drawUpdateRect_canvas_canvasview_inLayerHave(APPDRAW, pi);
		}
		else if(ret & LAYER_NEWOPTDLG_F_NAME)
			//名前のみ
			PanelLayer_update_layer(pi);
	}

	mStrFree(&info.strname);
	mStrFree(&info.strtexpath);
}

/** 線の色変更 */

void MainWindow_layer_setColor(MainWindow *p,LayerItem *pi)
{
	uint32_t col;

	if(LayerItem_isNeedColor(pi))
	{
		col = pi->col;
		
		if(LayerDialog_color(MLK_WINDOW(p), &col))
			drawLayer_setLayerColor(APPDRAW, pi, col);
	}
}

/** レイヤタイプ変更 */

void MainWindow_layer_setType(MainWindow *p,LayerItem *pi)
{
	int ret;

	if(LAYERITEM_IS_FOLDER(pi)) return;

	ret = LayerDialog_layerType(MLK_WINDOW(p), pi->type, LAYERITEM_IS_TEXT(pi));

	if(ret != -1)
		drawLayer_setLayerType(APPDRAW, pi, ret & 0xff, ((ret & (1<<9)) != 0));
}

/* 複数レイヤ結合 */

static void _combine_multi(MainWindow *p)
{
	int ret;

	ret = LayerDialog_combineMulti(MLK_WINDOW(p),
		LAYERITEM_IS_FOLDER(APPDRAW->curlayer),
		LayerList_haveCheckedLayer(APPDRAW->layerlist));

	if(ret != -1)
	{
		drawLayer_combineMulti(APPDRAW,
			ret & 3,		//対象
			(ret & 4) >> 2,	//新規レイヤ
			ret >> 3);		//結合後のタイプ
	}
}

/* トーン線数一括変換 */

static void _batch_tone_lines(MainWindow *p)
{
	uint32_t ret;

	ret = LayerDialog_batchToneLines(MLK_WINDOW(p));
	if(!ret) return;

	LayerList_replaceToneLines_all(APPDRAW->layerlist, ret & 0xffff, ret >> 16);

	drawUpdate_all();
}


//==============================
// ファイルに出力
//==============================


typedef struct
{
	const char *filename;
	int type;
}_thdata_savefile;


/* [スレッド] ファイルに出力 */

static int _thread_save_file(mPopupProgress *prog,void *data)
{
	_thdata_savefile *p = (_thdata_savefile *)data;

	if(p->type == 0)
		//apd
		return apd4save_saveSingleLayer(APPDRAW->curlayer, p->filename, prog);
	else
	{
		//png

		return TileImage_savePNG_rgba(APPDRAW->curlayer->img,
				p->filename, APPDRAW->imgdpi, prog);
	}
}

/* ファイルに出力 */

static void _save_file(MainWindow *p)
{
	mStr str = MSTR_INIT;
	_thdata_savefile dat;
	int type;
	mlkerr ret;

	//フォルダは除く

	if(LAYERITEM_IS_FOLDER(APPDRAW->curlayer)) return;

	//初期ファイル名 (レイヤ名から)

	mStrSetText(&str, APPDRAW->curlayer->name);
	mStrPathReplaceDisableChar(&str, '_');

	//ファイル名取得

	if(mSysDlg_saveFile(MLK_WINDOW(p),
		"AzPainter v4 (*.apd)\tapd\tPNG file (*.png)\tpng",
		0, APPCONF->strLayerFileDir.buf, 0, &str, &type))
	{
		//ディレクトリ記録

		mStrPathGetDir(&APPCONF->strLayerFileDir, str.buf);

		//保存

		dat.filename = str.buf;
		dat.type = type;

		ret = PopupThread_run(&dat, _thread_save_file);

		if(ret)
			MainWindow_errmes(ret, NULL);
	}

	mStrFree(&str);
}


//========================


/** コマンド処理 */

void MainWindow_layercmd(MainWindow *p,int id)
{
	AppDraw *draw = APPDRAW;

	switch(id)
	{
		//----- ツールバー

		//上へ移動
		case TRMENU_LAYER_TB_MOVE_UP:
			drawLayer_moveUpDown(draw, TRUE);
			break;
		//下へ移動
		case TRMENU_LAYER_TB_MOVE_DOWN:
			drawLayer_moveUpDown(draw, FALSE);
			break;
		
		//------ メインメニュー

		//新規作成
		case TRMENU_LAYER_NEW:
			_new_layer(p, NULL, 0);
			break;
		//現在のレイヤの上に新規作成
		case TRMENU_LAYER_NEW_ABOVE:
			_new_layer(p, draw->curlayer, 0);
			break;
		//新規フォルダ
		case TRMENU_LAYER_NEW_FOLDER:
			drawLayer_newLayer_direct(draw, -1);
			break;
		//ファイルから新規作成
		case TRMENU_LAYER_NEW_FROM_FILE:
			_newlayer_file(p);
			break;
		//複製
		case TRMENU_LAYER_COPY:
			drawLayer_copy(draw);
			break;
		//削除
		case TRMENU_LAYER_DELETE:
			drawLayer_delete(draw, TRUE);
			break;
		//イメージ消去
		case TRMENU_LAYER_ERASE:
			drawLayer_erase(draw);
			break;

		//下レイヤに移す
		case TRMENU_LAYER_DROP:
			drawLayer_combine(draw, TRUE);
			break;
		//下レイヤと結合
		case TRMENU_LAYER_COMBINE:
			drawLayer_combine(draw, FALSE);
			break;
		//複数レイヤ結合
		case TRMENU_LAYER_COMBINE_MULTI:
			_combine_multi(p);
			break;
		//画像の統合
		case TRMENU_LAYER_BLEND_ALL:
			drawLayer_blendAll(draw);
			break;

		//トーン化レイヤをグレイスケール表示
		case TRMENU_LAYER_TONE_TO_GRAY:
			drawLayer_toggle_tone_to_gray(draw);
			break;

		//ファイルに出力
		case TRMENU_LAYER_OUTPUT:
			_save_file(p);
			break;

		//----- 設定

		//レイヤ設定
		case TRMENU_LAYER_OPT_OPTION:
			MainWindow_layer_setOption(p, draw->curlayer);
			break;
		//線の色変更
		case TRMENU_LAYER_OPT_COLOR:
			MainWindow_layer_setColor(p, draw->curlayer);
			break;
		//レイヤタイプ変更
		case TRMENU_LAYER_OPT_TYPE:
			MainWindow_layer_setType(p, draw->curlayer);
			break;

		//---- 一括変換

		//トーン線数
		case TRMENU_LAYER_BATCH_TONE_LINES:
			_batch_tone_lines(p);
			break;

		//---- 編集

		//左右反転
		case TRMENU_LAYER_EDIT_REV_HORZ:
			drawLayer_editImage_full(draw, 0);
			break;
		//上下反転
		case TRMENU_LAYER_EDIT_REV_VERT:
			drawLayer_editImage_full(draw, 1);
			break;
		//左に回転
		case TRMENU_LAYER_EDIT_ROTATE_LEFT:
			drawLayer_editImage_full(draw, 2);
			break;
		//右に回転
		case TRMENU_LAYER_EDIT_ROTATE_RIGHT:
			drawLayer_editImage_full(draw, 3);
			break;

		//----- 表示
	
		//すべて表示
		case TRMENU_LAYER_VIEW_ALL_SHOW:
			drawLayer_visibleAll(draw, 1);
			break;
		//すべて非表示
		case TRMENU_LAYER_VIEW_ALL_HIDE:
			drawLayer_visibleAll(draw, 0);
			break;
		//カレントレイヤのみ表示
		case TRMENU_LAYER_VIEW_ONLY_CURRENT:
			drawLayer_visibleAll(draw, 2);
			break;
		//チェックレイヤの表示反転
		case TRMENU_LAYER_VIEW_REV_CHECKED:
			drawLayer_visibleAll_checked_rev(draw);
			break;
		//フォルダを除くレイヤの表示反転
		case TRMENU_LAYER_VIEW_REV_IMAGE:
			drawLayer_visibleAll_img_rev(draw);
			break;

		//---- フォルダ

		//チェックレイヤを現在のフォルダに移動
		case TRMENU_LAYER_FOLDER_CHECKED_MOVE:
			drawLayer_moveChecked_to_folder(draw, draw->curlayer);
			break;
		//現在のフォルダ以外を閉じる
		case TRMENU_LAYER_FOLDER_CLOSE_EX_CUR:
			drawLayer_folder_close_all(draw);
			break;
		//全て開く
		case TRMENU_LAYER_FOLDER_ALL_OPEN:
			drawLayer_folder_open_all(draw);
			break;

		//---- フラグ
	
		//ロック解除
		case TRMENU_LAYER_FLAG_OFF_LOCK:
			drawLayer_setFlags_all_off(draw, LAYERITEM_F_LOCK);
			break;
		//塗りつぶし判定元解除
		case TRMENU_LAYER_FLAG_OFF_FILL:
			drawLayer_setFlags_all_off(draw, LAYERITEM_F_FILLREF);
			break;
		//チェック解除
		case TRMENU_LAYER_FLAG_OFF_CHECKED:
			drawLayer_setFlags_all_off(draw, LAYERITEM_F_CHECKED);
			break;
	}
}

