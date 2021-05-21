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
 * mUndo
 *****************************************/

#include "mlk.h"
#include "mlk_undo.h"
#include "mlk_list.h"


/*-------------------
 <リスト順>
 
 [UNDO:古 -> 新] -> (CURRENT) -> [REDO:新 -> 古]
 
 <カレント位置>
 
 現在のアンドゥデータ位置。
 next 以降はリドゥデータ。
 NULL の場合は、アンドゥデータがなく、全てリドゥデータの状態。
---------------------*/


/**@ 解放 */

void mUndoFree(mUndo *p)
{
	if(p)
	{
		mListDeleteAll(&p->list);
		mFree(p);
	}
}

/**@ 作成
 *
 * @d:終了時にリストを手動で削除するなら、mUndoNew() で作成しなくても良い。
 *
 * @p:size mUndo のサイズ */

mUndo *mUndoNew(int size)
{
	mUndo *p;

	if(size < sizeof(mUndo))
		size = sizeof(mUndo);

	p = (mUndo *)mMalloc0(size);
	if(!p) return NULL;

	p->maxnum = 100;

	return p;
}

/**@ アンドゥデータがあるか */

mlkbool mUndoIsHaveUndo(mUndo *p)
{
	return (p->current != NULL);
}

/**@ リドゥデータがあるか */

mlkbool mUndoIsHaveRedo(mUndo *p)
{
	mListItem *pi;

	pi = (p->current)? p->current->next: p->list.top;

	return (pi != NULL);
}

/**@ すべてのデータを削除 */

void mUndoDeleteAll(mUndo *p)
{
	mListDeleteAll(&p->list);

	p->current = NULL;
}

/**@ アンドゥデータのみ削除 (リドゥデータは残す) */

void mUndoDelete_undo(mUndo *p)
{
	mListItem *pi,*next;

	if(!p->current) return;

	for(pi = p->list.top; pi; pi = next)
	{
		next = pi->next;

		mListDelete(&p->list, pi);

		if(pi == p->current) break;
	}

	p->current = NULL;
}

/**@ リドゥデータのみ削除 */

void mUndoDelete_redo(mUndo *p)
{
	mListItem *pi,*next;

	//カレントの次以降がリドゥデータ。
	//カレントが NULL なら、すべてリドゥデータ。

	pi = (p->current)? p->current->next: p->list.top;

	//

	for(; pi; pi = next)
	{
		next = pi->next;

		mListDelete(&p->list, pi);
	}
}

/**@ アンドゥデータ追加 */

void mUndoAdd(mUndo *p,mListItem *item)
{
	//リドゥデータをすべて削除

	mUndoDelete_redo(p);

	//最大回数を超えたら古いデータを削除

	while(p->list.num >= p->maxnum && p->list.top)
		mListDelete(&p->list, p->list.top);

	//リストに追加

	mListAppendItem(&p->list, item);

	//カレント

	p->current = item;
}

/**@ アンドゥ実行
 *
 * @r:EMPTY = データがない */

mlkerr mUndoRun_undo(mUndo *p)
{
	mListItem *pi;
	mlkerr ret;

	//アンドゥデータがない

	if(!p->current) return MLKERR_EMPTY;

	//リドゥデータをセット
	// [UNDO_B][UNDO_A(current)][REDO_A] => [UNDO_B(current)][_del_][REDO_NEW][REDO_A]

	ret = (p->newitem)(p, &pi);
	if(ret) return ret;

	mListInsertItem(&p->list, p->current->next, pi);

	ret = (p->set_reverse)(p, pi, p->current, MUNDO_TYPE_REDO);
	if(ret)
		mUndoDelete_redo(p);

	//アンドゥ処理を実行

	ret = (p->run)(p, p->current, MUNDO_TYPE_UNDO);
	if(ret)
	{
		mUndoDeleteAll(p);
		return ret;
	}

	//カレントを削除

	mListDelete(&p->list, p->current);

	p->current = pi->prev;

	return MLKERR_OK;
}

/**@ リドゥ実行 */

mlkerr mUndoRun_redo(mUndo *p)
{
	mListItem *redo,*undo;
	mlkerr ret;

	//[UNDO_A(CURRENT)][REDO_CUR] => [UNDO_A][UNDO_NEW(CURRENT)][_del_]

	//リドゥデータ

	redo = (p->current)? p->current->next: p->list.top;

	if(!redo) return MLKERR_EMPTY;

	//アンドゥデータをセット

	ret = (p->newitem)(p, &undo);
	if(ret) return ret;

	mListInsertItem(&p->list, redo, undo);

	ret = (p->set_reverse)(p, undo, redo, MUNDO_TYPE_UNDO);
	if(ret)
	{
		mUndoDeleteAll(p);
		return ret;
	}

	//リドゥ処理

	ret = (p->run)(p, redo, MUNDO_TYPE_REDO);
	if(ret)
	{
		mUndoDeleteAll(p);
		return ret;
	}

	//リドゥデータ削除

	mListDelete(&p->list, redo);

	//カレント

	p->current = undo;

	return MLKERR_OK;
}

