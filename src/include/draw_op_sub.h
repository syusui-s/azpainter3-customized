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

/**************************************
 * AppDraw : 操作関連のサブ処理関数
 **************************************/

typedef struct _TileImageDrawGradInfo TileImageDrawGradInfo;
typedef struct _DrawPoint DrawPoint;

enum
{
	CANDRAWLAYER_F_ENABLE_TEXT = 1<<0,	//テキストレイヤを有効
	CANDRAWLAYER_F_ENABLE_READ = 1<<1,	//ロックと非表示は有効 (イメージ参照のみの場合)
	CANDRAWLAYER_F_NO_HIDE = 1<<2,		//非表示は判定しない
	CANDRAWLAYER_F_NO_PASTE = 1<<3		//切り貼りツールは判定しない
};

enum
{
	CANDRAWLAYER_OK,
	CANDRAWLAYER_FOLDER,	//フォルダ
	CANDRAWLAYER_TEXT,		//テキストレイヤ
	CANDRAWLAYER_LOCK,		//ロック
	CANDRAWLAYER_HIDE,		//レイヤが非表示
	CANDRAWLAYER_PASTE		//切り貼りツールの貼り付けモード中
};

enum
{
	DRAWOPSUB_UPDATE_DIRECT,	//直接更新
	DRAWOPSUB_UPDATE_ADD,		//範囲追加
	DRAWOPSUB_UPDATE_TIMER		//タイマー
};


/*-----------*/

void drawOpSub_setOpInfo(AppDraw *p,int optype,
	void (*motion)(AppDraw *,uint32_t),mlkbool (*release)(AppDraw *),int sub);

int drawOpSub_canDrawLayer(AppDraw *p,uint32_t flags);
int drawOpSub_canDrawLayer_mes(AppDraw *p,uint32_t flags);
mlkbool drawOpSub_isCurLayer_folder(void);
mlkbool drawOpSub_isCurLayer_text(void);
mlkbool drawOpSub_isRunTextTool(void);

/* 作業用イメージなど */

mlkbool drawOpSub_createTmpImage_canvas(AppDraw *p);
mlkbool drawOpSub_createTmpImage_curlayer_imgsize(AppDraw *p);
mlkbool drawOpSub_createTmpImage_curlayer_rect(AppDraw *p,const mRect *rc);

void drawOpSub_freeTmpImage(AppDraw *p);
void drawOpSub_freeFillPolygon(AppDraw *p);

TileImage *drawOpSub_createSelectImage_forStamp(AppDraw *p,mRect *rcdst);

/* 描画 */

void drawOpSub_beginDraw(AppDraw *p);
void drawOpSub_endDraw(AppDraw *p,mRect *rcdraw);

void drawOpSub_beginDraw_single(AppDraw *p);
void drawOpSub_endDraw_single(AppDraw *p);

void drawOpSub_addrect_and_update(AppDraw *p,int type);
void drawOpSub_finishDraw_workrect(AppDraw *p);
void drawOpSub_finishDraw_single(AppDraw *p);
void drawOpSub_update_rcdraw(AppDraw *p);

void drawOpSub_setDrawInfo(AppDraw *p,int toolno,int param);

void drawOpSub_setDrawInfo_pixelcol(int type);
void drawOpSub_setDrawInfo_clearMasks(void);
void drawOpSub_setDrawInfo_select(void);
void drawOpSub_setDrawInfo_select_paste(void);
void drawOpSub_setDrawInfo_select_cut(void);
void drawOpSub_setDrawInfo_fillerase(mlkbool erase);
void drawOpSub_setDrawInfo_overwrite(void);
void drawOpSub_setDrawInfo_textlayer(AppDraw *p,mlkbool erase);
void drawOpSub_setDrawInfo_filter(void);

void drawOpSub_setDrawGradationInfo(AppDraw *p,TileImageDrawGradInfo *info);

mPixbuf *drawOpSub_beginCanvasDraw(void);
void drawOpSub_endCanvasDraw(mPixbuf *pixbuf,mBox *box);

/* ポイント取得など */

void drawOpSub_getCanvasPoint_int(AppDraw *p,mPoint *pt);

void drawOpSub_getImagePoint_double(AppDraw *p,mDoublePoint *pt);
void drawOpSub_getImagePoint_int(AppDraw *p,mPoint *pt);
void drawOpSub_getImagePoint_dotpen(AppDraw *p,mPoint *pt);

void drawOpSub_getDrawPoint(AppDraw *p,DrawPoint *dst);

/* 操作関連 */

void drawOpSub_copyTmpPoint_0toN(AppDraw *p,int max);
void drawOpSub_startMoveImage(AppDraw *p,TileImage *img);
mlkbool drawOpSub_isPress_inSelect(AppDraw *p);

void drawOpSub_getDrawLinePoints(AppDraw *p,mDoublePoint *pt1,mDoublePoint *pt2);
void drawOpSub_getDrawRectBox_angle0(AppDraw *p,mBox *box);
void drawOpSub_getDrawRectPoints(AppDraw *p,mDoublePoint *pt);
void drawOpSub_getDrawEllipseParam(AppDraw *p,mDoublePoint *pt_ct,mDoublePoint *pt_radius,mlkbool fdot);

