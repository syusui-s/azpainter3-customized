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
 * AppDraw
 * 矩形選択関連
 *****************************************/

#include "mlk_gui.h"
#include "mlk_rectbox.h"
#include "mlk_str.h"
#include "mlk_sysdlg.h"

#include "def_widget.h"
#include "def_draw.h"

#include "draw_main.h"
#include "draw_calc.h"
#include "draw_op_sub.h"

#include "tileimage.h"
#include "tileimage_drawinfo.h"
#include "layeritem.h"
#include "appresource.h"

#include "mainwindow.h"
#include "maincanvas.h"
#include "panel_func.h"
#include "filedialog.h"
#include "popup_thread.h"

#include "trid.h"


//------------

int TransformDlg_run(mWindow *parent,TileImage *imgcopy,double *dstval,mRect *rcdst);

//------------


/** 矩形範囲を解除
 *
 * direct: キャンバスを直接更新 */

void drawBoxSel_release(AppDraw *p,mlkbool direct)
{
	mBox box;

	if(p->boxsel.box.w)
	{
		//カーソル戻す

		drawBoxSel_restoreCursor(p);

		//

		box = p->boxsel.box;

		p->boxsel.box.w = 0;

		drawUpdateBox_canvaswg_forBoxSel(p, &box, direct);
	}
}

/** 矩形範囲を mRect からセット
 *
 * [!] 1x1 px の場合は解除として取り扱う。 */

void drawBoxSel_setRect(AppDraw *p,const mRect *rc)
{
	mBox box;

	if(drawCalc_image_rect_to_box(p, &box, rc))
	{
		if(!(box.w == 1 && box.h == 1))
		{
			p->boxsel.box = box;

			drawUpdateBox_canvaswg_forBoxSel(p, &box, FALSE);
		}
	}
}

/** コピーイメージをクリア */

void drawBoxSel_clearImage(AppDraw *p)
{
	TileImage_free(p->boxsel.img);
	p->boxsel.img = NULL;
}

/** リサイズカーソルの状態の場合、元に戻す */

void drawBoxSel_restoreCursor(AppDraw *p)
{
	if(p->boxsel.flag_resize_cursor)
	{
		MainCanvasPage_setCursor_forTool();

		p->boxsel.flag_resize_cursor = 0;
	}
}

/** 貼り付けモードを解除 */

void drawBoxSel_cancelPaste(AppDraw *p)
{
	mRect rc;

	if(p->boxsel.is_paste_mode)
	{
		mRectSetBox(&rc, &p->boxsel.box);
	
		p->boxsel.is_paste_mode = FALSE;
		p->boxsel.box.w = 0;

		drawUpdateRect_canvas_forBoxSel(p, &rc);

		//貼り付けモード中にキャンバスビューが更新されていた場合、
		//貼り付けイメージを消すために再更新。

		if(p->is_canvview_update)
			PanelCanvasView_update();
	}
}

/** 選択の状態を変更する必要がある時
 *
 * type: [0] キャンバス/キャンバスサイズ変更後 [1] ビット数変更時 [2] ツール変更時 */

void drawBoxSel_onChangeState(AppDraw *p,int type)
{
	switch(type)
	{
		//キャンバス変更時
		// :更新は行わない。
		case 0:
			drawBoxSel_restoreCursor(p);

			p->boxsel.is_paste_mode = FALSE;
			p->boxsel.box.w = 0;

			//ビット数が異なる場合、イメージクリア

			if(p->imgbits != p->boxsel.copyimg_bits)
				drawBoxSel_clearImage(p);
			break;

		//すべて解除
		case 1:
		case 2:
			drawBoxSel_clearImage(p);

			if(p->boxsel.is_paste_mode)
				drawBoxSel_cancelPaste(p);
			else
				drawBoxSel_release(p, FALSE);
			break;
	}
}


//============================
// 切り貼りツール:実行
//============================


/** コピー/切り取り */

mlkbool drawBoxSel_copy_cut(AppDraw *p,mlkbool cut)
{
	if(p->boxsel.box.w == 0
		|| p->boxsel.is_paste_mode
		|| drawOpSub_canDrawLayer_mes(p, (cut)? 0: CANDRAWLAYER_F_ENABLE_READ))
		return FALSE;

	//イメージ削除

	drawBoxSel_clearImage(p);

	//

	if(!cut)
	{
		//コピー

		p->boxsel.img = TileImage_createBoxSelCopyImage(p->curlayer->img, &p->boxsel.box, FALSE);
	}
	else
	{
		//切り取り

		drawOpSub_setDrawInfo_select_cut();
		drawOpSub_beginDraw(p);

		p->boxsel.img = TileImage_createBoxSelCopyImage(p->curlayer->img, &p->boxsel.box, TRUE);

		drawOpSub_finishDraw_single(p);
	}

	p->boxsel.copyimg_bits = p->imgbits;
	p->boxsel.size_img.w = p->boxsel.box.w;
	p->boxsel.size_img.h = p->boxsel.box.h;

	return TRUE;
}

/** 貼り付け
 *
 * 貼り付け中はカレントレイヤを変更できるため、
 * 最終的な貼り付け時に描画可能かどうか判断する。
 *
 * ptstart: 貼り付け開始時の左上イメージ位置 (NULL で自動) */

mlkbool drawBoxSel_paste(AppDraw *p,const mPoint *ptstart)
{
	double dx,dy;
	int x,y;

	//コピーイメージがない or 貼り付け中
	
	if(!p->boxsel.img
		|| p->boxsel.is_paste_mode)
		return FALSE;

	//現在の範囲を解除

	drawBoxSel_release(p, FALSE);

	//貼り付けモードへ

	p->boxsel.is_paste_mode = TRUE;
	p->is_canvview_update = FALSE;

	//貼り付けイメージの開始位置

	if(ptstart)
	{
		x = ptstart->x;
		y = ptstart->y;
	}
	else
	{
		//キャンバス中央のイメージ位置

		drawCalc_getImagePos_atCanvasCenter(p, &dx, &dy);

		x = dx - p->boxsel.size_img.w * 0.5;
		y = dy - p->boxsel.size_img.h * 0.5;
	}

	mBoxSet(&p->boxsel.box, x, y, p->boxsel.size_img.w, p->boxsel.size_img.h);

	//貼り付けイメージを範囲の位置に合わせる

	TileImage_setOffset(p->boxsel.img, p->boxsel.box.x, p->boxsel.box.y);

	drawUpdateRect_canvas_forBoxSel(p, NULL);

	return TRUE;
}

/* 画像ファイルから読み込んでコピー */

static mlkbool _cutpaste_load_image(AppDraw *p)
{
	mStr str = MSTR_INIT;
	mSize size;
	mlkerr ret;

	if(!FileDialog_openImage_confdir(MLK_WINDOW(APPWIDGET->mainwin),
		AppResource_getOpenFileFilter_normal(), 0, &str))
	{
		return FALSE;
	}
	else
	{
		ret = drawSub_loadTileImage(&p->boxsel.img, str.buf, &size);

		mStrFree(&str);

		if(ret)
		{
			MainWindow_errmes(ret, NULL);
			return FALSE;
		}
		else
		{
			p->boxsel.copyimg_bits = p->imgbits;
			p->boxsel.size_img = size;
		}
	}

	return TRUE;
}

/** 切り貼りコマンド実行 */

void drawBoxSel_run_cutpaste(AppDraw *p,int no)
{
	switch(no)
	{
		//コピー
		case 0:
			drawBoxSel_copy_cut(p, FALSE);
			break;
		//切り取り
		case 1:
			drawBoxSel_copy_cut(p, TRUE);
			break;
		//貼り付け
		case 2:
			drawBoxSel_paste(p, NULL);
			break;
		//画像から貼り付け
		case 3:
			if(!p->boxsel.is_paste_mode)
			{
				if(_cutpaste_load_image(p))
					drawBoxSel_paste(p, NULL);
			}
			break;
	}
}


//============================
// 矩形編集・変形
//============================


typedef struct
{
	int type;
	TileImage *imgdst,*imgcopy;
	double *dparam;
	mBox box;
	const mRect *rcdst;
}_thdata_trans;


/* スレッド */

static int _thread_transform(mPopupProgress *prog,void *data)
{
	_thdata_trans *p = (_thdata_trans *)data;
	double *d = p->dparam;

	if(p->type == 0)
	{
		//アフィン変換
		
		TileImage_transformAffine(p->imgdst, p->imgcopy, &p->box,
			d[0], d[1], d[2], d[3], d[4], d[5], prog);
	}
	else
	{
		//射影変換

		TileImage_transformHomography(p->imgdst, p->imgcopy, p->rcdst,
			d, p->box.x, p->box.y, p->box.w, p->box.h, prog);
	}

	return 1;
}

/* 描画処理 */

static void _draw_transform(AppDraw *p,TileImage *imgcopy,int type,double *dval,const mRect *rcdst)
{
	mBox box;
	_thdata_trans dat;

	box = p->boxsel.box;

	drawOpSub_setDrawInfo_overwrite();
	drawOpSub_beginDraw(p);

	//範囲内を消去
	// :色関数は上書きにセットされている

	TileImage_clearBox_forTransform(p->w.dstimg, p->tileimg_sel, &box);

	//変形描画は重ね塗り

	drawOpSub_setDrawInfo_pixelcol(TILEIMAGE_PIXELCOL_NORMAL);

	//スレッド

	dat.type = type;
	dat.imgdst = p->w.dstimg;
	dat.imgcopy = imgcopy;
	dat.dparam = dval;
	dat.box = box;
	dat.rcdst = rcdst;

	PopupThread_run(&dat, _thread_transform);

	drawOpSub_finishDraw_single(p);
}

/* 変形 */

static void _run_transform(AppDraw *p)
{
	TileImage *imgcopy;
	mBox box;
	mRect rc;
	double dval[11];
	int ret;

	if(drawOpSub_canDrawLayer_mes(p, 0)) return;

	box = p->boxsel.box;

	//範囲内イメージをコピー & 消去
	// :選択範囲内のみ。
	// :合成イメージで元範囲を非表示にするため、一時的に範囲内を消去する。

	imgcopy = TileImage_createCopyImage_forTransform(p->curlayer->img, p->tileimg_sel, &box);
	if(!imgcopy) return;

	//ダイアログの表示用に imgcanvas を更新
	// :キャンバスは更新しない。

	drawUpdate_blendImage_full(p, &box);

	//ダイアログ中の XOR 描画用

	g_tileimage_dinfo.func_setpixel = TileImage_setPixel_new;

	//ダイアログ

	ret = TransformDlg_run(MLK_WINDOW(APPWIDGET->mainwin), imgcopy, dval, &rc);

	//消去した範囲を元に戻す

	TileImage_restoreCopyImage_forTransform(p->curlayer->img, imgcopy, &box);

	//

	if(ret >= 0)
		//描画
		_draw_transform(p, imgcopy, ret, dval, &rc);
	else
	{
		//キャンセル時は imgcanvas を元に戻す
		// :OK 時は描画時に更新されるので必要ない。

		drawUpdate_blendImage_full(p, &box);
	}

	//イメージ解放

	TileImage_free(imgcopy);
}


//=======================
// 矩形編集
//=======================

enum
{
	_BOXEDIT_NO_FLIP_HORZ,
	_BOXEDIT_NO_FLIP_VERT,
	_BOXEDIT_NO_ROTATE_LEFT,
	_BOXEDIT_NO_ROTATE_RIGHT,
	_BOXEDIT_NO_TRANSFORM,
	_BOXEDIT_NO_TRIMMING,
	
	_BOXEDIT_NO_SCALEUP = 100,
	_BOXEDIT_NO_TILES_FULL,
	_BOXEDIT_NO_TILES_HORZ,
	_BOXEDIT_NO_TILES_VERT
};


/* 各処理共通 */

static void _run_common(AppDraw *p,int no,int param)
{
	TileImage *img = NULL;
	mBox box;

	if(drawOpSub_canDrawLayer_mes(p, 0)) return;

	box = p->boxsel.box;

	//----------

	drawOpSub_setDrawInfo_overwrite();
	drawOpSub_beginDraw_single(p);

	//範囲内イメージをコピー
	// :回転時は、回転前と回転後の範囲が異なる場合があるため、クリアする。
	// :その場合、すでに描画を開始しているので、イメージの作成失敗時も続ける。

	if(no >= _BOXEDIT_NO_ROTATE_LEFT)
	{
		img = TileImage_createBoxSelCopyImage(p->w.dstimg, &box,
			(no == _BOXEDIT_NO_ROTATE_LEFT || no == _BOXEDIT_NO_ROTATE_RIGHT));
	}

	//描画

	switch(no)
	{
		//反転 (コピーイメージ必要なし)
		case _BOXEDIT_NO_FLIP_HORZ:
			TileImage_flipHorz_box(p->w.dstimg, &box);
			break;
		case _BOXEDIT_NO_FLIP_VERT:
			TileImage_flipVert_box(p->w.dstimg, &box);
			break;
		//回転
		case _BOXEDIT_NO_ROTATE_LEFT:
			TileImage_rotateLeft_box(p->w.dstimg, img, &box);
			break;
		case _BOXEDIT_NO_ROTATE_RIGHT:
			TileImage_rotateRight_box(p->w.dstimg, img, &box);
			break;
		//拡大
		case _BOXEDIT_NO_SCALEUP:
			TileImage_scaleup_nearest(p->w.dstimg, img, &box, param);
			break;
		//タイル状に並べる(全体)
		case _BOXEDIT_NO_TILES_FULL:
			TileImage_putTiles_full(p->w.dstimg, img, &box);
			break;
		//タイル状に並べる(横一列)
		case _BOXEDIT_NO_TILES_HORZ:
			TileImage_putTiles_horz(p->w.dstimg, img, &box);
			break;
		//タイル状に並べる(縦一列)
		case _BOXEDIT_NO_TILES_VERT:
			TileImage_putTiles_vert(p->w.dstimg, img, &box);
			break;
	}

	TileImage_free(img);

	drawOpSub_endDraw_single(p);
}

/* トリミング */

static void _run_trimming(AppDraw *p)
{
	mBox box;

	box = p->boxsel.box;

	if(!drawImage_resizeCanvas(p, box.w, box.h, -box.x, -box.y, TRUE))
		MainWindow_errmes(MLKERR_ALLOC, NULL);

	MainWindow_updateNewCanvas(APPWIDGET->mainwin, NULL);
}

/* 拡大 */

static void _run_scaleup(AppDraw *p,int no)
{
	int n = 2;

	MLK_TRGROUP(TRGROUP_DLG_BOXEDIT_SCALE);

	if(mSysDlg_inputTextNum(MLK_WINDOW(APPWIDGET->mainwin),
		MLK_TR(0), MLK_TR(1), MSYSDLG_INPUTTEXT_F_SET_DEFAULT,
		2, 20, &n))
	{
		_run_common(p, no, n);
	}
}

/** 矩形編集実行 */

void drawBoxSel_run_edit(AppDraw *p,int no)
{
	if(p->boxsel.box.w == 0) return;

	switch(no)
	{
		//変形
		case _BOXEDIT_NO_TRANSFORM:
			_run_transform(p);
			break;
		//トリミング
		case _BOXEDIT_NO_TRIMMING:
			_run_trimming(p);
			break;
		//拡大
		case _BOXEDIT_NO_SCALEUP:
			_run_scaleup(p, no);
			break;

		default:
			_run_common(p, no, 0);
			break;
	}
}

