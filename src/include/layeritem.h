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
 * レイヤアイテム
 ************************************/

#ifndef AZPT_LAYERITEM_H
#define AZPT_LAYERITEM_H

typedef struct _LayerItem LayerItem;
typedef struct _TileImage TileImage;
typedef struct _ImageMaterial ImageMaterial;
typedef struct _mPopupProgress mPopupProgress;
typedef struct _DrawTextData DrawTextData;


/** レイヤアイテム */

struct _LayerItem
{
	mTreeItem i;

	mList list_text;	//テキストレイヤ:テキストリスト

	TileImage *img;			//レイヤイメージ
	ImageMaterial *img_texture;	//テクスチャイメージ
	LayerItem *link;		//作業用リンク
	
	char *name,     	//レイヤ名 (NULL で空文字列)
		*texture_path;	//レイヤテクスチャパス (NULL でなし)
	uint32_t flags, 	//フラグ
		col;			//レイヤ色
	int16_t tone_lines, //トーン:線数 (1=0.1)
		tone_angle;		//トーン:角度 (-360 〜 +360)
	uint8_t	type,		//レイヤタイプ
		opacity,   		//レイヤの不透明度 [0-128]
		blendmode,		//合成モード
		alphamask,		//アルファマスクタイプ
		tone_density;	//トーン:固定濃度 [0-100, 0で固定濃度使わない]
};

/** テキストアイテム */

typedef struct _LayerTextItem
{
	mListItem i;
	int32_t x,y,	//描画位置
		datsize,	//データサイズ
		tmp;		//作業用
	mRect rcdraw;	//描画範囲 [!] 範囲がない場合、(0,0)-(-1,-1)
	uint8_t dat[1];
}LayerTextItem;

/* レイヤタイプ */

enum
{
	LAYERTYPE_RGBA,
	LAYERTYPE_GRAY,
	LAYERTYPE_ALPHA,
	LAYERTYPE_ALPHA1BIT
};

/* フラグ */

enum
{
	LAYERITEM_F_VISIBLE = 1<<0,			//表示
	LAYERITEM_F_FOLDER_OPENED = 1<<1,	//フォルダ開いている
	LAYERITEM_F_LOCK     = 1<<2,		//描画ロック
	LAYERITEM_F_FILLREF  = 1<<3,		//塗りつぶし参照
	LAYERITEM_F_MASK_UNDER = 1<<4,		//下レイヤをマスクに
	LAYERITEM_F_CHECKED  = 1<<5,		//チェック
	LAYERITEM_F_TONE = 1<<6,			//トーン化
	LAYERITEM_F_TEXT = 1<<7,			//テキストレイヤ
	LAYERITEM_F_TONE_WHITE = 1<<8		//トーンの背景を白に
};

/* マクロ */

#define LAYERITEM_OPACITY_MAX    128

#define LAYERITEM(p)               ((LayerItem *)(p))
#define LAYERITEM_IS_VISIBLE(p)    ((p)->flags & LAYERITEM_F_VISIBLE)
#define LAYERITEM_IS_FOLDER(p)     (!(p)->img)
#define LAYERITEM_IS_IMAGE(p)      ((p)->img != 0)
#define LAYERITEM_IS_LOCK(p)       ((p)->flags & LAYERITEM_F_LOCK)
#define LAYERITEM_IS_FOLDER_OPENED(p)  ((p)->flags & LAYERITEM_F_FOLDER_OPENED)
#define LAYERITEM_IS_FILLREF(p)    ((p)->flags & LAYERITEM_F_FILLREF)
#define LAYERITEM_IS_MASK_UNDER(p) ((p)->flags & LAYERITEM_F_MASK_UNDER)
#define LAYERITEM_IS_CHECKED(p)    ((p)->flags & LAYERITEM_F_CHECKED)
#define LAYERITEM_IS_TONE(p)       ((p)->flags & LAYERITEM_F_TONE)
#define LAYERITEM_IS_TEXT(p)       ((p)->flags & LAYERITEM_F_TEXT)
#define LAYERITEM_IS_TONE_WHITE(p) ((p)->flags & LAYERITEM_F_TONE_WHITE)


/*---- function ----*/

void LayerItem_setVisible(LayerItem *p);
void LayerItem_setOpened_parent(LayerItem *p);

void LayerItem_setLink(LayerItem *p,LayerItem **pptop,LayerItem **pplast);

void LayerItem_setName_layerno(LayerItem *p,int no);
mlkbool LayerItem_setTexture(LayerItem *p,const char *path);
void LayerItem_loadTextureImage(LayerItem *p);
void LayerItem_setLayerColor(LayerItem *p,uint32_t col);
void LayerItem_replaceImage(LayerItem *p,TileImage *img,int type);
void LayerItem_setImage(LayerItem *p,TileImage *img);

void LayerItem_copyInfo(LayerItem *dst,LayerItem *src);
mlkbool LayerItem_isHave_editImageFull(LayerItem *item);
void LayerItem_editImage_full(LayerItem *item,int type,mRect *rcupdate);
void LayerItem_moveImage(LayerItem *p,int relx,int rely);

/* get */

mlkbool LayerItem_isNeedColor(LayerItem *p);
mlkbool LayerItem_isVisible_onlist(LayerItem *p);
mlkbool LayerItem_isVisible_real(LayerItem *p);
mlkbool LayerItem_isLock_real(LayerItem *p);
mlkbool LayerItem_isInParent(LayerItem *p,LayerItem *parent);
mlkbool LayerItem_isFolderOrText(LayerItem *p);
mlkbool LayerItem_isEnableUnderDrop(LayerItem *p);
mlkbool LayerItem_isEnableUnderCombine(LayerItem *p);

int LayerItem_getTreeDepth(LayerItem *p);
int LayerItem_getOpacity_real(LayerItem *p);
int LayerItem_getLinkNum(LayerItem *p);

mlkbool LayerItem_getVisibleImageRect(LayerItem *p,mRect *rc);
mlkbool LayerItem_getVisibleImageRect_link(LayerItem *p,mRect *rc);

/* item */

LayerItem *LayerItem_getNext(LayerItem *p);
LayerItem *LayerItem_getNextPass(LayerItem *p);
LayerItem *LayerItem_getNextRoot(LayerItem *p,LayerItem *root);
LayerItem *LayerItem_getPrev(LayerItem *p);
LayerItem *LayerItem_getPrevVisibleImage_incSelf(LayerItem *p);
LayerItem *LayerItem_getPrevVisibleImage(LayerItem *p);
LayerItem *LayerItem_getPrevOpened(LayerItem *p);
LayerItem *LayerItem_getNextOpened(LayerItem *p);
LayerItem *LayerItem_getNextVisibleImage(LayerItem *p,LayerItem *root);

/* text */

LayerTextItem *LayerItem_getTextItem_atIndex(LayerItem *p,int index);

LayerTextItem *LayerItem_addText_read(LayerItem *p,int x,int y,const mRect *rc,int size);
LayerTextItem *LayerItem_addText(LayerItem *p,DrawTextData *dt,mPoint *ptpos,mRect *rcdraw);
LayerTextItem *LayerItem_replaceText(LayerItem *p,LayerTextItem *item,DrawTextData *dt,mPoint *ptpos,mRect *rcdraw);
void LayerItem_deleteText(LayerItem *p,LayerTextItem *item);
void LayerItem_moveTextPos_all(LayerItem *p,int relx,int rely);
void LayerItem_appendText_dup(LayerItem *dst,LayerItem *src);
void LayerItem_clearText(LayerItem *p);

LayerTextItem *LayerItem_addText_undo(LayerItem *p,int x,int y,int datsize,uint8_t *buf);
LayerTextItem *LayerItem_replaceText_undo(LayerItem *p,LayerTextItem *item,int x,int y,int datsize,uint8_t *buf);

LayerTextItem *LayerItem_getTextItem_atPoint(LayerItem *p,int x,int y);
void LayerItem_getTextRedrawInfo(LayerItem *p,mRect *rcdst,const mRect *rcsrc);

void LayerTextItem_getDrawData(LayerTextItem *p,DrawTextData *dt);
void LayerTextItem_getText_limit(LayerTextItem *p,mStr *str,int max);

#endif
