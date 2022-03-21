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

/*************************************
 * フィルタダイアログ
 *************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_label.h"
#include "mlk_checkbutton.h"
#include "mlk_button.h"
#include "mlk_lineedit.h"
#include "mlk_combobox.h"
#include "mlk_event.h"
#include "mlk_str.h"
#include "mlk_rectbox.h"
#include "mlk_util.h"

#include "def_config.h"
#include "def_draw.h"

#include "layeritem.h"
#include "def_tileimage.h"
#include "tileimage.h"
#include "tileimage_drawinfo.h"
#include "appcursor.h"

#include "def_filterdraw.h"

#include "filterbar.h"
#include "filterprev.h"
#include "filter_save_param.h"

#include "panel_func.h"

#include "draw_main.h"
#include "draw_calc.h"
#include "draw_op_sub.h"

#include "trid.h"


//--------------------

typedef struct
{
	MLK_DIALOG_DEF

	int menuid;		//各フィルタのメニュー項目文字列ID
	FilterDrawInfo *info;

	TileImage *img_prev,	//プレビュー用イメージ
		*img_current;		//カレントイメージ保存用 (キャンバスプレビュー時)

	uint8_t dat_flags,
		prevtype,		//プレビュータイプ
		fpreview;		//プレビューのチェックが ON か

	char saveid_bar[FILTER_BAR_NUM],		//データ保存の識別ID (文字)
		saveid_combo[FILTER_COMBOBOX_NUM],
		saveid_ckbtt[FILTER_COMBOBOX_NUM];

	int wgnum_bar,		//各ウィジェットの作成数
		wgnum_ckbtt,
		wgnum_combo;

	int16_t resetval_bar[FILTER_BAR_NUM];	//リセット時用、バーの値

	char *savedata_text;	//パラメータ保存のテキスト (読み込み用。NULL でなし)

	mRect rc_update_last;	//漫画用プレビュー時、前回の更新範囲

	FilterPrev *prevwg;
	FilterBar *bar[FILTER_BAR_NUM];
	mLineEdit *edit_bar[FILTER_BAR_NUM];
	mCheckButton *ckbtt[FILTER_CHECKBTT_NUM];
	mComboBox *combo[FILTER_COMBOBOX_NUM];
	mWidget *wgdef;		//定義ウィジェット
}_dialog;

//--------------------

#define DATFLAGS_MASK_PREVTYPE  7		//プレビュータイプ
#define DATFLAGS_RESET_BUTTON   (1<<3)	//リセットボタンあり
#define DATFLAGS_GET_CANVAS_POS (1<<4)	//キャンバス位置取得
#define DATFLAGS_SAVE_DATA      (1<<5)	//データ保存

//プレビュータイプ
enum
{
	PREVTYPE_NONE,			//なし
	PREVTYPE_IN_DIALOG,		//ダイアログ内
	PREVTYPE_CANVAS_FOR_COLOR,	//キャンバスプレビュー (カラー処理用)
	PREVTYPE_CANVAS_FOR_COMIC	//キャンバスプレビュー (漫画用)
};

//ウィジェットタイプ
enum
{
	WGTYPE_BAR,
	WGTYPE_BAR_TYPE,
	WGTYPE_CHECKBUTTON,
	WGTYPE_COMBOBOX,
	WGTYPE_LABEL,

	WGTYPE_DEF_CLIPPING = 128,
	WGTYPE_DEF_LEVEL,
	WGTYPE_DEF_REPLACE_COL
};

//バータイプ
enum
{
	WG_BAR_TYPE_IMGBITVAL
};

//--------------------

enum
{
	WID_FILTERPREV = 100,
	WID_CK_PREVIEW,
	WID_BTT_RESET,

	WID_BAR_TOP = 200,
	WID_BAREDIT_TOP = 250,
	WID_CKBTT_TOP = 300,
	WID_COMBO_TOP = 400,

	WID_DEF_LEVEL = 500,
	WID_DEF_CLIPPING,
	WID_DEF_REPLACE_COL
};

enum
{
	TRID_RESET = 1,
	TRID_CLIPPING
};

//--------------------

typedef struct _FilterWgLevel FilterWgLevel;

FilterWgLevel *FilterWgLevel_new(mWidget *parent,int id,int bits,uint32_t *histogram);
void FilterWgLevel_getValue(FilterWgLevel *p,int *buf);

mWidget *FilterWgRepcol_new(mWidget *parent,int id,RGB8 *drawcol);
void FilterWgRepcol_getColor(mWidget *wg,int *dst);

//--------------------



//===========================
// プレビュー
//===========================


/* プレビューを ON->OFF 時、元のイメージを表示 */

static void _change_preview_off(_dialog *p)
{
	FilterDrawInfo *info = p->info;

	switch(p->prevtype)
	{
		//ダイアログ内プレビュー
		case PREVTYPE_IN_DIALOG:
			if(p->prevwg && p->img_prev)
			{
				TileImage_freeAllTiles(p->img_prev);

				FilterPrev_drawImage(p->prevwg, info->imgsrc);
			}
			break;

		//キャンバスプレビュー (カラー処理用)
		case PREVTYPE_CANVAS_FOR_COLOR:
			if(p->img_prev)
			{
				mWindowSetCursor(MLK_WINDOW(p), AppCursor_getWaitCursor());

				APPDRAW->curlayer->img = p->img_current;

				drawUpdateRect_canvas(APPDRAW, &info->rc);

				APPDRAW->curlayer->img = info->imgdst;

				mWindowResetCursor(MLK_WINDOW(p));
			}
			break;

		//キャンバスプレビュー (漫画用)
		// :前回の描画範囲がある場合、クリア
		case PREVTYPE_CANVAS_FOR_COMIC:
			if(p->img_prev && !mRectIsEmpty(&p->rc_update_last))
			{
				mWindowSetCursor(MLK_WINDOW(p), AppCursor_getWaitCursor());

				TileImage_freeAllTiles(p->img_prev);

				drawUpdateRect_canvas(APPDRAW, &p->rc_update_last);

				mRectEmpty(&p->rc_update_last);

				mWindowResetCursor(MLK_WINDOW(p));
			}
			break;
	}
}

/* プレビュー実行 */

static void _run_preview(_dialog *p)
{
	FilterDrawInfo *info = p->info;
	mRect rc;

	switch(p->prevtype)
	{
		//ダイアログ内プレビュー
		case PREVTYPE_IN_DIALOG:
			if(p->fpreview && p->prevwg && p->img_prev)
			{
				mWindowSetCursor(MLK_WINDOW(p), AppCursor_getWaitCursor());

				TileImage_freeAllTiles(p->img_prev);

				(info->func_draw)(info);

				FilterPrev_drawImage(p->prevwg, p->img_prev);

				mWindowResetCursor(MLK_WINDOW(p));
			}
			break;

		//キャンバスプレビュー (カラー処理用)
		case PREVTYPE_CANVAS_FOR_COLOR:
			if(p->fpreview && p->img_prev)
			{
				mWindowSetCursor(MLK_WINDOW(p), AppCursor_getWaitCursor());

				(info->func_draw)(info);

				drawUpdateRect_canvas(APPDRAW, &info->rc);

				mWindowResetCursor(MLK_WINDOW(p));
			}
			break;

		//キャンバスプレビュー (漫画用)
		case PREVTYPE_CANVAS_FOR_COMIC:
			if(p->fpreview && p->img_prev)
			{
				mWindowSetCursor(MLK_WINDOW(p), AppCursor_getWaitCursor());

				//前回の描画範囲がある場合、クリア

				rc = p->rc_update_last;

				if(!mRectIsEmpty(&p->rc_update_last))
				{
					TileImage_freeAllTiles(p->img_prev);
					mRectEmpty(&p->rc_update_last);
				}

				//描画

				TileImageDrawInfo_clearDrawRect();

				(info->func_draw)(info);

				//更新 (前回の描画範囲と合わせた範囲)

				mRectUnion(&rc, &g_tileimage_dinfo.rcdraw);

				drawUpdateRect_canvas(APPDRAW, &rc);

				//描画範囲記録

				p->rc_update_last = g_tileimage_dinfo.rcdraw;

				mWindowResetCursor(MLK_WINDOW(p));
			}
			break;
	}
}

/* キャンバスプレビュー時の描画処理範囲を取得
 *
 * return: FALSE で処理範囲がない */

static mlkbool _get_canvasprev_rect(FilterDrawInfo *info,int prevtype)
{
	mRect rc;

	if(prevtype == PREVTYPE_CANVAS_FOR_COMIC)
	{
		//---- 漫画用 (キャンバス全体)
		
		rc.x1 = rc.y1 = 0;
		rc.x2 = APPDRAW->imgw - 1;
		rc.y2 = APPDRAW->imgh - 1;
	}
	else
	{
		//---- カラー用
	
		if(drawSel_isHave())
			//選択範囲がある場合は、選択範囲
			rc = APPDRAW->sel.rcsel;
		else
		{
			//選択範囲がない場合
			// :タイルがある範囲。
			// :空イメージの場合は処理しても意味がないので、プレビューなし。
				
			if(!TileImage_getHaveImageRect_pixel(APPDRAW->curlayer->img, &rc, NULL))
				return FALSE;
		}

		//キャンバス範囲外は処理しても見えないので、クリッピング

		if(!drawCalc_clipImageRect(APPDRAW, &rc))
			return FALSE;
	}

	//セット

	info->rc = rc;

	mBoxSetRect(&info->box, &rc);

	return TRUE;
}

/* プレビュー初期化 */

static void _init_preview(_dialog *p)
{
	FilterDrawInfo *info = p->info;
	TileImage *curimg;

	curimg = APPDRAW->curlayer->img;

	switch(p->prevtype)
	{
		//ダイアログ内プレビュー
		// :img_prev の作成に失敗した場合、プレビューなし
		case PREVTYPE_IN_DIALOG:
			if(p->prevwg)
			{
				//プレビューの描画範囲を mRect,mBox で取得
				
				FilterPrev_getDrawArea(p->prevwg, &info->rc, &info->box);

				//プレビュー用イメージ作成

				p->img_prev = TileImage_newFromRect(curimg->type, &info->rc);

				if(p->img_prev)
				{
					p->img_prev->col = curimg->col;

					//

					info->imgsrc = curimg;
					info->imgdst = p->img_prev;

					//プレビューが初期状態で ON の場合は、プレビュー実行。
					//OFF の場合は、ソースイメージをセット。

					if(p->fpreview)
						_run_preview(p);
					else
						FilterPrev_drawImage(p->prevwg, curimg);
				}
			}
			break;

		//キャンバスプレビュー (カラー処理用)
		// :カレントレイヤのイメージを、プレビュー用画像と置き換える。
		// :元のイメージをソースとし、プレビュー用画像に描画する。
		// :(img_prev が NULL でプレビューなし)
		case PREVTYPE_CANVAS_FOR_COLOR:
			if(_get_canvasprev_rect(info, PREVTYPE_CANVAS_FOR_COLOR))
			{
				//プレビュー用にカレントイメージを複製
			
				p->img_prev = TileImage_newClone(curimg);

				if(p->img_prev)
				{
					//カレントレイヤのイメージと置換
					
					p->img_current = curimg;

					APPDRAW->curlayer->img = p->img_prev;

					//

					info->imgsrc = curimg;
					info->imgdst = p->img_prev;

					_run_preview(p);
				}
			}
			break;

		//キャンバスプレビュー (漫画用)
		case PREVTYPE_CANVAS_FOR_COMIC:
			if(_get_canvasprev_rect(info, PREVTYPE_CANVAS_FOR_COMIC))
			{
				//イメージ作成 (色は1色のみなので、A16)
				
				p->img_prev = TileImage_newFromRect(TILEIMAGE_COLTYPE_ALPHA, &info->rc);

				if(p->img_prev)
				{
					//描画初期化

					g_tileimage_dinfo.func_setpixel = TileImage_setPixel_work;

					//

					APPDRAW->tileimg_filterprev = p->img_prev; //キャンバス更新時に挿入させる

					info->imgsrc = curimg;
					info->imgdst = p->img_prev;

					mRectEmpty(&p->rc_update_last);

					_run_preview(p);
				}
			}
			break;
	}
}

/* エディットの内容変更時のプレビュー */

static void _run_preview_edit(_dialog *p)
{
	if(p->prevtype != PREVTYPE_NONE && p->fpreview)
	{
		//タイマーで遅延
		mWidgetTimerAdd(MLK_WIDGET(p), 0, 300, 0);
	}
}


//===========================
// イベント
//===========================


/* 値をリセット */

static void _reset_value(_dialog *p)
{
	int i,n;

	//バーの値

	for(i = 0; i < p->wgnum_bar; i++)
	{
		n = p->resetval_bar[i];
		
		FilterBar_setPos(p->bar[i], n);
		mLineEditSetNum(p->edit_bar[i], n);

		p->info->val_bar[i] = n;
	}

	_run_preview(p);
}

/* 通知 */

static void _event_notify(_dialog *p,mEventNotify *ev)
{
	FilterDrawInfo *info = p->info;
	int id,ntype,n;

	id = ev->id;
	ntype = ev->notify_type;

	if(id >= WID_BAR_TOP && id < WID_BAR_TOP + FILTER_BAR_NUM)
	{
		//バー

		if(ev->param2)
			//確定
			_run_preview(p);
		else
		{
			//値の変更中
			
			id -= WID_BAR_TOP;
			n = ev->param1;

			info->val_bar[id] = n;

			mLineEditSetNum(p->edit_bar[id], n);
		}
	}
	else if(id >= WID_BAREDIT_TOP && id < WID_BAREDIT_TOP + FILTER_BAR_NUM)
	{
		//バーのエディット

		if(ntype == MLINEEDIT_N_CHANGE)
		{
			id -= WID_BAREDIT_TOP;
			n = mLineEditGetNum(p->edit_bar[id]);

			info->val_bar[id] = n;

			FilterBar_setPos(p->bar[id], n);

			_run_preview_edit(p);
		}
	}
	else if(id >= WID_COMBO_TOP && id < WID_COMBO_TOP + FILTER_COMBOBOX_NUM)
	{
		//コンボボックス

		if(ntype == MCOMBOBOX_N_CHANGE_SEL)
		{
			info->val_combo[id - WID_COMBO_TOP] = ev->param2;

			_run_preview(p);
		}
	}
	else if(id >= WID_CKBTT_TOP && id < WID_CKBTT_TOP + FILTER_CHECKBTT_NUM)
	{
		//チェックボタン

		info->val_ckbtt[id - WID_CKBTT_TOP] ^= 1;

		_run_preview(p);
	}
	else
	{
		//----- ほか
		
		switch(id)
		{
			//プレビューの表示位置変更
			case WID_FILTERPREV:
				FilterPrev_getDrawArea(p->prevwg, &info->rc, &info->box);

				if(p->img_prev)
					TileImage_setOffset(p->img_prev, info->box.x, info->box.y);

				if(p->fpreview)
					_run_preview(p);
				else
					FilterPrev_drawImage(p->prevwg, info->imgsrc);
				break;
		
			//レベル補正
			case WID_DEF_LEVEL:
				FilterWgLevel_getValue((FilterWgLevel *)p->wgdef, info->val_bar);

				_run_preview(p);
				break;
			//クリッピング
			case WID_DEF_CLIPPING:
				info->clipping ^= 1;

				_run_preview(p);
				break;
			//色置き換え
			case WID_DEF_REPLACE_COL:
				FilterWgRepcol_getColor(p->wgdef, info->val_bar);
			
				if(ntype == 1)
					_run_preview_edit(p);
				else
					_run_preview(p);
				break;

			//プレビュー
			case WID_CK_PREVIEW:
				p->fpreview ^= 1;

				APPCONF->fview ^= CONFIG_VIEW_F_FILTERDLG_PREVIEW;

				if(p->fpreview)
					_run_preview(p);
				else
					_change_preview_off(p);
				break;
			//リセット
			case WID_BTT_RESET:
				_reset_value(p);
				break;
		}
	}
}

/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	switch(ev->type)
	{
		case MEVENT_NOTIFY:
			_event_notify((_dialog *)wg, (mEventNotify *)ev);
			break;

		//タイマー (プレビュー更新)
		case MEVENT_TIMER:
			mWidgetTimerDelete(wg, 0);

			_run_preview((_dialog *)wg);
			break;
	}

	return mDialogEventDefault_okcancel(wg, ev);
}


//===========================
// 項目ウィジェット作成
//===========================


/* ウィジェット作成: bar + edit */

static void _create_wg_bar(_dialog *p,mWidget *parent,const uint8_t *dat,int size)
{
	mWidget *ct;
	FilterBar *bar;
	mLineEdit *edit;
	int no,barw,min,max,def,center,dig,savid,reset_val;

	//----- 値

	barw = mGetBufBE16(dat);
	min = (int16_t)mGetBufBE16(dat + 2);
	max = (int16_t)mGetBufBE16(dat + 4);

	//初期値 (省略の場合、最小が負なら 0。そうでなければ最小値)

	if(size >= 8)
		def = (int16_t)mGetBufBE16(dat + 6);
	else
		def = (min < 0)? 0: min;

	//中央の値 (省略の場合、最小が負なら 0。そうでなければ最小値=なし)

	if(size >= 10)
		center = (int16_t)mGetBufBE16(dat + 8);
	else
		center = (min < 0)? 0: min;
	
	if(size > 10) dig = dat[10]; else dig = 0;
	if(size > 11) savid = dat[11]; else savid = 0;

	//リセット時の値

	reset_val = def;

	//保存値を取得して初期値に

	if(savid)
		FilterParam_getValue(p->savedata_text, savid, &def);

	//--------

	no = p->wgnum_bar;

	ct = mContainerCreateHorz(parent, 6, MLF_EXPAND_W, 0);

	//bar

	p->bar[no] = bar = FilterBar_new(ct, WID_BAR_TOP + no, MLF_EXPAND_W | MLF_MIDDLE, barw,
		min, max, def, center);

	//edit

	p->edit_bar[no] = edit = mLineEditCreate(ct, WID_BAREDIT_TOP + no,
		MLF_MIDDLE, 0, MLINEEDIT_S_SPIN | MLINEEDIT_S_NOTIFY_CHANGE);

	mLineEditSetWidth_textlen(edit, 6);
	mLineEditSetNumStatus(edit, min, max, dig);
	mLineEditSetNum(edit, def);

	//セット

	p->resetval_bar[no] = reset_val;
	p->saveid_bar[no] = savid;

	p->info->val_bar[no] = def; //初期値

	p->wgnum_bar++;
}

/* ウィジェット作成: バー(タイプ指定) */

static void _create_wg_bar_type(_dialog *p,mWidget *parent,const uint8_t *dat,int size)
{
	mWidget *ct;
	FilterBar *bar;
	mLineEdit *edit;
	int no,type,barw,min,max,def,center;

	//----- 値

	barw = mGetBufBE16(dat);
	type = dat[2];

	switch(type)
	{
		//ビット数分の値
		case WG_BAR_TYPE_IMGBITVAL:
			if(APPDRAW->imgbits == 8)
				max = 255, center = 128;
			else
				max = 0x8000, center = 0x4000;

			min = 0;
			def = center;
			break;
		default:
			return;
	}

	//--------

	no = p->wgnum_bar;

	ct = mContainerCreateHorz(parent, 6, MLF_EXPAND_W, 0);

	//bar

	p->bar[no] = bar = FilterBar_new(ct, WID_BAR_TOP + no, MLF_EXPAND_W | MLF_MIDDLE, barw,
		min, max, def, center);

	//edit

	p->edit_bar[no] = edit = mLineEditCreate(ct, WID_BAREDIT_TOP + no,
		MLF_MIDDLE, 0, MLINEEDIT_S_SPIN | MLINEEDIT_S_NOTIFY_CHANGE);

	mLineEditSetWidth_textlen(edit, 6);
	mLineEditSetNumStatus(edit, min, max, 0);
	mLineEditSetNum(edit, def);

	//セット

	p->resetval_bar[no] = def;

	p->info->val_bar[no] = def;

	p->wgnum_bar++;
}

/* ウィジェット作成: チェックボタン */

static void _create_wg_checkbtt(_dialog *p,mWidget *parent,int trid,const uint8_t *dat,int size)
{
	int no,savid,checked;

	//値

	checked = 0;

	if(size >= 1) checked = dat[0];
	if(size >= 2) savid = dat[1]; else savid = 0;

	if(savid)
		FilterParam_getValue(p->savedata_text, savid, &checked);

	//作成

	no = p->wgnum_ckbtt;

	p->ckbtt[no] = mCheckButtonCreate(parent, WID_CKBTT_TOP + no, 0, 0, 0,
		MLK_TR(trid), checked);

	//

	p->saveid_ckbtt[no] = savid;

	p->info->val_ckbtt[no] = (checked != 0);

	p->wgnum_ckbtt++;
}

/* ウィジェット作成: コンボボックス */

static void _create_wg_combobox(_dialog *p,mWidget *parent,const uint8_t *dat,int size)
{
	mComboBox *cb;
	int trid,def,no,savid;

	//値

	trid = mGetBufBE16(dat);

	if(size >= 3) def = dat[2]; else def = 0;
	if(size >= 4) savid = dat[3]; else savid = 0;

	if(savid)
		FilterParam_getValue(p->savedata_text, savid, &def);

	//作成

	no = p->wgnum_combo;

	p->combo[no] = cb = mComboBoxCreate(parent, WID_COMBO_TOP + no, 0, 0, 0);

	mComboBoxAddItems_sepnull(cb, MLK_TR(trid), 0);
	mComboBoxSetAutoWidth(cb);
	mComboBoxSetSelItem_atIndex(cb, def);

	//

	p->saveid_combo[no] = savid;

	p->info->val_combo[no] = def;

	p->wgnum_combo++;
}

/* ウィジェット作成: レベル補正 */

static void _create_wg_def_level(_dialog *p,mWidget *parent)
{
	uint32_t *hist;

	//ヒストグラム取得

	hist = TileImage_getHistogram(APPDRAW->curlayer->img);
	if(!hist) return;

	//作成

	p->wgdef = (mWidget *)FilterWgLevel_new(parent, WID_DEF_LEVEL, APPDRAW->imgbits, hist);

	mFree(hist);

	//値

	FilterWgLevel_getValue((FilterWgLevel *)p->wgdef, p->info->val_bar);
}

/* 項目ウィジェット作成 */

static void _create_item_widgets(_dialog *p,mWidget *parent,const uint8_t *dat)
{
	mWidget *ct;
	uint8_t type,trid,size;
	char m[2] = {0,0};

	ct = mContainerCreateVert(parent, 3, MLF_EXPAND_W, 0);

	while(1)
	{
		//ウィジェットタイプ
		
		type = *(dat++);
		if(type == 255) break;

		//

		if(type >= 128)
		{
			//----- 定義ウィジェット

			switch(type)
			{
				//レベル補正
				case WGTYPE_DEF_LEVEL:
					_create_wg_def_level(p, ct);
					break;
				//クリッピング
				case WGTYPE_DEF_CLIPPING:
					mCheckButtonCreate(ct, WID_DEF_CLIPPING, 0, MLK_MAKE32_4(0,3,0,0), 0,
						MLK_TR(TRID_CLIPPING), TRUE);

					p->info->clipping = TRUE;
					break;
				//描画色置換
				case WGTYPE_DEF_REPLACE_COL:
					p->wgdef = FilterWgRepcol_new(ct, WID_DEF_REPLACE_COL, &APPDRAW->col.drawcol.c8);

					FilterWgRepcol_getColor(p->wgdef, p->info->val_bar);
					break;
			}
		}
		else
		{
			//ラベルの文字列ID (0 でなし)
			
			trid = *(dat++);

			//ラベル

			if(trid &&
				(type == WGTYPE_BAR || type == WGTYPE_BAR_TYPE || type == WGTYPE_COMBOBOX))
			{
				//バー/コンボボックス
				
				if(trid == 255)
				{
					//次の 1byte が ASCII 文字
					
					m[0] = *(dat++);
					mLabelCreate(ct, 0, 0, MLABEL_S_COPYTEXT, m);
				}
				else
					mLabelCreate(ct, 0, 0, 0, MLK_TR(trid));
			}

			//各ウィジェット

			size = *(dat++);

			switch(type)
			{
				//バー
				case WGTYPE_BAR:
					_create_wg_bar(p, ct, dat, size);
					break;
				//バー(タイプ指定)
				case WGTYPE_BAR_TYPE:
					_create_wg_bar_type(p, ct, dat, size);
					break;
				//チェックボタン
				case WGTYPE_CHECKBUTTON:
					_create_wg_checkbtt(p, ct, trid, dat, size);
					break;
				//コンボボックス
				case WGTYPE_COMBOBOX:
					_create_wg_combobox(p, ct, dat, size);
					break;
				//ラベル
				case WGTYPE_LABEL:
					mLabelCreate(ct, 0, MLK_MAKE32_4(0,5,0,0), MLABEL_S_BORDER, MLK_TR(trid));
					break;
			}

			dat += size;
		}
	}
}


//===========================
// main
//===========================


/* 内容のウィジェット作成 */

static void _create_widgets(_dialog *p,const uint8_t *dat,mSize *prevsize)
{
	mWidget *ct,*wg;

	MLK_TRGROUP(TRGROUP_DLG_FILTER);

	//------- プレビュー + ウィジェット

	ct = mContainerCreateHorz(MLK_WIDGET(p), 0, MLF_EXPAND_W, 0);

	//プレビュー

	if(p->prevtype == PREVTYPE_IN_DIALOG)
	{
		p->prevwg = FilterPrev_new(ct, WID_FILTERPREV,
			prevsize->w, prevsize->h,
			APPDRAW->curlayer->img, APPDRAW->imgw, APPDRAW->imgh);
	}

	//項目ウィジェット

	_create_item_widgets(p, ct, dat);

	//------ ボタン

	ct = mContainerCreateHorz(MLK_WIDGET(p), 4, MLF_EXPAND_W, MLK_MAKE32_4(0,15,0,0));

	//プレビューチェック

	if(p->prevtype != PREVTYPE_NONE)
	{
		mCheckButtonCreate(ct, WID_CK_PREVIEW, 0, MLK_MAKE32_4(0,0,4,0), 0,
			MLK_TR2(TRGROUP_WORD, TRID_WORD_PREVIEW), p->fpreview);
	}

	//OK

	wg = (mWidget *)mButtonCreate(ct, MLK_WID_OK, MLF_EXPAND_X | MLF_RIGHT,
		0, 0, MLK_TR_SYS(MLK_TRSYS_OK));

	wg->fstate |= MWIDGET_STATE_ENTER_SEND;

	//キャンセル

	mButtonCreate(ct, MLK_WID_CANCEL, 0, 0, 0, MLK_TR_SYS(MLK_TRSYS_CANCEL));

	//リセット

	if(p->dat_flags & DATFLAGS_RESET_BUTTON)
		mButtonCreate(ct, WID_BTT_RESET, 0, 0, 0, MLK_TR(TRID_RESET));
}

/* ダイアログ作成 */

static _dialog *_create_dialog(mWindow *parent,
	int menuid,const uint8_t *dat,FilterDrawInfo *info)
{
	_dialog *p;
	mSize prevsize;

	p = (_dialog *)mDialogNew(parent, sizeof(_dialog), MTOPLEVEL_S_DIALOG_NORMAL);
	if(!p) return NULL;

	p->menuid = menuid;
	p->info = info;
	p->fpreview = ((APPCONF->fview & CONFIG_VIEW_F_FILTERDLG_PREVIEW) != 0);

	p->wg.event = _event_handle;

	//

	mContainerSetPadding_same(MLK_CONTAINER(p), 8);

	mToplevelSetTitle(MLK_TOPLEVEL(p), MLK_TR2(TRGROUP_MAINMENU, menuid));

	//----- データからウィジェット作成

	//先頭データ

	p->dat_flags = *(dat++);
	p->prevtype = p->dat_flags & DATFLAGS_MASK_PREVTYPE;

	//プレビューサイズ (ダイアログ内プレビュー時)

	if(p->prevtype == PREVTYPE_IN_DIALOG)
	{
		prevsize.w = mGetBufBE16(dat);
		prevsize.h = mGetBufBE16(dat + 2);

		dat += 4;
	}

	//保存データのテキストを取得

	if(p->dat_flags & DATFLAGS_SAVE_DATA)
		p->savedata_text = FilterParam_getText(&APPCONF->list_filterparam, menuid);

	//ウィジェット作成

	_create_widgets(p, dat, &prevsize);

	//--------------

	//キャンバス上でのクリックを取得するため、ダイアログのポインタセット

	APPDRAW->w.ptmp = p;

	//キャンバス位置のデフォルトをイメージ中央に

	if(p->dat_flags & DATFLAGS_GET_CANVAS_POS)
	{
		info->imgx = APPDRAW->imgw / 2;
		info->imgy = APPDRAW->imgh / 2;
	}

	//選択範囲

	info->imgsel = APPDRAW->tileimg_sel;

	//プレビュー初期化

	_init_preview(p);

	return p;
}


//==============================


/* データ保存: 文字列に追加 */

static void _save_data_add(mStr *str,char id,int val)
{
	if(id)
	{
		mStrAppendChar(str, id);
		mStrAppendInt(str, val);
	}
}

/* データ保存 */

static void _save_data(_dialog *p)
{
	mStr str = MSTR_INIT;
	int i;

	//バー

	for(i = 0; i < p->wgnum_bar; i++)
		_save_data_add(&str, p->saveid_bar[i], p->info->val_bar[i]);

	//チェック

	for(i = 0; i < p->wgnum_ckbtt; i++)
		_save_data_add(&str, p->saveid_ckbtt[i], p->info->val_ckbtt[i]);

	//コンボボックス

	for(i = 0; i < p->wgnum_combo; i++)
		_save_data_add(&str, p->saveid_combo[i], p->info->val_combo[i]);

	FilterParam_setData(&APPCONF->list_filterparam, p->menuid, str.buf);

	mStrFree(&str);
}

/* 終了処理 */

static void _finish(_dialog *p,mlkbool ok)
{
	//タイマー消去

	mWidgetTimerDelete(MLK_WIDGET(p), 0);

	//プレビュー終了処理

	switch(p->prevtype)
	{
		//キャンバスプレビュー (カラー用)
		case PREVTYPE_CANVAS_FOR_COLOR:
			if(p->img_prev)
			{
				//カレントレイヤのイメージポインタを戻す
				
				APPDRAW->curlayer->img = p->img_current;

				//キャンセル時は元に戻す
				
				if(!ok)
					drawUpdateRect_canvas(APPDRAW, &p->info->rc);

				//ダイアログ中に、プレビューの画像でキャンバスビューパネルが更新された時は再描画

				if(APPDRAW->is_canvview_update)
					PanelCanvasView_update();
			}
			break;

		//漫画用 (前回の描画範囲があれば更新)
		case PREVTYPE_CANVAS_FOR_COMIC:
			if(p->img_prev)
			{
				APPDRAW->tileimg_filterprev = NULL;
			
				drawUpdateRect_canvas(APPDRAW, &p->rc_update_last);

				if(APPDRAW->is_canvview_update)
					PanelCanvasView_update();
			}
			break;
	}

	TileImage_free(p->img_prev);

	//OK 時、データ保存

	if(ok && (p->dat_flags & DATFLAGS_SAVE_DATA))
		_save_data(p);

	//キャンバスプレビュー時、ウィンドウ位置記録

	if(p->prevtype == PREVTYPE_CANVAS_FOR_COLOR
		|| p->prevtype == PREVTYPE_CANVAS_FOR_COMIC)
	{
		mToplevelGetSaveState(MLK_TOPLEVEL(p), &APPCONF->winstate_filterdlg);
	}
}

/* 初期表示 (キャンバスプレビュー時) */

static void _show_canvas_preview(_dialog *p)
{
	if(APPCONF->winstate_filterdlg.flags & MTOPLEVEL_SAVESTATE_F_HAVE_POS)
		mToplevelSetSaveState(MLK_TOPLEVEL(p), &APPCONF->winstate_filterdlg);

	mWindowResizeShow_initSize(MLK_WINDOW(p));
}

/** ダイアログ実行 */

mlkbool FilterDlg_run(mWindow *parent,int menuid,const uint8_t *dlgdat,FilterDrawInfo *info)
{
	_dialog *p;
	mlkbool ret;

	//ダイアログ作成

	p = _create_dialog(parent, menuid, dlgdat, info);
	if(!p) return FALSE;

	//位置、サイズ、表示

	if(p->prevtype == PREVTYPE_NONE || p->prevtype == PREVTYPE_IN_DIALOG)
		//プレビューなし or ダイアログ内
		mWindowResizeShow_initSize(MLK_WINDOW(p));
	else
		_show_canvas_preview(p);
	
	//
	
	ret = mDialogRun(MLK_DIALOG(p), FALSE);

	_finish(p, ret);

	mWidgetDestroy(MLK_WIDGET(p));

	return ret;
}

/** ダイアログ中、キャンバス上で左クリックされた時 */

void FilterDlg_onCanvasClick(double x,double y)
{
	_dialog *p = (_dialog *)APPDRAW->w.ptmp;
	mPoint pt;

	if(p->dat_flags & DATFLAGS_GET_CANVAS_POS)
	{
		drawCalc_canvas_to_image_pt(APPDRAW, &pt, x, y);

		p->info->imgx = pt.x;
		p->info->imgy = pt.y;

		_run_preview(p);
	}
}

