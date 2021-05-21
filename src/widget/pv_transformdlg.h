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
 * 変形ダイアログ:内部定義
 *****************************************/

/** TransformView */

typedef struct
{
	mWidget wg;

	TileImage *imgcopy,	//変形元イメージ
		*imgxor;		//キャンバスのXOR描画用イメージ

	mCursor cursor_restore; //ドラッグ中のカーソル変更時、元のカーソル

	double angle,		//[affine] 回転角度 (ラジアン)
		scalex,			//[affine] 拡大縮小倍率
		scaley,
		scalex_div,
		scaley_div,
		dcos,			//[affine] 回転角度の cos,sin (逆回転)
		dsin,
		centerx,		//処理中心位置 (ソース画像における座標)
		centery,
		canv_origx,		//キャンバス表示におけるイメージの原点位置
		canv_origy,
		dtmp[4],		//ドラッグ時の作業用
		homog_param[9];	//[射影変換] 逆変換のパラメータ

	mPoint pt_mov,			//[共通] 平行移動
		pt_corner_canv[4],	//[共通] 4隅の位置 (キャンバス座標)
		pt_last_canv;		//ドラッグ時、前回のキャンバス位置

	//[射影変換] 4隅の位置 (イメージ座標)
	//src = 変形前の位置。dst = 変形後の位置。
	mDoublePoint dpt_corner_src[4],
		dpt_corner_dst[4];

	mBox boxsrc;		//編集対象の矩形範囲

	int type,			//編集のタイプ
		canv_zoom,		//キャンバス表示倍率 (1=0.1%)
		canv_scrx,		//キャンバススクロール位置
		canv_scry,
		fdrag,
		dragbtt,		//ドラッグ中のボタン
		cur_area;		//現在のカーソル下のエリア
	mlkbool keep_aspect,	//縦横比維持
		low_quality;		//ビューを低品質表示 (バーのドラッグ中はONにする)

	CanvasDrawInfo canvinfo;	//キャンバス描画情報
	CanvasViewParam viewparam;	//キャンバス表示用のパラメータ
}TransformView;


//---------------------

//変形タイプ
enum
{
	TRANSFORM_TYPE_NORMAL,		//通常
	TRANSFORM_TYPE_TRAPEZOID	//遠近
};

//ポインタ下の変形後のエリア
enum
{
	TRANSFORM_DSTAREA_NONE,			//操作対象外
	TRANSFORM_DSTAREA_LEFT_TOP,		//4隅の点
	TRANSFORM_DSTAREA_RIGHT_TOP,
	TRANSFORM_DSTAREA_RIGHT_BOTTOM,
	TRANSFORM_DSTAREA_LEFT_BOTTOM,
	TRANSFORM_DSTAREA_IN_IMAGE,		//イメージ内 (位置の移動)
	TRANSFORM_DSTAREA_OUT_IMAGE		//イメージ外 (affine 時は回転)
};

//TransformView_update() 時のフラグ
enum
{
	TRANSFORM_UPDATE_F_CANVAS = 1,		//キャンバスの状態更新時
	TRANSFORM_UPDATE_F_TRANSPARAM = 2	//変形パラメータ変更時
};

//通知タイプ
enum
{
	TRANSFORM_NOTIFY_CHANGE_AFFINE_PARAM = 1,	//[affine] 操作によりパラメータ変更
	TRANSFORM_NOTIFY_CHANGE_CANVAS_ZOOM			//表示倍率変更
};

#define TRANSFORM_ZOOM_MAX  10000	//表示倍率最大

//-----------------


TransformView *TransformView_create(mWidget *parent,int wid);

void TransformView_init(TransformView *p);
void TransformView_setCanvasZoom(TransformView *p,int zoom);
void TransformView_resetParam(TransformView *p);
void TransformView_update(TransformView *p,uint8_t flags);
void TransformView_startCanvasZoomDrag(TransformView *p);

/* sub */

void TransformView_image_to_canv_pt(TransformView *p,mPoint *pt,double x,double y);
void TransformView_canv_to_image_dpt(TransformView *p,mDoublePoint *pt,double x,double y);

void TransformView_affine_source_to_canv_pt(TransformView *p,mPoint *pt,double x,double y);
void TransformView_affine_canv_to_source_dpt(TransformView *p,mDoublePoint *ptdst,int x,int y);
void TransformView_affine_canv_to_source_pt(TransformView *p,mPoint *ptdst,int x,int y);
double TransformView_affine_get_canvas_angle(TransformView *p,int x,int y);
void TransformView_affine_canv_to_source_for_drag(TransformView *p,mDoublePoint *ptdst,int x,int y);

mlkbool TransformView_homog_getDstCanvasBox(TransformView *p,mBox *box);
void TransformView_homog_getDstImageRect(TransformView *p,mRect *rcdst);

void TransformView_onMotion_nodrag(TransformView *p,int x,int y);
void TransformView_onPress_affineZoom(TransformView *p,int cx,int cy);
mlkbool TransformView_onMotion_homog(TransformView *p,int x,int y,uint32_t state);

void getHomographyRevParam(double *dst,const mDoublePoint *s,const mDoublePoint *d);

