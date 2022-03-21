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
 * レイヤアイテム
 *****************************************/

#include <stdio.h>
#include <string.h>

#include "mlk.h"
#include "mlk_tree.h"
#include "mlk_list.h"
#include "mlk_rectbox.h"
#include "mlk_str.h"
#include "mlk_util.h"

#include "def_draw.h"
#include "def_tileimage.h"

#include "layeritem.h"
#include "tileimage.h"
#include "materiallist.h"


//-----------------------

#define _ITEM_PARENT(p)    ((LayerItem *)((p)->i.parent))
#define _ITEM_LAST(p)      ((LayerItem *)((p)->i.last))
#define _NEXTITEM_ROOT(p,root)  ((LayerItem *)mTreeItemGetNext_root((mTreeItem *)p, (mTreeItem *)root))

//-----------------------


//==============================
// セット
//==============================


/** 指定レイヤが表示されるようにする
 *
 * - 親フォルダをすべて表示させる。
 * - item がフォルダの場合は、下位もすべて表示させる。 */

void LayerItem_setVisible(LayerItem *item)
{
	LayerItem *pi;

	//item と親

	for(pi = item; pi; pi = _ITEM_PARENT(pi))
		pi->flags |= LAYERITEM_F_VISIBLE;

	//フォルダ下

	for(pi = item; pi; pi = _NEXTITEM_ROOT(pi, item))
		pi->flags |= LAYERITEM_F_VISIBLE;
}

/** 親のフォルダをすべて開き、p がリスト上に表示されるようにする */

void LayerItem_setOpened_parent(LayerItem *p)
{
	for(p = _ITEM_PARENT(p); p; p = _ITEM_PARENT(p))
		p->flags |= LAYERITEM_F_FOLDER_OPENED;
}


/** アイテムリンクをセット
 *
 * pptop:  先頭アイテム (NULL で p がセットされる)
 * pplast: 最後のリンクアイテム */

void LayerItem_setLink(LayerItem *p,LayerItem **pptop,LayerItem **pplast)
{
	if(*pptop)
		(*pplast)->link = p;
	else
		*pptop = p;

	p->link = NULL;
	*pplast = p;
}


//==============================
// 
//==============================


/** 名前を「layer＋番号」にセット */

void LayerItem_setName_layerno(LayerItem *p,int no)
{
	char m[32];

	snprintf(m, 32, "layer%d", no);

	mStrdup_free(&p->name, m);
}

/** テクスチャをセット (変更)
 *
 * path: NULL または空文字列で、なし
 * return: 画像が変更されたか */

mlkbool LayerItem_setTexture(LayerItem *p,const char *path)
{
	//空文字列 -> NULL

	if(path && !(*path)) path = NULL;

	//なしの状態で変わらない時

	if(!path && !p->texture_path) return FALSE;

	//現在のイメージを解放

	MaterialList_releaseImage(APPDRAW->list_material, MATERIALLIST_TYPE_TEXTURE, p->img_texture);

	//セット

	mStrdup_free(&p->texture_path, path);

	LayerItem_loadTextureImage(p);

	return TRUE;
}

/** 現在のテクスチャパスから、画像を読み込み */

void LayerItem_loadTextureImage(LayerItem *p)
{
	p->img_texture = MaterialList_getImage(APPDRAW->list_material,
		MATERIALLIST_TYPE_LAYER_TEXTURE, p->texture_path, TRUE);
}

/** レイヤ色をセット */

void LayerItem_setLayerColor(LayerItem *p,uint32_t col)
{
	p->col = col;

	TileImage_setColor(p->img, col);
}

/** イメージを置き換え (レイヤ色はコピーしない)
 *
 * type: レイヤタイプの変更時はレイヤタイプ。負の値で変更なし。 */

void LayerItem_replaceImage(LayerItem *p,TileImage *img,int type)
{
	TileImage_free(p->img);

	p->img = img;

	if(type >= 0) p->type = type;
}

/** イメージをセット
 *
 * 線の色とレイヤタイプは、イメージから取得する。 */

void LayerItem_setImage(LayerItem *p,TileImage *img)
{
	p->img = img;
	p->type = img->type;
	p->col = RGBcombo_to_32bit(&img->col);
}

/** 情報をコピー */

void LayerItem_copyInfo(LayerItem *dst,LayerItem *src)
{
	mStrdup_free(&dst->name, src->name);
	mStrdup_free(&dst->texture_path, src->texture_path);

	dst->opacity = src->opacity;
	dst->blendmode = src->blendmode;
	dst->alphamask = src->alphamask;
	dst->flags = src->flags;

	dst->tone_lines = src->tone_lines;
	dst->tone_angle = src->tone_angle;
	dst->tone_density = src->tone_density;

	LayerItem_setLayerColor(dst, src->col);
}

/** イメージ全体の編集時、対象となるレイヤがあるか */

mlkbool LayerItem_isHave_editImageFull(LayerItem *item)
{
	LayerItem *pi;

	for(pi = item; pi; pi = _NEXTITEM_ROOT(pi, item))
	{
		if(LAYERITEM_IS_IMAGE(pi) && !LAYERITEM_IS_TEXT(pi))
			return TRUE;
	}

	return FALSE;
}

/** イメージ全体の編集 (反転/回転)
 *
 * - フォルダの場合は下位すべて対象。
 * - テキストレイヤ、空イメージは除く。
 *
 * type:  0=左右反転 1=上下反転 2=左90度回転 3=右90度回転
 * rcupdate: 更新範囲が入る */

void LayerItem_editImage_full(LayerItem *item,int type,mRect *rcupdate)
{
	LayerItem *pi;
	mRect rc,rc2;
	void (*func[4])(TileImage *) = {
		TileImage_flipHorz_full, TileImage_flipVert_full,
		TileImage_rotateLeft_full, TileImage_rotateRight_full
	};

	mRectEmpty(&rc);

	for(pi = item; pi; pi = _NEXTITEM_ROOT(pi, item))
	{
		if(pi->img && !LAYERITEM_IS_TEXT(pi))
		{
			//現在のイメージ範囲
			//(空イメージの場合は処理しない)
			
			if(!TileImage_getHaveImageRect_pixel(pi->img, &rc2, NULL))
				continue;

			mRectUnion(&rc, &rc2);

			//処理

			(func[type])(pi->img);

			//処理後のイメージ範囲

			if(TileImage_getHaveImageRect_pixel(pi->img, &rc2, NULL))
				mRectUnion(&rc, &rc2);
		}
	}

	*rcupdate = rc;
}

/** イメージの位置を相対移動 */

void LayerItem_moveImage(LayerItem *p,int relx,int rely)
{
	TileImage_moveOffset_rel(p->img, relx, rely);

	//テキスト位置

	LayerItem_moveTextPos_all(p, relx, rely);
}


//==============================
// 情報取得
//==============================


/** レイヤ色が必要なカラータイプか */

mlkbool LayerItem_isNeedColor(LayerItem *p)
{
	return (p->img &&
		(p->type == LAYERTYPE_ALPHA
			|| p->type == LAYERTYPE_ALPHA1BIT
			|| (p->type == LAYERTYPE_GRAY && LAYERITEM_IS_TONE(p)) ));
}

/** p がレイヤ一覧上に表示されている状態か
 *
 * 親がすべて展開されているか */

mlkbool LayerItem_isVisible_onlist(LayerItem *p)
{
	for(p = _ITEM_PARENT(p); p; p = _ITEM_PARENT(p))
	{
		if(!LAYERITEM_IS_FOLDER_OPENED(p))
			return FALSE;
	}

	return TRUE;
}

/** 実際に表示されている状態か (親の状態を適用) */

mlkbool LayerItem_isVisible_real(LayerItem *p)
{
	for(; p; p = _ITEM_PARENT(p))
	{
		if(!LAYERITEM_IS_VISIBLE(p)) return FALSE;
	}
	
	return TRUE;
}

/** ロックされているか (親の状態も適用) */

mlkbool LayerItem_isLock_real(LayerItem *p)
{
	for(; p; p = _ITEM_PARENT(p))
	{
		if(LAYERITEM_IS_LOCK(p)) return TRUE;
	}

	return FALSE;
}

/** p が parent 以下の階層に存在するか */

mlkbool LayerItem_isInParent(LayerItem *p,LayerItem *parent)
{
	return mTreeItemIsChild(MTREEITEM(p), MTREEITEM(parent));
}

/** フォルダまたはテキストレイヤかどうか */

mlkbool LayerItem_isFolderOrText(LayerItem *p)
{
	return (LAYERITEM_IS_FOLDER(p) || LAYERITEM_IS_TEXT(p));
}

/** "下レイヤに移す" が実行可能か */

mlkbool LayerItem_isEnableUnderDrop(LayerItem *p)
{
	LayerItem *dst = (LayerItem *)p->i.next;

	if(!dst) return FALSE;

	return (!LayerItem_isFolderOrText(p)
		&& !LayerItem_isFolderOrText(dst));
}

/** "下レイヤに結合" が実行可能か */

mlkbool LayerItem_isEnableUnderCombine(LayerItem *p)
{
	LayerItem *dst = (LayerItem *)p->i.next;

	if(!dst
		|| LAYERITEM_IS_FOLDER(p) || LAYERITEM_IS_FOLDER(dst))
		return FALSE;

	if(LAYERITEM_IS_TEXT(p) && LAYERITEM_IS_TEXT(dst))
		//テキスト同士
		return TRUE;
	else if(LAYERITEM_IS_TEXT(dst))
		//上が通常、下がテキストの場合は不可
		return FALSE;

	return TRUE;
}

/** ツリーの深さ取得
 *
 * return: 親がなければ 0 */

int LayerItem_getTreeDepth(LayerItem *p)
{
	mTreeItem *pi;
	int depth = 0;

	for(pi = p->i.parent; pi; pi = pi->parent, depth++);

	return depth;
}

/** 合成時の不透明度取得 (親の状態も適用) */

int LayerItem_getOpacity_real(LayerItem *p)
{
	int n = p->opacity;

	for(p = _ITEM_PARENT(p); p; p = _ITEM_PARENT(p))
	{
		if(p->opacity != 128)
			n = (n * p->opacity + 64) >> 7;
	}

	return n;
}

/** p をリンクの先頭として、リンクされたレイヤ数を取得 */

int LayerItem_getLinkNum(LayerItem *p)
{
	int num = 0;

	for(; p; p = p->link)
		num++;

	return num;
}

/** キャンバスに表示されるイメージの px 範囲取得 (イメージ範囲外の部分も含む)
 *
 * - 通常レイヤの場合、親の表示状態は適用されない。
 * - フォルダの場合、フォルダ下の全てのイメージの範囲 (表示状態適用される)。
 *
 * return: FALSE で表示範囲がない */

mlkbool LayerItem_getVisibleImageRect(LayerItem *item,mRect *rc)
{
	LayerItem *pi;
	mRect rc1,rc2;

	mRectEmpty(&rc1);

	if(LAYERITEM_IS_IMAGE(item))
	{
		//通常レイヤ

		if(LAYERITEM_IS_VISIBLE(item))
			TileImage_getHaveImageRect_pixel(item->img, &rc1, NULL);
	}
	else
	{
		//フォルダ

		pi = LayerItem_getNextVisibleImage(item, item);

		for(; pi; pi = LayerItem_getNextVisibleImage(pi, item))
		{
			if(TileImage_getHaveImageRect_pixel(pi->img, &rc2, NULL))
				mRectUnion(&rc1, &rc2);
		}
	}

	*rc = rc1;

	return !mRectIsEmpty(&rc1);
}

/** p を先頭として、リンクされているレイヤのすべてのイメージの表示範囲を取得
 *
 * return: 範囲があるか */

mlkbool LayerItem_getVisibleImageRect_link(LayerItem *p,mRect *rc)
{
	mRect rc1,rc2;

	mRectEmpty(&rc1);

	for(; p; p = p->link)
	{
		if(LayerItem_getVisibleImageRect(p, &rc2))
			mRectUnion(&rc1, &rc2);
	}

	*rc = rc1;

	return !mRectIsEmpty(&rc1);
}


//==============================
// アイテム取得
//==============================


/** 次のツリーアイテム */

LayerItem *LayerItem_getNext(LayerItem *p)
{
	return (LayerItem *)mTreeItemGetNext(MTREEITEM(p));
}

/** 子を飛ばした次のツリーアイテム */

LayerItem *LayerItem_getNextPass(LayerItem *p)
{
	return (LayerItem *)mTreeItemGetNextPass(MTREEITEM(p));
}

/** root 以下の次のツリーアイテム */

LayerItem *LayerItem_getNextRoot(LayerItem *p,LayerItem *root)
{
	return (LayerItem *)mTreeItemGetNext_root(MTREEITEM(p), MTREEITEM(root));
}

/** 一つ前のツリーアイテム */

LayerItem *LayerItem_getPrev(LayerItem *p)
{
	return (LayerItem *)mTreeItemGetPrev(MTREEITEM(p));
}

/** 自身も含む、一つ前の表示イメージアイテム取得 */

LayerItem *LayerItem_getPrevVisibleImage_incSelf(LayerItem *p)
{
	while(p)
	{
		if(!LAYERITEM_IS_VISIBLE(p))
			//非表示 -> 前のアイテムへ
			p = (LayerItem *)mTreeItemGetPrevPass(MTREEITEM(p));
		else
		{
			//表示

			if(LAYERITEM_IS_IMAGE(p))  //通常レイヤなら終了
				break;
			else if(p->i.last)         //フォルダで子がある場合、最後の子
				p = _ITEM_LAST(p);
			else                       //フォルダで子がなければ、前のアイテム
				p = (LayerItem *)mTreeItemGetPrevPass(MTREEITEM(p));
		}
	}

	return p;
}

/** 一つ前の表示イメージアイテム取得 */

LayerItem *LayerItem_getPrevVisibleImage(LayerItem *p)
{
	p = (LayerItem *)mTreeItemGetPrevPass(MTREEITEM(p));

	if(p)
		p = LayerItem_getPrevVisibleImage_incSelf(p);

	return p;
}

/** 一つ前の展開されたアイテム取得 */

LayerItem *LayerItem_getPrevOpened(LayerItem *p)
{
	if(!p->i.prev)
		//前がなければ親へ
		return (LayerItem *)p->i.parent;
	else
	{
		//開いているフォルダの場合は、最後のアイテム
	
		p = LAYERITEM(p->i.prev);

		for(; p && p->i.first && LAYERITEM_IS_FOLDER_OPENED(p); p = LAYERITEM(p->i.last));

		return p;
	}
}

/** 次の展開されたアイテム取得 (レイヤ一覧表示用) */

LayerItem *LayerItem_getNextOpened(LayerItem *p)
{
	if(p->i.first && LAYERITEM_IS_FOLDER_OPENED(p))
		//子があり開いている場合は、子へ
		return (LayerItem *)p->i.first;
	else
		//次へ
		return (LayerItem *)mTreeItemGetNextPass(MTREEITEM(p));
}

/** 次の表示イメージアイテム取得
 *
 * root: この下位のみ取得 */

LayerItem *LayerItem_getNextVisibleImage(LayerItem *p,LayerItem *root)
{
	LayerItem *top = p;

	while(p)
	{
		if(LAYERITEM_IS_FOLDER(p))
		{
			//フォルダ

			if(p->i.first && LAYERITEM_IS_VISIBLE(p))
				p = (LayerItem *)p->i.first;
			else
				//非表示 or 子がない
				p = (LayerItem *)mTreeItemGetNextPass_root(MTREEITEM(p), MTREEITEM(root));
		}
		else
		{
			//通常レイヤ

			if(p != top && LAYERITEM_IS_VISIBLE(p))
				break;
			else
				p = (LayerItem *)mTreeItemGetNextPass_root(MTREEITEM(p), MTREEITEM(root));
		}
	}

	return p;
}


//=================================
// テキストデータ
//=================================


/* LayerTextItem::tmp = 0 にする */

static void _textitem_clear_tmp(LayerItem *p)
{
	LayerTextItem *pi;

	MLK_LIST_FOR(p->list_text, pi, LayerTextItem)
	{
		pi->tmp = 0;
	}
}

/* 文字列のセット */

static uint8_t *_set_text_string(uint8_t *pd,mStr *str)
{
	mSetBufBE32(pd, str->len);
	pd += 4;

	if(str->len)
		memcpy(pd, str->buf, str->len);

	return pd + str->len;
}

/* 文字列の取得 */

static uint8_t *_get_text_string(uint8_t *ps,mStr *str)
{
	int len;

	len = mGetBufBE32(ps);
	ps += 4;

	mStrSetText_len(str, (char *)ps, len);

	return ps + len;
}

/* LayerTextItem から複製を追加 */

static mlkbool _add_text_from_item(LayerItem *p,LayerTextItem *src)
{
	LayerTextItem *pi;

	pi = (LayerTextItem *)mListAppendNew(&p->list_text, sizeof(LayerTextItem) - 1 + src->datsize);
	if(!pi) return FALSE;

	pi->x = src->x;
	pi->y = src->y;
	pi->datsize = src->datsize;
	pi->rcdraw = src->rcdraw;

	memcpy(pi->dat, src->dat, src->datsize);

	return TRUE;
}

/** インデックスからテキストアイテムを取得 */

LayerTextItem *LayerItem_getTextItem_atIndex(LayerItem *p,int index)
{
	return (LayerTextItem *)mListGetItemAtIndex(&p->list_text, index);
}

/** テキストデータ追加 (読み込み時) */

LayerTextItem *LayerItem_addText_read(LayerItem *p,int x,int y,const mRect *rc,int size)
{
	LayerTextItem *pi;

	pi = (LayerTextItem *)mListAppendNew(&p->list_text, sizeof(LayerTextItem) - 1 + size);
	if(!pi) return NULL;

	pi->x = x;
	pi->y = y;
	pi->rcdraw = *rc;
	pi->datsize = size;

	return pi;
}

/** テキストデータを追加
 *
 * ptpos: 描画位置
 * rcdraw: 描画範囲 */

LayerTextItem *LayerItem_addText(LayerItem *p,DrawTextData *dt,mPoint *ptpos,mRect *rcdraw)
{
	LayerTextItem *pi;
	uint8_t *pd;
	int size;
	uint32_t flags2;

	size = 4 * 13 + dt->str_text.len + dt->str_font.len + dt->str_style.len;

	pi = (LayerTextItem *)mListAppendNew(&p->list_text, sizeof(LayerTextItem) - 1 + size);
	if(!pi) return NULL;

	pi->x = ptpos->x;
	pi->y = ptpos->y;
	pi->rcdraw = *rcdraw;
	pi->datsize = size;

	//flags2

	flags2 = dt->fontsel
		| (dt->unit_fontsize << 3)
		| (dt->unit_rubysize << 6)
		| (dt->unit_char_space << 9)
		| (dt->unit_line_space << 12)
		| (dt->unit_ruby_pos << 15)
		| (dt->hinting << 18);

	//

	pd = pi->dat;

	pd += mSetBuf_format(pd, ">iiiiiiiiii",
		dt->font_param, dt->fontsize, dt->rubysize, dt->dpi,
		dt->char_space, dt->line_space, dt->ruby_pos, dt->angle,
		dt->flags, flags2);

	//str

	pd = _set_text_string(pd, &dt->str_text);
	pd = _set_text_string(pd, &dt->str_font);
	pd = _set_text_string(pd, &dt->str_style);
	
	return pi;
}

/** テキストアイテムを置き換え */

LayerTextItem *LayerItem_replaceText(LayerItem *p,LayerTextItem *item,
	DrawTextData *dt,mPoint *ptpos,mRect *rcdraw)
{
	mListItem *ins;

	ins = item->i.next;

	//削除

	mListDelete(&p->list_text, MLISTITEM(item));

	//追加

	item = LayerItem_addText(p, dt, ptpos, rcdraw);

	//元の位置に移動

	if(item && ins)
		mListMove(&p->list_text, MLISTITEM(item), ins);

	return item;
}

/** テキストを削除 */

void LayerItem_deleteText(LayerItem *p,LayerTextItem *item)
{
	mListDelete(&p->list_text, MLISTITEM(item));
}

/** テキスト位置をすべて相対移動 */

void LayerItem_moveTextPos_all(LayerItem *p,int relx,int rely)
{
	LayerTextItem *pi;

	MLK_LIST_FOR(p->list_text, pi, LayerTextItem)
	{
		pi->x += relx;
		pi->y += rely;

		mRectMove(&pi->rcdraw, relx, rely);
	}
}

/** src のテキストをすべて複製して追加 */

void LayerItem_appendText_dup(LayerItem *dst,LayerItem *src)
{
	LayerTextItem *pi;

	if(!LAYERITEM_IS_TEXT(dst)) return;

	MLK_LIST_FOR(src->list_text, pi, LayerTextItem)
	{
		_add_text_from_item(dst, pi);
	}
}

/** テキストをすべてクリア */

void LayerItem_clearText(LayerItem *p)
{
	mListDeleteAll(&p->list_text);
}

/** アンドゥデータから追加
 *
 * rcdraw は別途セットする。
 *
 * buf: NULL でなし */

LayerTextItem *LayerItem_addText_undo(LayerItem *p,int x,int y,int datsize,uint8_t *buf)
{
	LayerTextItem *pi;

	pi = (LayerTextItem *)mListAppendNew(&p->list_text, sizeof(LayerTextItem) - 1 + datsize);
	if(!pi) return NULL;

	pi->x = x;
	pi->y = y;
	pi->datsize = datsize;

	if(buf)
		memcpy(pi->dat, buf, datsize);

	return pi;
}

/** アンドゥデータからテキスト置き換え */

LayerTextItem *LayerItem_replaceText_undo(LayerItem *p,LayerTextItem *item,int x,int y,int datsize,uint8_t *buf)
{
	mListItem *ins;

	ins = item->i.next;

	//削除

	mListDelete(&p->list_text, MLISTITEM(item));

	//追加

	item = LayerItem_addText_undo(p, x, y, datsize, buf);

	//元の位置に移動

	if(item && ins)
		mListMove(&p->list_text, MLISTITEM(item), ins);

	return item;
}

/** イメージの指定位置上のテキストアイテムを取得
 *
 * 重なっている場合は、大きさが小さい方を選択する。 */

LayerTextItem *LayerItem_getTextItem_atPoint(LayerItem *p,int x,int y)
{
	LayerTextItem *pi,*pmin;
	mRect rc;
	int min;

	//tmp に幅+高さをセット

	MLK_LIST_FOR(p->list_text, pi, LayerTextItem)
	{
		rc = pi->rcdraw;

		if(mRectIsEmpty(&rc))
			pi->tmp = -1;
		else
			pi->tmp = (rc.x2 - rc.x1) + (rc.y2 - rc.y1);
	}

	//範囲内にあり、大きさが最小のアイテム

	min = 0;
	pmin = NULL;

	MLK_LIST_FOR(p->list_text, pi, LayerTextItem)
	{
		if(mRectIsPointIn(&pi->rcdraw, x, y))
		{
			if(!pmin || pi->tmp < min)
			{
				min = pi->tmp;
				pmin = pi;
			}
		}
	}

	return pmin;
}

/** テキストの rcsrc の範囲を消去/再描画する際の情報を取得
 *
 * 再描画が必要なアイテムは tmp = 1 となる。
 * 重なるテキストにさらに重なっているテキストがあれば、すべて対象となる。
 *
 * rcdst: 描画が必要なすべての範囲を取得。 */

void LayerItem_getTextRedrawInfo(LayerItem *p,mRect *rcdst,const mRect *rcsrc)
{
	LayerTextItem *pi;
	mRect rc,rc2;
	int flag;

	_textitem_clear_tmp(p);

	rc = *rcsrc;

	while(1)
	{
		//rc の範囲と重なっているか
		
		flag = FALSE;
		rc2 = rc;
		
		MLK_LIST_FOR(p->list_text, pi, LayerTextItem)
		{
			if(!pi->tmp && mRectIsCross(&rc, &pi->rcdraw))
			{
				pi->tmp = 1;
				flag = TRUE;

				mRectUnion(&rc2, &pi->rcdraw);
			}
		}

		if(!flag) break;

		//重なるテキストがあった場合、そのすべての範囲を含めて再度チェック

		rc = rc2;
	}

	*rcdst = rc;
}

/** LayerTextItem から DrawTextData にデータを取得 */

void LayerTextItem_getDrawData(LayerTextItem *p,DrawTextData *dt)
{
	uint8_t *ps;
	uint32_t flags2;

	ps = p->dat;

	ps += mGetBuf_format(ps, ">iiiiiiiiii",
		&dt->font_param, &dt->fontsize, &dt->rubysize, &dt->dpi,
		&dt->char_space, &dt->line_space, &dt->ruby_pos, &dt->angle,
		&dt->flags, &flags2);

	//flags2

	dt->fontsel = flags2 & 7;
	dt->unit_fontsize = (flags2 >> 3) & 7;
	dt->unit_rubysize = (flags2 >> 6) & 7;
	dt->unit_char_space = (flags2 >> 9) & 7;
	dt->unit_line_space = (flags2 >> 12) & 7;
	dt->unit_ruby_pos = (flags2 >> 15) & 7;
	dt->hinting = (flags2 >> 18) & 7;

	//str

	ps = _get_text_string(ps, &dt->str_text);
	ps = _get_text_string(ps, &dt->str_font);
	ps = _get_text_string(ps, &dt->str_style);
}

/** LayerTextItem からテキストを取得 (文字数制限あり) */

void LayerTextItem_getText_limit(LayerTextItem *p,mStr *str,int max)
{
	uint8_t *ps = p->dat;

	ps += 4 * 10;

	_get_text_string(ps, str);

	mStrLimitText(str, max);
}

