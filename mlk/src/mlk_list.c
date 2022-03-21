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
 * mList (双方向リスト)
 *****************************************/

#include "mlk.h"
#include "mlk_list.h"



/**@ アイテムを確保
 *
 * @d:サイズ分ゼロクリアされる
 * 
 * @p:size 構造体の全サイズ。\
 * mListItem + データのサイズを確保する。\
 * mListItem のサイズ以下の場合は、自動で mListItem のサイズ。 */

mListItem *mListItemNew(int size)
{
	mListItem *p;

	if(size < sizeof(mListItem))
		size = sizeof(mListItem);
	
	p = (mListItem *)mMalloc0(size);
	if(!p) return NULL;
	
	return p;
}

/**@ mList 初期化 */

void mListInit(mList *list)
{
	mMemset0(list, sizeof(mList));
}


//====================
// 追加/取り外し
//====================


/**@ アイテムを新規作成して末尾に追加
 * 
 * @p:size アイテム構造体のサイズ */

mListItem *mListAppendNew(mList *list,int size)
{
	mListItem *pi;
	
	pi = mListItemNew(size);

	mListAppendItem(list, pi);

	return pi;
}

/**@ アイテムを新規作成して挿入
 *
 * @p:pos 挿入位置。このアイテムの前に挿入される。NULL で末尾に追加。 */

mListItem *mListInsertNew(mList *list,mListItem *pos,int size)
{
	mListItem *pi;
	
	pi = mListItemNew(size);

	mListInsertItem(list, pos, pi);

	return pi;
}

/**@ アイテムをリストの末尾に追加 */

void mListAppendItem(mList *list,mListItem *item)
{
	if(item)
	{
		mListLinkAppend(list, item);
		list->num++;
	}
}

/**@ アイテムをリストに挿入
 *
 * @p:pos NULL で末尾に追加 */

void mListInsertItem(mList *list,mListItem *pos,mListItem *item)
{
	if(item)
	{
		mListLinkInsert(list, item, pos);
		list->num++;
	}
}

/**@ アイテムを削除せずにリストから取り外す
 *
 * @d:mList::num の値も変化する。 */

void mListRemoveItem(mList *list,mListItem *item)
{
	if(item)
	{
		mListLinkRemove(list, item);
		list->num--;
	}
}


//=========================


/**@ リスト全体を複製
 *
 * @d:※各アイテムは固定サイズであること。
 *
 * @p:itemsize 各アイテムのサイズ
 * @r:メモリが足りなかった場合 FALSE */

mlkbool mListDup(mList *dst,mList *src,int itemsize)
{
	mListItem *ps,*pd;

	if(itemsize < sizeof(mListItem))
		itemsize = sizeof(mListItem);

	for(ps = src->top; ps; ps = ps->next)
	{
		pd = (mListItem *)mMemdup(ps, itemsize);
		if(!pd)
		{
			mListDeleteAll(dst);
			return FALSE;
		}

		mListLinkAppend(dst, pd);
	}

	dst->item_destroy = src->item_destroy;
	dst->num = src->num;

	return TRUE;
}


//========================
// 削除
//========================


/**@ アイテムを全て削除 */

void mListDeleteAll(mList *list)
{
	mListItem *pi,*next;
	void (*destroy)(mList *,mListItem *) = list->item_destroy;
	
	for(pi = list->top; pi; pi = next)
	{
		next = pi->next;
		
		if(destroy)
			(destroy)(list, pi);
		
		mFree(pi);
	}
	
	list->top = list->bottom = NULL;
	list->num = 0;
}

/**@ リストからアイテムを削除
 *
 * @p:item NULL の場合は何もしない */

void mListDelete(mList *list,mListItem *item)
{
	if(item)
	{
		if(list->item_destroy)
			(list->item_destroy)(list, item);

		mListLinkRemove(list, item);
		
		mFree(item);
		
		list->num--;
	}
}

/**@ リストからアイテムを削除
 * 
 * @d:item_destroy ハンドラは呼ばない。 */

void mListDelete_no_handler(mList *list,mListItem *item)
{
	mListLinkRemove(list, item);
	
	mFree(item);
	
	list->num--;
}

/**@ インデックス位置からアイテム削除
 * 
 * @r:FALSE で位置が範囲外 */

mlkbool mListDelete_index(mList *list,int index)
{
	mListItem *p = mListGetItemAtIndex(list, index);
	
	if(!p)
		return FALSE;
	else
	{
		mListDelete(list, p);
		return TRUE;
	}
}

/**@ 先頭から指定個数を削除 */

void mListDelete_tops(mList *list,int num)
{
	mListItem *p,*next;

	for(p = list->top; p && num > 0; num--, p = next)
	{
		next = p->next;

		mListDelete(list, p);
	}
}

/**@ 終端から指定個数を削除 */

void mListDelete_bottoms(mList *list,int num)
{
	mListItem *p,*next;

	for(p = list->bottom; p && num > 0; num--, p = next)
	{
		next = p->prev;

		mListDelete(list, p);
	}
}


//========================
// アイテム位置移動
//========================


/**@ アイテムの位置を移動
 *
 * @p:src 移動元アイテム
 * @p:dst 移動先。dst の前に挿入される。NULL で最後尾へ。
 * @r:FALSE で位置が変わらなかった */

mlkbool mListMove(mList *list,mListItem *src,mListItem *dst)
{
	if(src == dst) return FALSE;
	
	if(!dst && list->bottom == src) return FALSE;
	
	mListLinkRemove(list, src);
	mListLinkInsert(list, src, dst);

	return TRUE;
}

/**@ アイテムを先頭へ移動
 *
 * @r:FALSE ですでに先頭 */

mlkbool mListMoveToTop(mList *list,mListItem *item)
{
	if(list->top == item)
		return FALSE;
	else
	{
		mListMove(list, item, list->top);
		return TRUE;
	}
}

/**@ アイテムを上下に移動
 *
 * @p:up TRUE で上へ、FALSE で下へ
 * @r:移動したか */

mlkbool mListMoveUpDown(mList *list,mListItem *item,mlkbool up)
{
	if(!item) return FALSE;
	
	if(up)
	{
		if(item->prev)
		{
			mListMove(list, item, item->prev);
			return TRUE;
		}
	}
	else
	{
		if(item->next)
		{
			mListMove(list, item->next, item);
			return TRUE;
		}
	}

	return FALSE;
}

/**@ アイテムの位置を入れ替える */

void mListSwapItem(mList *list,mListItem *item1,mListItem *item2)
{
	mListItem *p;

	if(item1 != item2)
	{
		p = item1->next;
		
		mListMove(list, item1, item2->next);
		mListMove(list, item2, p);
	}
}

/**@ ソート
 *
 * @p:comp 比較関数。\
 * item1 == item2 で 0、item1 < item2 で負の値、item1 > item2 で正の値を返す。
 * @p:param 比較関数に渡すパラメータ */

void mListSort(mList *list,int (*comp)(mListItem *,mListItem *,void *),void *param)
{
	mList res,tmp;
	mListItem *pi,*pi_r,*pinext;
	int i,j,lcnt,rcnt;

	if(!list->top || list->top == list->bottom)
		return;

	//マージソート

	res = *list;

	for(i = 1; 1; i <<= 1)
	{
		tmp.top = tmp.bottom = NULL;
		pi = res.top;

		//各ブロックをソートして tmp に追加

		for(j = 0; pi; j++, pi = pi_r)
		{
			//2分割の右側の位置と左の個数

			lcnt = 0;
			rcnt = i;
			
			for(pi_r = pi; lcnt < i && pi_r; lcnt++, pi_r = pi_r->next);

			//左側・右側の先頭アイテムを比較。
			//小さい方を取り出して tmp に追加

			while(lcnt || rcnt)
			{
				if(lcnt && (rcnt == 0 || !pi_r || (comp)(pi, pi_r, param) <= 0))
				{
					//左側
					
					pinext = pi->next;
					mListLinkRemove(&res, pi);
					mListLinkAppend(&tmp, pi);
					pi = pinext;

					if(!pi) break;
					
					lcnt--;
				}
				else
				{
					//右側
					
					if(!pi_r) break;

					pinext = pi_r->next;
					mListLinkRemove(&res, pi_r);
					mListLinkAppend(&tmp, pi_r);
					pi_r = pinext;
					
					rcnt--;
				}
			}
		}

		res = tmp;

		//結合が1回しか行われなかった場合、終了
		if(j == 1) break;
	}

	list->top = res.top;
	list->bottom = res.bottom;
}


//========================
// ほか
//========================


/**@ インデックス位置からアイテム取得
 *
 * @p:index 負の値、またはアイテム数より大きい場合は NULL が返る。 */

mListItem *mListGetItemAtIndex(mList *list,int index)
{
	mListItem *p;

	if(index < 0) return NULL;
	
	for(p = list->top; p && index > 0; p = p->next, index--);
	
	return p;
}

/**@ 2つのアイテムの位置関係を得る
 *
 * @r:0 で item1 == item2。\
 * 1 で item1 は item2 より後。\
 * -1 で item1 は item2 より前 */

int mListItemGetDir(mListItem *item1,mListItem *item2)
{
	mListItem *prev,*next;
	
	if(item1 == item2) return 0;

	prev = item1->prev;
	next = item1->next;

	while(prev && next)
	{
		if(prev == item2) return 1;
		if(next == item2) return -1;
	
		prev = prev->prev;
		next = next->next;
	}

	//どちらかが NULL になった
	return (!next)? 1: -1;
}

/**@ アイテムのインデックス位置取得
 *
 * @r:item == NULL の場合、-1 */

int mListItemGetIndex(mListItem *item)
{
	int pos = -1;

	for(; item; item = item->prev, pos++);
	
	return pos;
}


//================================
// リンク操作
//================================


/**@ リストの末尾にリンクを追加
 *
 * @g:リンク操作
 * @d:{em:mList::num の値は操作されない:em} */

void mListLinkAppend(mList *list,mListItem *item)
{
	if(list->bottom)
	{
		item->prev = list->bottom;
		(list->bottom)->next = item;
	}
	else
	{
		item->prev = NULL;
		list->top = item;
	}
	
	item->next = NULL;
	list->bottom = item;
}

/**@ リストの指定位置にリンクを挿入
 * 
 * @p:pos 挿入位置。この前に挿入される。NULL で末尾に追加。 */

void mListLinkInsert(mList *list,mListItem *item,mListItem *pos)
{
	if(!pos)
		mListLinkAppend(list, item);
	else
	{
		if(pos->prev)
			(pos->prev)->next = item;
		else
			list->top = item;
		
		item->prev = pos->prev;
		pos->prev = item;
		item->next = pos;
	}
}

/**@ リストからアイテムのリンクを外す */

void mListLinkRemove(mList *list,mListItem *item)
{
	if(item->prev)
		(item->prev)->next = item->next;
	else
		list->top = item->next;
	
	if(item->next)
		(item->next)->prev = item->prev;
	else
		list->bottom = item->prev;
}


//================================
// キャッシュリスト
//================================


/**@ キャッシュリストにアイテムを新規作成
 *
 * @g:キャッシュリスト
 * @d:キャッシュリスト内では、
 * 参照カウンタ (mListCacheItem::refcnt) が 1 以上のアイテムは常に存在する。\
 * 参照カウンタが 0 のアイテムは、明示的に削除するまで保持される。\
 * \
 * リストのアイテム順は、[top] (NEW) refcnt > 0 (OLD) | (OLD) refcnt == 0 (NEW) [bottom] となる。\
 * \
 * アイテム追加時は、参照カウンタ 1 で、リストの先頭に置かれる。 */

mListCacheItem *mListCache_appendNew(mList *list,int size)
{
	mListCacheItem *pi;

	pi = (mListCacheItem *)mListItemNew(size);
	if(pi)
	{
		//先頭に追加

		mListInsertItem(list, list->top, MLISTITEM(pi));

		pi->refcnt = 1;
	}

	return pi;
}

/**@ キャッシュアイテムの参照カウンタを増やす
 *
 * @d:同時に、アイテムを先頭へ移動させる。 */

void mListCache_refItem(mList *list,mListCacheItem *item)
{
	item->refcnt++;

	mListMoveToTop(list, MLISTITEM(item));
}

/**@ キャッシュリストからアイテムを解放
 *
 * @d:参照カウンタを一つ減らす。\
 * 参照カウンタが 0 になったら末尾へ移動し、そのまま残る。 */

void mListCache_releaseItem(mList *list,mListCacheItem *item)
{
	if(item->refcnt)
	{
		item->refcnt--;

		if(item->refcnt == 0)
			mListMove(list, MLISTITEM(item), NULL);
	}
}

/**@ 参照カウンタが 0 のアイテムを、最大で指定数になるように削除
 *
 * @d:リスト内の参照カウンタ 0 のアイテム (未使用状態のアイテム) が、
 * 全体で maxnum の数になるように、古いものから削除する。 */

void mListCache_deleteUnused(mList *list,int maxnum)
{
	mListCacheItem *pi,*prev;
	int cnt = 0;

	//recnt == 0 のアイテムは、終端に old <--> new の順で並んでいるので、
	//末尾から maxnum 分を保持し、それより前のアイテムは削除

	for(pi = MLISTCACHEITEM(list->bottom); pi && pi->refcnt == 0; pi = prev)
	{
		prev = MLISTCACHEITEM(pi->prev);

		if(cnt < maxnum)
			//保持
			cnt++;
		else
			mListDelete(list, MLISTITEM(pi));
	}
}

/**@ リスト全体の数が指定数以上の時、未使用の古いアイテムを削除
 *
 * @d:リスト全体の数が num になるように、未使用アイテムを古い順に削除する。 */

void mListCache_deleteUnused_allnum(mList *list,int num)
{
	mListCacheItem *pi,*next;
	int delnum;

	if(list->num <= num) return;

	//未使用の先頭

	for(pi = MLISTCACHEITEM(list->top); pi && pi->refcnt; pi = MLISTCACHEITEM(pi->next));

	//未使用の古いものを削除

	delnum = list->num - num;

	for(; pi && delnum; pi = next, delnum--)
	{
		next = MLISTCACHEITEM(pi->next);

		mListDelete(list, MLISTITEM(pi));
	}
}

