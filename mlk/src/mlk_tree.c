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
 * mTree (ツリーデータ)
 *****************************************/

#include "mlk.h"
#include "mlk_tree.h"



//***********************************
// sub
//***********************************


/* アイテム削除 (指定アイテムのみ) */

static void _delete_item(mTree *p,mTreeItem *item)
{
	if(p->item_destroy)
		(p->item_destroy)(p, item);

	//前後をつなげる

	if(item->prev)
		item->prev->next = item->next;
	else if(item->parent)
		item->parent->first = item->next;

	if(item->next)
		item->next->prev = item->prev;
	else if(item->parent)
		item->parent->last = item->prev;

	//

	mFree(item);
}


//***********************************
// mTree
//***********************************



/**@ アイテムを新規作成して親の末尾に追加
 *
 * @g:mTree
 * @p:parent NULL でルート
 * @p:size アイテム構造体のサイズ */

mTreeItem *mTreeAppendNew(mTree *p,mTreeItem *parent,int size)
{
	mTreeItem *pi;

	if((pi = mTreeItemNew(size)))
		mTreeLinkBottom(p, parent, pi);

	return pi;
}

/**@ アイテムを新規作成して親の先頭に追加 */

mTreeItem *mTreeAppendNew_top(mTree *p,mTreeItem *parent,int size)
{
	mTreeItem *pi;

	if((pi = mTreeItemNew(size)))
		mTreeLinkTop(p, parent, pi);

	return pi;
}

/**@ アイテムを新規作成して挿入
 *
 * @p:insert 挿入位置。\
 * このアイテムの前に挿入される。\
 * NULL の場合、エラー。 */

mTreeItem *mTreeInsertNew(mTree *p,mTreeItem *insert,int size)
{
	mTreeItem *pi;
	
	if(!insert) return NULL;

	if((pi = mTreeItemNew(size)))
		mTreeLinkInsert(p, insert, pi);

	return pi;
}


//============== 削除


/**@ アイテムをすべて削除 */

void mTreeDeleteAll(mTree *p)
{
	mTreeItem *pi,*next;

	for(pi = p->top; pi; pi = next)
	{
		next = pi->next;

		mTreeDeleteChildren(p, pi);
		_delete_item(p, pi);
	}

	p->top = p->bottom = NULL;
}

/**@ 指定アイテムを削除 (子もすべて削除される) */

void mTreeDeleteItem(mTree *p,mTreeItem *item)
{
	if(item)
	{
		mTreeLinkRemove(p, item);

		mTreeDeleteChildren(p, item);
		_delete_item(p, item);
	}
}

/**@ 子アイテムをすべて削除 (root は削除しない) */

void mTreeDeleteChildren(mTree *p,mTreeItem *root)
{
	mTreeItem *pi,*parent,*next;

	pi = root->first;
	parent = root;

	while(pi)
	{
		if(pi->first)
		{
			//子アイテムがある場合 -> 子アイテムがないアイテムが来るまで繰り返す

			parent = pi;
			pi = pi->first;
		}
		else
		{
			//子アイテムがない場合、削除

			next = pi->next;

			_delete_item(p, pi);

			//次のアイテム (NULL の場合は親へ戻る)

			pi = next;

			while(!pi)
			{
				if(parent == root) break;

				pi = parent;
				parent = pi->parent;
			}
		}
	}
}


//============= リンク


/**@ アイテムを親の末尾にリンク
 *
 * @p:parent NULL でルート */

void mTreeLinkBottom(mTree *p,mTreeItem *parent,mTreeItem *item)
{
	if(!item) return;
	
	item->parent = parent;

	if(parent)
	{
		//親アイテムあり

		item->prev = parent->last;
		parent->last = item;

		if(item->prev)
			item->prev->next = item;
		else
			parent->first = item;
	}
	else
	{
		//ルート

		if(!p->top)
			//データがひとつもない時
			p->top = p->bottom = item;
		else
		{
			//データが一つ以上存在する時

			p->bottom->next = item;
			item->prev = p->bottom;
			p->bottom = item;
		}
	}
}

/**@ アイテムを親の先頭にリンク */

void mTreeLinkTop(mTree *p,mTreeItem *parent,mTreeItem *item)
{
	mTreeItem *ins;

	ins = (parent)? parent->first: p->top;

	if(ins)
		mTreeLinkInsert(p, ins, item);
	else
		mTreeLinkBottom(p, parent, item);
}

/**@ アイテムを指定アイテムの前に挿入してリンク
 *
 * @p:ins NULL の場合は挿入されない */

void mTreeLinkInsert(mTree *p,mTreeItem *ins,mTreeItem *item)
{
	if(!item || !ins) return;
	
	item->parent = ins->parent;

	/* すでに存在している ins の前に挿入するので、
	 * item が親の最後のアイテムになることはない。
	 * よって、mTree::bottom や mTreeItem::last は変化しない。 */

	if(ins->prev)
		//前のアイテムあり -> 前の next = item
		ins->prev->next = item;
	else if(item->parent)
		//ルートでなければ、親の先頭へ
		item->parent->first = item;
	else
		//ルートの先頭
		p->top = item;

	//前後をつなげる

	item->prev = ins->prev;
	ins->prev = item;
	item->next = ins;
}

/**@ 親と挿入位置を指定してリンク
 *
 * @p:parent 親。NULL でルート
 * @p:ins 挿入位置。NULL で parent の末尾へ。 */

void mTreeLinkInsert_parent(mTree *p,mTreeItem *parent,mTreeItem *ins,mTreeItem *item)
{
	if(ins)
		//ins の前へ
		mTreeLinkInsert(p, ins, item);
	else
		//parent の終端へ
		mTreeLinkBottom(p, parent, item);
}

/**@ リンクを外す (子のリンクは維持される) */

void mTreeLinkRemove(mTree *p,mTreeItem *item)
{
	if(!item) return;
	
	//前後をつなげる

	if(item->prev)
		item->prev->next = item->next;
	else if(item->parent)
		item->parent->first = item->next;
	else
		p->top = item->next;

	if(item->next)
		item->next->prev = item->prev;
	else if(item->parent)
		item->parent->last = item->prev;
	else
		p->bottom = item->prev;

	//

	item->parent = item->prev = item->next = NULL;
}


//=========== 移動


/**@ アイテム位置を移動 (src を dst の前に) */

void mTreeMoveItem(mTree *p,mTreeItem *src,mTreeItem *dst)
{
	if(src && dst && src != dst)
	{
		mTreeLinkRemove(p, src);
		mTreeLinkInsert(p, dst, src);
	}
}

/**@ アイテム位置を移動 (src を parent の先頭に)
 *
 * @p:parent NULL でルート */

void mTreeMoveItemToTop(mTree *p,mTreeItem *src,mTreeItem *parent)
{
	mTreeLinkRemove(p, src);

	if(parent)
		mTreeLinkInsert_parent(p, parent, parent->first, src);
	else
		mTreeLinkInsert_parent(p, NULL, p->top, src);
}

/**@ アイテム位置を移動 (src を parent の末尾に)
 *
 * @p:parent NULL でルート */

void mTreeMoveItemToBottom(mTree *p,mTreeItem *src,mTreeItem *parent)
{
	mTreeLinkRemove(p, src);
	mTreeLinkBottom(p, parent, src);
}


//================


/**@ parent の最初の子を取得
 *
 * @p:parent NULL でルート */

mTreeItem *mTreeGetFirstItem(mTree *p,mTreeItem *parent)
{
	return (parent)? parent->first: p->top;
}

/**@ ツリー上において一番下にあるアイテムを取得
 *
 * @d:ルートの最後のアイテムから、一番最後の子を辿っていく。 */

mTreeItem *mTreeGetBottomLastItem(mTree *p)
{
	mTreeItem *pi;

	for(pi = p->bottom; pi && pi->last; pi = pi->last);

	return pi;
}

/**@ アイテムの全体数取得
 *
 * @d:すべての親・子アイテムの総数を取得する。 */

int mTreeItemGetNum(mTree *p)
{
	mTreeItem *pi;
	int num = 0;

	for(pi = p->top; pi; pi = mTreeItemGetNext(pi), num++);

	return num;
}

/**@ 子アイテムをソート
 *
 * @d:1階層分のみソートする。root の子より下の子はそのまま。
 *
 * @p:root 親アイテム。NULL でルート。
 * @p:comp 比較関数。\
 * item1 == item2 で 0、item1 < item2 で負の値、item1 > item2 で正の値を返す。
 * @p:param 比較関数に渡すパラメータ */

void mTreeSortChildren(mTree *p,mTreeItem *root,
	int (*comp)(mTreeItem *,mTreeItem *,void *),void *param)
{
	mTree res,tmp;
	mTreeItem *pi,*pi_r,*pinext;
	int i,j,lcnt,rcnt;

	//先頭と終端

	if(root)
	{
		res.top = root->first;
		res.bottom = root->last;
	}
	else
		res = *p;

	//子アイテムが1個以下ならソートなし

	if(res.top == res.bottom) return;

	//---- マージソート

	for(i = 1; 1; i <<= 1)
	{
		tmp.top = tmp.bottom = NULL;
		pi = res.top;

		//[i][i]... をソートして tmp に追加

		for(j = 0; pi; j++, pi = pi_r)
		{
			//右側の位置

			lcnt = 0;
			rcnt = i;
			
			for(pi_r = pi; lcnt < i && pi_r; lcnt++, pi_r = pi_r->next);

			//左側・右側の先頭から、比較して小さい方を取り出して tmp に追加

			while(lcnt || rcnt)
			{
				if(lcnt && (rcnt == 0 || !pi_r || (comp)(pi, pi_r, param) <= 0))
				{
					//左側
					
					pinext = pi->next;
					mTreeLinkRemove(&res, pi);
					mTreeLinkBottom(&tmp, NULL, pi);
					pi = pinext;

					if(!pi) break;
					
					lcnt--;
				}
				else
				{
					//右側
					
					if(!pi_r) break;

					pinext = pi_r->next;
					mTreeLinkRemove(&res, pi_r);
					mTreeLinkBottom(&tmp, NULL, pi_r);
					pi_r = pinext;
					
					rcnt--;
				}
			}
		}

		res = tmp;

		//結合が1回しか行われなかった場合、終了
		if(j == 1) break;
	}

	//適用

	if(root)
	{
		root->first = res.top;
		root->last = res.bottom;

		//親を再セット

		for(pi = res.top; pi; pi = pi->next)
			pi->parent = root;
	}
	else
	{
		p->top = res.top;
		p->bottom = res.bottom;
	}
}

/**@ ツリーのアイテムをすべてソート */

void mTreeSortAll(mTree *p,int (*comp)(mTreeItem *,mTreeItem *,void *),void *param)
{
	mTreeItem *pi;

	//ルート

	mTreeSortChildren(p, NULL, comp, param);

	//子すべて

	for(pi = p->top; pi; pi = mTreeItemGetNext(pi))
	{
		if(pi->first)
			mTreeSortChildren(p, pi, comp, param);
	}
}



//***********************************
// mTreeItem
//***********************************



/**@ アイテムを新規作成
 *
 * @g:mTreeItem
 * @d:サイズ分ゼロクリアされる。 */

mTreeItem *mTreeItemNew(int size)
{
	if(size < sizeof(mTreeItem))
		size = sizeof(mTreeItem);

	return (mTreeItem *)mMalloc0(size);
}

/**@ 次のアイテム取得 (ツリー全体が対象)
 *
 * @d:子がある場合、先に子を辿る。\
 * 終端に来たら、親の次へ移動する。 */

mTreeItem *mTreeItemGetNext(mTreeItem *p)
{
	if(p->first)
		return p->first;
	else
		return mTreeItemGetNextPass(p);
}

/**@ 次のアイテム取得 (root の下位のみ)
 *
 * @d:子がある場合、子を辿る。\
 * 親に戻る場合は、root より上には行かない。 */

mTreeItem *mTreeItemGetNext_root(mTreeItem *p,mTreeItem *root)
{
	if(p->first)
		return p->first;
	else
		return mTreeItemGetNextPass_root(p, root);
}

/**@ 次のアイテム取得 (子は通過する)
 *
 * @d:子を辿らないが、親へは戻る。 */

mTreeItem *mTreeItemGetNextPass(mTreeItem *p)
{
	while(p)
	{
		if(p->next)
			return p->next;
		else
			p = p->parent;
	}

	return NULL;
}

/**@ 次のアイテム取得 (root の下位のみ。子は通過する)
 *
 * @d:子は辿らず、root より上に行かない。 */

mTreeItem *mTreeItemGetNextPass_root(mTreeItem *p,mTreeItem *root)
{
	if(p == root) return NULL;

	while(p)
	{
		if(p->next)
			return p->next;
		else
		{
			p = p->parent;
			if(p == root) return NULL;
		}
	}

	return NULL;
}

/**@ 前のアイテム取得
 *
 * @d:前のアイテムに子アイテムがある場合は、その一番下層のアイテム。\
 * 親の先頭アイテムの場合は、親が返る。 */

mTreeItem *mTreeItemGetPrev(mTreeItem *p)
{
	if(!p->prev)
		return p->parent;
	else
	{
		for(p = p->prev; p->last; p = p->last);

		return p;
	}
}

/**@ 前のアイテム取得 (root の下位のみ)
 *
 * @d:root より上には戻らない。 */

mTreeItem *mTreeItemGetPrev_root(mTreeItem *p,mTreeItem *root)
{
	if(p == root)
		return NULL;
	else if(!p->prev)
		return (p->parent == root)? NULL: p->parent;
	else
	{
		for(p = p->prev; p->last; p = p->last);

		return p;
	}
}

/**@ 前のアイテム取得 (子アイテムは通過する)
 *
 * @d:前のアイテム、または親アイテムの前のアイテム。 */

mTreeItem *mTreeItemGetPrevPass(mTreeItem *p)
{
	while(p)
	{
		if(p->prev)
			return p->prev;
		else
			p = p->parent;
	}

	return NULL;
}

/**@ 一番下層の最後のアイテム取得
 *
 * @d:p を先頭として、最後のアイテムを辿っていき、一番下にあるアイテムを取得。
 * @r:子がない場合 NULL */

mTreeItem *mTreeItemGetBottom(mTreeItem *p)
{
	for(p = p->last; p && p->last; p = p->last);

	return p;
}

/**@ 子アイテムに含まれているか
 *
 * @d:p が parent の子アイテムとして存在しているかどうか。 */

mlkbool mTreeItemIsChild(mTreeItem *p,mTreeItem *parent)
{
	//p の親を順に辿っていく

	for(p = p->parent; p; p = p->parent)
	{
		if(p == parent) return TRUE;
	}

	return FALSE;
}

