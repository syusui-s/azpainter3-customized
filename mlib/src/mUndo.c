/*$
 Copyright (C) 2013-2020 Azel.

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
/*
 * # リスト順
 *
 * [UNDO 古 -> 新] -> (CURRENT) -> [REDO 新 -> 古]
 *
 * # カレント位置
 *
 * 現在のアンドゥデータ位置。次以降のデータはリドゥデータ。
 * NULL の場合はアンドゥデータなしで全てリドゥデータの状態。
 */

#include "mDef.h"
#include "mUndo.h"
#include "mList.h"


/*****************//**

@defgroup undo mUndo
@brief アンドゥ管理

- 実行時などにエラーとなった場合は、データが全て削除される。

@ingroup group_data
@{

@file mUndo.h

*********************/


/** 解放 */

void mUndoFree(mUndo *p)
{
	if(p)
	{
		mListDeleteAll(&p->list);
		mFree(p);
	}
}

/** 作成
 *
 * 派生した構造体を使う場合、ゼロクリアすれば作成しなくても良い。 */

mUndo *mUndoNew(int size)
{
	mUndo *p;

	if(size < sizeof(mUndo)) size = sizeof(mUndo);

	p = (mUndo *)mMalloc(size, TRUE);
	if(!p) return NULL;

	p->maxnum = 100;

	return p;
}

/** アンドゥまたはリドゥデータがあるか */

mBool mUndoIsHave(mUndo *p,mBool redo)
{
	mListItem *pi;

	if(redo)
		pi = (p->current)? p->current->next: p->list.top;
	else
		pi = (mListItem *)p->current;

	return (pi != NULL);
}

/** すべて削除 */

void mUndoDeleteAll(mUndo *p)
{
	mListDeleteAll(&p->list);

	p->current = NULL;
}

/** アンドゥデータのみ削除 (リドゥデータは残す) */

void mUndoDelete_onlyUndo(mUndo *p)
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

/** リドゥデータのみ削除 */

void mUndoDelete_onlyRedo(mUndo *p)
{
	mListItem *pi,*next;

	/* カレントの次以降がリドゥデータ。
	 * カレントが NULL ならすべてリドゥデータ。 */

	pi = (p->current)? p->current->next: p->list.top;

	//

	for(; pi; pi = next)
	{
		next = pi->next;

		mListDelete(&p->list, pi);
	}
}

/** アンドゥデータ追加 */

void mUndoAdd(mUndo *p,mListItem *item)
{
	//リドゥデータをすべて削除

	mUndoDelete_onlyRedo(p);

	//最大回数を超えたら古いデータを削除

	while(p->list.num >= p->maxnum && p->list.top)
		mListDelete(&p->list, p->list.top);

	//リストに追加

	mListAppend(&p->list, item);

	//カレント

	p->current = item;
}

/** アンドゥ実行 */

int mUndoRunUndo(mUndo *p)
{
	mListItem *pi;

	//アンドゥデータがない

	if(!p->current) return MUNDO_RUNERR_NO_DATA;

	//リドゥデータをセット
	/* [UNDO_B][UNDO_A(current)][REDO_A] => [UNDO_B(current)][_del_][REDO_NEW][REDO_A] */

	pi = (p->create)(p);
	if(!pi) return MUNDO_RUNERR_CREATE;

	mListInsert(&p->list, p->current->next, pi);

	if(!(p->setreverse)(p, pi, p->current, MUNDO_TYPE_REDO))
		mUndoDelete_onlyRedo(p);

	//アンドゥ処理を実行

	if(!(p->run)(p, p->current, MUNDO_TYPE_UNDO))
	{
		mUndoDeleteAll(p);
		return MUNDO_RUNERR_RUN;
	}

	//カレントを削除

	mListDelete(&p->list, p->current);

	p->current = pi->prev;

	return MUNDO_RUNERR_OK;
}

/** リドゥ実行 */

int mUndoRunRedo(mUndo *p)
{
	mListItem *redo,*undo;

	/* [UNDO_A(CURRENT)][REDO_CUR] => [UNDO_A][UNDO_NEW(CURRENT)][_del_] */

	//リドゥデータ

	redo = (p->current)? p->current->next: p->list.top;

	if(!redo) return MUNDO_RUNERR_NO_DATA;

	//アンドゥデータをセット

	undo = (p->create)(p);
	if(!undo) return MUNDO_RUNERR_CREATE;

	mListInsert(&p->list, redo, undo);

	if(!(p->setreverse)(p, undo, redo, MUNDO_TYPE_UNDO))
	{
		mUndoDeleteAll(p);
		return MUNDO_RUNERR_CREATE;
	}

	//リドゥ処理

	if(!(p->run)(p, redo, MUNDO_TYPE_REDO))
	{
		mUndoDeleteAll(p);
		return MUNDO_RUNERR_RUN;
	}

	//リドゥデータ削除

	mListDelete(&p->list, redo);

	//カレント

	p->current = undo;

	return MUNDO_RUNERR_OK;
}

