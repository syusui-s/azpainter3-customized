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

/******************************
 * AppDraw 定義
 ******************************/

#ifndef AZPT_DEF_DRAW_H
#define AZPT_DEF_DRAW_H

#include "def_draw_ptr.h"
#include "def_tool.h"
#include "def_drawtext.h"
#include "colorvalue.h"
#include "canvasinfo.h"

typedef struct _ImageCanvas ImageCanvas;
typedef struct _ImageMaterial ImageMaterial;
typedef struct _TileImage  TileImage;
typedef struct _LayerList  LayerList;
typedef struct _LayerItem  LayerItem;
typedef struct _FillPolygon FillPolygon;
typedef struct _DrawFont DrawFont;
typedef struct _PointBuf PointBuf;
typedef struct _AppDrawToolList AppDrawToolList;


#define DRAW_COLORMASK_NUM  6	//色マスクの色数
#define DRAW_GRADBAR_NUM    3	//中間色バーの数
#define DRAW_LAYERCOLPAL_NUM 30	//レイヤ色パレットの数
#define DRAW_RULE_RECORD_NUM 10	//定規の記録数
#define DRAW_HEADTAIL_REGIST_NUM 10	//入り抜き記録数


/*---- data ----*/


/** 描画用ポイントデータ */

typedef struct _DrawPoint
{
	double x,y,pressure;
}DrawPoint;

/** 定規の記録データ */

typedef struct
{
	int type;	//0 でなし
	double dval[4];
}RuleRecord;


/*---- sub data ----*/

/** ツールデータ */

typedef struct
{
	int no;		//現在のツール
	uint8_t subno[TOOL_NUM],	//ツールのサブ情報
		opt_stamp,			//スタンプ
		opt_select,			//選択範囲
		opt_cutpaste,		//切り貼り
		opt_boxedit;		//矩形編集
	uint16_t opt_fillpoly,	//図形塗り
		opt_fillpoly_erase;	//図形消し
	uint32_t opt_dotpen,	//ドットペン
		opt_dotpen_erase,	//ドットペン消しゴム
		opt_finger,			//指先
		opt_grad,			//グラデーション
		opt_fill,			//塗りつぶし
		opt_selfill;		//自動選択
}AppDrawTool;

/** 色関連データ */

typedef struct
{
	mList list_pal;		//パレットリスト
	mListItem *cur_pallist;	//現在のパレットリスト

	RGBcombo drawcol,	//描画色
		bkgndcol,		//背景色
		checkbkcol[2];	//チェック柄背景の色
	RGBA8 maskcol[DRAW_COLORMASK_NUM];	//色マスクの色 (a != 0 でON)

	uint32_t gradcol[DRAW_GRADBAR_NUM][2], //中間色、左右の色 ([0]の上位8bit = 段階数)
		layercolpal[DRAW_LAYERCOLPAL_NUM]; //レイヤ色パレット
	uint8_t colmask_type,	//色マスク:タイプ (0:OFF 1:MASK 2:REV)
		hslpal_hue,			//HSLパレット:H選択位置 (起動時初期化)
		hslpal_x,			//HSLパレット:パレット部分 XY 位置
		hslpal_y,
		pal_fmodify;	//パレットが変更されたか
}AppDrawColor;

/** 定規データ */

typedef struct
{
	uint8_t type,		//定規タイプ
		setting_mode;	//設定モードか
	int drawpoint_num,	//描画する点の数 (線対称のみ2)
		ntmp;
	double line_rd,		//平行線角度(ラジアン)
		grid_rd,		//グリッド線角度
		ellipse_rd,		//楕円角度
		ellipse_yx,		//楕円扁平率 (y/x)
		symm_rd,		//線対称角度
		dtmp_guide,
		dtmp[3];		//描画中の作業値
	mDoublePoint dpt_press,	//[描画時] ボタン押し時のイメージ位置
		dpt_conc,			//集中線の中心位置
		dpt_circle,
		dpt_ellipse,
		dpt_symm;		//線対称
	mRect rc_guide[2];

	RuleRecord record[DRAW_RULE_RECORD_NUM];  //記録データ

	void (*func_draw_guide)(AppDraw *p,mPixbuf *pixbuf,const mBox *box);
	void (*func_set_guide)(AppDraw *p);
	void (*func_get_point)(AppDraw *p,double *x,double *y,int no);	//点を補正する関数
}AppDrawRule;

/** 入り抜きデータ */

typedef struct
{
	uint8_t selno;  	//0:直線, 1:ベジェ曲線
	uint32_t curval[2],	//現在の値 (上位16bit:入り,下位:抜き)
		regist[DRAW_HEADTAIL_REGIST_NUM];  //登録リスト
}AppDrawHeadTail;

/** 選択範囲用データ */

typedef struct
{
	mRect rcsel;	//選択範囲のおおよそのpx範囲 (tileimg_sel != NULL の時。範囲がなければ空状態の値)
	
	TileImage *tileimg_copy;	//コピーされたイメージ (ファイル出力失敗時)

	uint8_t	is_hide,	//選択範囲の枠を非表示にするか (範囲イメージ移動で、ドラッグ中に枠を表示しない場合)
		copyimg_bits;	//コピーイメージのビット数
}AppDrawSelect;

/** 矩形範囲データ (切り貼り/矩形編集時) */

typedef struct
{
	TileImage *img;	//コピーイメージ (ツール中のみ有効)
	mSize size_img;	//イメージのサイズ
	
	mBox box;		//選択範囲 (w == 0 で範囲なし)
	uint8_t copyimg_bits,	//コピーイメージのビット数
		is_paste_mode,		//切り貼りツール時の貼り付けモード状態か
		flag_resize_cursor;	//現在のリサイズカーソル状態のフラグ
}AppDrawBoxSel;

/** スタンプ用データ */

typedef struct
{
	TileImage *img;	//スタンプ画像 [!] ビット数が変わったときはクリアすること。
	mSize size;		//スタンプ画像のサイズ
	int bits;		//コピー時のビット数
}AppDrawStamp;

/** テキスト描画データ */

typedef struct
{
	DrawFont *font;		//ダイアログ中のフォント
	DrawTextData dt,	//ダイアログでの設定情報
		dt_copy;		//テキストレイヤ時のコピー情報
	uint8_t fpreview,	//プレビュー
		in_dialog,		//ダイアログ中か
		fhave_copy;		//dt_copy にデータがセットされているか
}AppDrawText;

/** 作業用データ */

typedef struct
{
	int ntmp[3],
		optype,				//現在の操作タイプ
		optype_sub,			//操作タイプのサブ情報
		optoolno,			//現在操作中のツール動作
		optool_subno,		//ツールのサブタイプ
		press_btt,			//操作開始時に押されたボタン (MLK_BTT_*)
		release_btt,		//ボタン離し時:指定されたボタンのみ処理する
		brush_regno,		//ブラシ時、登録アイテム番号
		drag_cursor_type;	//(ボタン押し時) グラブ中のみカーソルを変更する場合、APPCURSOR_* を指定。
							// -1 = カーソル変更なし。

	uint32_t press_state,	//操作開始時の装飾キー (MLK_STATE_*。LOCK などは除外されている)
		toollist_toolopt;	//ツールリスト時のツール設定

	uint8_t opflags,		//操作用オプションフラグ
		drawinfo_flags,		//drawOpSub_setDrawInfo() でセットされるフラグ
		is_toollist_toolopt;	//ブラシ以外のツール時、toollist_toolopt の値を使うか

	mPoint pt_canv_last,	//前回のカーソル位置 (int)
		pttmp[4];
	mDoublePoint ptd_tmp[1];
	mRect rcdraw,			//描画された範囲
		rctmp[1];
	mBox boxtmp[1];

	DrawPoint dpt_canv_press,	//操作開始時のカーソル位置 (キャンバス座標)
		dpt_canv_cur,			//現在のカーソル位置
		dpt_canv_last;			//前回のカーソル位置

	void *ptmp;

	uint64_t drawcol,	//描画色 (RGBA。カラー値はビット数による)
		midcol_sec;		//中間色作成の最初の押し時間 (sec)

	RGBcombo midcol_col;	//中間色の最初の色

	TileImage *dstimg;		//描画先イメージ
	LayerItem *layer;		//リンクの先頭レイヤ

	FillPolygon *fillpolygon;

	void (*func_motion)(AppDraw *p,uint32_t state);	//[ポインタ移動時] NULL で何もしない
	mlkbool (*func_release)(AppDraw *p);			//[ボタン離し時] NULL でグラブ解放のみ行う
	void (*func_press_in_grab)(AppDraw *p,uint32_t state);	//グラブ中に左ボタンが押された時
	mlkbool (*func_action)(AppDraw *p,int action);	//右ボタンやキーなどが押された時
}AppDrawWork;


/*---- AppDraw ----*/

typedef struct _AppDraw
{
	int imgw,		//キャンバスの情報
		imgh,
		imgdpi,
		imgbits;	//8or16
	RGBcombo imgbkcol;	//イメージ背景色

	uint8_t fnewcanvas,		//読み込み時用、新規キャンバスが作成されたか
		in_thread_imgcanvas,	//imgcanvas のバッファを操作するスレッド中か
		in_filter_dialog,	//フィルタダイアログ中か (TRUE 時、ptmp = ダイアログのポインタ)
		is_canvview_update,	//キャンバスビューが更新された時 (合成イメージを、作業イメージを使って更新する場合に使う)
		ftonelayer_to_gray,	//トーンレイヤをグレイスケール表示
		fmodify_grad_list,	//グラデーションリストが更新されたか
		fmodify_brushsize;	//ブラシサイズリストが更新されたか

	//---- キャンバス用

	int canvas_zoom,		//表示倍率 (1 = 0.1%)
		canvas_angle;		//表示角度 (1 = 0.01)
	uint8_t canvas_mirror,	//左右反転表示
		canvas_lowquality;	//キャンバス表示を低品質で行うか
	mSize canvas_size;		//キャンバスの領域サイズ
	mPoint canvas_scroll;	//キャンバススクロール位置
	mDoublePoint imgorigin;	//イメージの原点位置

	CanvasViewParam viewparam;	//座標計算用パラメータ

	//----

	AppDrawWork w;
	AppDrawColor col;
	AppDrawTool tool;
	AppDrawRule rule;
	AppDrawHeadTail headtail;
	AppDrawSelect sel;
	AppDrawStamp stamp;
	AppDrawBoxSel boxsel;
	AppDrawText text;
	
	AppDrawToolList *tlist;

	//----

	mList list_material[2],		//素材画像の管理リスト
		list_grad_custom;		//グラデーションカスタムのリスト

	mStr strOptTexturePath;		//オプションテクスチャのファイルパス

	ImageCanvas *imgcanvas;		//全レイヤ合成後のイメージ
	ImageMaterial *imgmat_opttex;	//オプションテクスチャの現在イメージ

	TileImage *tileimg_sel,	//選択範囲用 (1bit)
		*tileimg_tmp_save,	//描画用作業イメージ (描画部分の元イメージ保存用)
		*tileimg_tmp_brush,	//ブラシ描画時の描画濃度バッファ
		*tileimg_tmp,		//操作中のみの作業用イメージ
		*tileimg_filterprev;	//フィルタのキャンバスプレビュー時の挿入イメージ (NULL でなし)

	LayerList *layerlist;	//レイヤリスト
	PointBuf *pointbuf;		//自由線描画用の位置データ

	LayerItem *curlayer,	//カレントレイヤ
		*masklayer,			//マスクレイヤ
		*ttip_layer;		//最後にツールチップで表示したレイヤ
}AppDraw;

#endif
