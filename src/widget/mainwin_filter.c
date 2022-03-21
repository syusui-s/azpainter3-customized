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
 * MainWindow: フィルタ関連
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_popup_progress.h"
#include "mlk_rectbox.h"
#include "mlk_sysdlg.h"

#include "def_widget.h"
#include "def_draw.h"

#include "mainwindow.h"
#include "popup_thread.h"

#include "layeritem.h"
#include "layerlist.h"
#include "tileimage.h"
#include "tileimage_drawinfo.h"
#include "def_filterdraw.h"

#include "draw_main.h"
#include "draw_calc.h"
#include "draw_op_sub.h"

#include "trid.h"
#include "trid_mainmenu.h"

#include "filter_drawfunc.h"

#include "def_filterdlg_dat.h"


//-----------------

typedef struct
{
	const uint8_t *dlgdat;	//ダイアログの項目データ
	FilterDrawFunc func;	//描画関数
	uint8_t flags;
}_cmddat;

enum
{
	_FDF_PROC_COLOR = 1<<0,		//RGB を対象としたカラー処理 (透明部分は処理しないため、描画範囲から空タイル部分を除外)
	_FDF_COPYSRC    = 1<<1,		//実際の処理時、imgsrc を imgdst から複製する
	_FDF_CLIPPING   = 1<<2,		//イメージ範囲外は処理しない & ソース画像クリッピング
	_FDF_NODLG_VAL  = 1<<3,		//ダイアログなし。dlgdat の値を数値にして、FilterDrawInfo::ntmp[0] へセット
	_FDF_NEED_CHECK = 1<<4		//チェックレイヤがあるか確認する
};

#define _DLGDAT_INT(n)  (const uint8_t *)(n)

//-----------------

static const _cmddat g_filter_cmd_dat[] = {
	//カラー
	{g_col_brightconst, FilterDraw_color_brightcont, _FDF_PROC_COLOR},
	{g_col_gamma, FilterDraw_color_gamma, _FDF_PROC_COLOR},
	{g_col_level, FilterDraw_color_level, _FDF_PROC_COLOR},
	{g_col_rgb, FilterDraw_color_rgb, _FDF_PROC_COLOR},
	{g_col_hsv, FilterDraw_color_hsv, _FDF_PROC_COLOR},
	{g_col_hsl, FilterDraw_color_hsl, _FDF_PROC_COLOR},
	{NULL, FilterDraw_color_nega, _FDF_PROC_COLOR},
	{NULL, FilterDraw_color_grayscale, _FDF_PROC_COLOR},
	{NULL, FilterDraw_color_sepia, _FDF_PROC_COLOR},
	{NULL, FilterDraw_color_gradmap, _FDF_PROC_COLOR},
	{g_col_threshold, FilterDraw_color_threshold, _FDF_PROC_COLOR},
	{g_col_threshold_dither, FilterDraw_color_threshold_dither, _FDF_PROC_COLOR},
	{g_col_posterize, FilterDraw_color_posterize, _FDF_PROC_COLOR},

	//色置換
	{g_col_replace_drawcol, FilterDraw_color_replace_drawcol, _FDF_PROC_COLOR},
	{_DLGDAT_INT(0), FilterDraw_color_replace, _FDF_PROC_COLOR | _FDF_NODLG_VAL}, //描画色を透明に
	{_DLGDAT_INT(1), FilterDraw_color_replace, _FDF_PROC_COLOR | _FDF_NODLG_VAL}, //描画色以外を透明に
	{_DLGDAT_INT(2), FilterDraw_color_replace, _FDF_PROC_COLOR | _FDF_NODLG_VAL}, //描画色を背景色に
	{_DLGDAT_INT(3), FilterDraw_color_replace, _FDF_NODLG_VAL}, //透明色を描画色に

	//アルファ操作(チェックレイヤ)
	{_DLGDAT_INT(0), FilterDraw_alpha_checked, _FDF_NODLG_VAL | _FDF_NEED_CHECK}, //すべて透明な点を透明に
	{_DLGDAT_INT(1), FilterDraw_alpha_checked, _FDF_NODLG_VAL | _FDF_NEED_CHECK}, //いずれかが不透明な点を透明に
	{_DLGDAT_INT(2), FilterDraw_alpha_checked, _FDF_NODLG_VAL | _FDF_NEED_CHECK}, //すべての値を合成してコピー
	{_DLGDAT_INT(3), FilterDraw_alpha_checked, _FDF_NODLG_VAL | _FDF_NEED_CHECK}, //すべての値を足す
	{_DLGDAT_INT(4), FilterDraw_alpha_checked, _FDF_NODLG_VAL | _FDF_NEED_CHECK}, //すべての値を引く
	{_DLGDAT_INT(5), FilterDraw_alpha_checked, _FDF_NODLG_VAL | _FDF_NEED_CHECK}, //すべての値を乗算
	{_DLGDAT_INT(6), FilterDraw_alpha_checked, _FDF_NODLG_VAL | _FDF_NEED_CHECK}, //指定レイヤで明るい色ほど透明に
	{_DLGDAT_INT(7), FilterDraw_alpha_checked, _FDF_NODLG_VAL | _FDF_NEED_CHECK}, //指定レイヤで暗い色ほど透明に

	//アルファ操作(カレント)
	{_DLGDAT_INT(0), FilterDraw_alpha_current, _FDF_NODLG_VAL}, //明るい色ほど透明に
	{_DLGDAT_INT(1), FilterDraw_alpha_current, _FDF_NODLG_VAL}, //暗い色ほど透明に
	{_DLGDAT_INT(2), FilterDraw_alpha_current, _FDF_NODLG_VAL},	//透明以外を最大値に
	{_DLGDAT_INT(3), FilterDraw_alpha_current, _FDF_NODLG_VAL}, //テクスチャ適用
	{_DLGDAT_INT(4), FilterDraw_alpha_current, _FDF_NODLG_VAL}, //アルファ値からグレイスケール作成

	//ぼかし
	{g_blur_blur, FilterDraw_blur, _FDF_COPYSRC},
	{g_blur_gauss, FilterDraw_gaussblur, _FDF_COPYSRC},
	{g_blur_motion, FilterDraw_motionblur, _FDF_COPYSRC},
	{g_blur_radial, FilterDraw_radialblur, _FDF_COPYSRC},
	{g_blur_lens, FilterDraw_lensblur, _FDF_COPYSRC},

	//描画
	{g_draw_cloud, FilterDraw_draw_cloud, 0},
	{g_draw_amitone, FilterDraw_draw_amitone, _FDF_CLIPPING},
	{g_draw_rndpoint, FilterDraw_draw_randpoint, 0},
	{g_draw_edgepoint, FilterDraw_draw_edgepoint, _FDF_COPYSRC},
	{g_draw_frame, FilterDraw_draw_frame, _FDF_CLIPPING},
	{g_draw_horzvert_line, FilterDraw_draw_horzvertLine, _FDF_CLIPPING},
	{g_draw_plaid, FilterDraw_draw_plaid, _FDF_CLIPPING},

	//漫画用
	{g_comic_amitone_create, FilterDraw_comic_amitone_create, _FDF_CLIPPING},
	{g_comic_to_amitone, FilterDraw_comic_to_amitone, _FDF_PROC_COLOR},
	{g_comic_sand_tone, FilterDraw_comic_sand_tone, 0},
	{g_comic_concline, FilterDraw_comic_concline_flash, 0},
	{g_comic_flash, FilterDraw_comic_concline_flash, 0},
	{g_comic_popupflash, FilterDraw_comic_popupflash, 0},
	{g_comic_uniflash, FilterDraw_comic_uniflash, 0},
	{g_comic_uniflash_wave, FilterDraw_comic_uniflash_wave, 0},

	//ピクセレート
	{g_pix_mozaic, FilterDraw_mozaic, 0},
	{g_pix_crystal, FilterDraw_crystal, 0},
	{g_pix_halftone, FilterDraw_halftone, 0},

	//輪郭
	{g_edge_sharp, FilterDraw_sharp, _FDF_PROC_COLOR|_FDF_COPYSRC|_FDF_CLIPPING},
	{g_edge_unsharpmask, FilterDraw_unsharpmask, _FDF_PROC_COLOR|_FDF_COPYSRC},
	{NULL, FilterDraw_edge_sobel, _FDF_PROC_COLOR|_FDF_COPYSRC|_FDF_CLIPPING},
	{NULL, FilterDraw_edge_laplacian, _FDF_PROC_COLOR|_FDF_COPYSRC|_FDF_CLIPPING},
	{g_edge_highpass, FilterDraw_highpass, _FDF_PROC_COLOR|_FDF_COPYSRC},

	//効果
	{g_eff_glow, FilterDraw_effect_glow, _FDF_COPYSRC},
	{g_eff_rgbshift, FilterDraw_effect_rgbshift, _FDF_COPYSRC},
	{g_eff_oilpaint, FilterDraw_effect_oilpaint, _FDF_COPYSRC},
	{g_eff_emboss, FilterDraw_effect_emboss, _FDF_PROC_COLOR|_FDF_COPYSRC},
	{g_eff_noise, FilterDraw_effect_noise, _FDF_PROC_COLOR},
	{g_eff_diffusion, FilterDraw_effect_diffusion, _FDF_COPYSRC},
	{g_eff_scratch, FilterDraw_effect_scratch, _FDF_COPYSRC},
	{g_eff_median, FilterDraw_effect_median, _FDF_COPYSRC},
	{g_eff_blurring, FilterDraw_effect_blurring, _FDF_COPYSRC},

	//変形
	{g_trans_wave, FilterDraw_trans_wave, _FDF_COPYSRC},
	{g_trans_ripple, FilterDraw_trans_ripple, _FDF_COPYSRC},
	{g_trans_polar, FilterDraw_trans_polar, _FDF_COPYSRC|_FDF_CLIPPING},
	{g_trans_radial_shift, FilterDraw_trans_radial_shift, _FDF_COPYSRC},
	{g_trans_swirl, FilterDraw_trans_swirl, _FDF_COPYSRC|_FDF_CLIPPING},

	//ほか
	{NULL, FilterDraw_lum_to_alpha, 0},
	{NULL, FilterDraw_dot_thinning, _FDF_CLIPPING},
	{g_other_antialiasing, FilterDraw_antialiasing, _FDF_COPYSRC|_FDF_CLIPPING},
	{g_other_hemming, FilterDraw_hemming, _FDF_COPYSRC},
	{g_other_3dframe, FilterDraw_3dframe, _FDF_CLIPPING},
	{g_other_shift, FilterDraw_shift, _FDF_COPYSRC|_FDF_CLIPPING}
};

//-----------------

mlkbool FilterDlg_run(mWindow *parent,int menuid,const uint8_t *dlgdat,FilterDrawInfo *info);

//-----------------



/* 実行前にメッセージ表示
 *
 * return: TRUE で中止 */

static mlkbool _before_message(int menuid,uint8_t flags)
{
	int id,coltype;

	//描画関連

	if(drawOpSub_canDrawLayer_mes(APPDRAW, CANDRAWLAYER_F_NO_HIDE))
		return TRUE;

	//ほか

	coltype = APPDRAW->curlayer->type;

	if((flags & _FDF_PROC_COLOR)
		&& (coltype == LAYERTYPE_ALPHA || coltype == LAYERTYPE_ALPHA1BIT))
	{
		//[カラー処理] アルファ値のみの場合は対象外
		
		id = TRID_MESSAGE_FILTER_NO_COLOR;
	}
	else if((flags & _FDF_NEED_CHECK)
		&& !LayerList_haveCheckedLayer(APPDRAW->layerlist))
	{
		//チェックレイヤが存在しない

		id = TRID_MESSAGE_FILTER_NEED_CHECK;
	}
	else if(menuid == TRMENU_FILTER_ALPHA2_TEXTURE && !APPDRAW->imgmat_opttex)
	{
		//"テクスチャ適用" で、テクスチャイメージがない

		id = TRID_MESSAGE_FILTER_NEED_OPT_TEXTURE;
	}
	else
		return FALSE;

	//メッセージ

	mMessageBoxOK(MLK_WINDOW(APPWIDGET->mainwin), MLK_TR2(TRGROUP_MESSAGE, id));

	return TRUE;
}

/* 実際の実行時、処理するイメージ範囲をセット
 *
 * return: FALSE で処理なし */

static mlkbool _set_image_area(FilterDrawInfo *info,uint8_t flags)
{
	mRect rc;

	if(drawSel_isHave())
		//選択範囲
		rc = APPDRAW->sel.rcsel;
	else
	{
		//イメージ全体
		
		if(flags & _FDF_PROC_COLOR)
		{
			//カラー処理の場合は、空タイルを除いた範囲 (空イメージで処理なし)
			
			if(!TileImage_getHaveImageRect_pixel(APPDRAW->curlayer->img, &rc, NULL))
				return FALSE;
		}
		else
			//描画可能範囲
			TileImage_getCanDrawRect_pixel(APPDRAW->curlayer->img, &rc);
	}

	//キャンバス範囲内にクリッピング

	if(info->clipping)
	{
		if(!drawCalc_clipImageRect(APPDRAW, &rc))
			return FALSE;
	}

	//セット

	info->rc = rc;

	mBoxSetRect(&info->box, &rc);

	return TRUE;
}

/* フィルタ処理スレッド */

static int _thread_filter(mPopupProgress *prog,void *data)
{
	FilterDrawInfo *info = (FilterDrawInfo *)data;

	info->prog = prog;

	//進捗の最大は、デフォルトで処理範囲の高さ

	mPopupProgressThreadSubStep_begin_onestep(prog, 50, info->box.h);

	return (info->func_draw)(info);
}

/* フィルタの実行 */

static void _run_filter(int menuid,const uint8_t *dlgdat,FilterDrawFunc func,uint8_t flags)
{
	FilterDrawInfo info;
	TileImage *imgsrc = NULL;
	int ret;

	if(_before_message(menuid, flags)) return;

	//FilterDrawInfo

	mMemset0(&info, sizeof(FilterDrawInfo));

	info.func_draw = func;
	info.clipping = ((flags & _FDF_CLIPPING) != 0);
	info.rgb_drawcol = APPDRAW->col.drawcol;
	info.rgb_bkgnd = APPDRAW->col.bkgndcol;
	info.bits = APPDRAW->imgbits;
	info.rand = (mRandSFMT *)TileImage_global_getRand();

	//------- ダイアログ実行

	if(dlgdat && !(flags & _FDF_NODLG_VAL))
	{
		info.in_dialog = TRUE;
		
		APPDRAW->is_canvview_update = FALSE;
		APPDRAW->in_filter_dialog = TRUE;
		APPDRAW->tileimg_filterprev = NULL;

		//ダイアログ内プレビュー用、初期化

		g_tileimage_dinfo.func_setpixel = TileImage_setPixel_new_notp;

		//ダイアログ実行

		ret = FilterDlg_run(MLK_WINDOW(APPWIDGET->mainwin), menuid, dlgdat, &info);

		info.in_dialog = FALSE;
		
		APPDRAW->in_filter_dialog = FALSE;

		if(!ret) return;
	}

	//------ フィルタ処理実行

	//処理範囲セット

	if(!_set_image_area(&info, flags))
		return;

	//描画情報

	info.imgsrc = info.imgdst = APPDRAW->curlayer->img;
	info.imgsel = APPDRAW->tileimg_sel;

	if(flags & _FDF_NODLG_VAL)
		info.ntmp[0] = (intptr_t)dlgdat;

	//描画先をコピーしてソースとする

	if(flags & _FDF_COPYSRC)
	{
		imgsrc = TileImage_newClone(info.imgdst);
		if(!imgsrc)
		{
			MainWindow_errmes(MLKERR_ALLOC, NULL);
			return;
		}

		info.imgsrc = imgsrc;
	}

	//準備

	drawOpSub_setDrawInfo_filter();
	drawOpSub_beginDraw(APPDRAW);

	//スレッド

	ret = PopupThread_run(&info, _thread_filter);

	drawOpSub_finishDraw_single(APPDRAW);

	//複製イメージ削除

	TileImage_free(imgsrc);

	//エラー

	if(ret != 1)
		MainWindow_errmes(MLKERR_UNKNOWN, NULL);
}

/** フィルタコマンド実行 */

void MainWindow_runFilterCommand(int id)
{
	const _cmddat *dat;

	dat = g_filter_cmd_dat + (id - TRMENU_FILTER_ID_TOP);

	_run_filter(id, dat->dlgdat, dat->func, dat->flags);
}

