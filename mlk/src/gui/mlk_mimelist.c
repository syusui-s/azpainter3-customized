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
 * MIME タイプリスト
 *
 * クリップボード/D&D で使われる。
 *****************************************/

#include <string.h>

#include "mlk_gui.h"
#include "mlk_list.h"

#include "mlk_pv_gui.h"
#include "mlk_pv_mimelist.h"


//------------------

#define _ITEM(p) ((mMimeListItem *)(p))

/* mMimeListItem::param
    [X11] Atom
    [Wayland] struct wl_data_offer *
*/

//------------------



/** リスト内に指定タイプが存在するか (type と param が一致するもののみ) */

mlkbool mMimeListFind_eq_param(mList *list,const char *type,void *param)
{
	mMimeListItem *pi;

	for(pi = _ITEM(list->top); pi; pi = _ITEM(pi->i.next))
	{
		if(pi->param == param
			&& strcmp(pi->name, type) == 0)
			return TRUE;
	}

	return FALSE;
}

/** リストに指定タイプが存在するか (名前のみ判定) */

mlkbool mMimeListFind_eq(mList *list,const char *type)
{
	mMimeListItem *pi;

	for(pi = _ITEM(list->top); pi; pi = _ITEM(pi->i.next))
	{
		if(strcmp(pi->name, type) == 0)
			return TRUE;
	}

	return FALSE;
}

/** リストに追加 */

void mMimeListAdd(mList *list,const char *type,void *param)
{
	mMimeListItem *pi;
	int len;

	len = strlen(type);

	pi = (mMimeListItem *)mListAppendNew(list, sizeof(mMimeListItem) + len);
	if(pi)
	{
		memcpy(pi->name, type, len + 1);

		pi->param = param;
	}
}

/** リストに追加
 *  (タイプと param が同じアイテムが存在する場合は追加しない) */

void mMimeListAdd_check(mList *list,const char *type,void *param)
{
	if(!mMimeListFind_eq_param(list, type, param))
		mMimeListAdd(list, type, param);
}

/** param が一致するアイテムをすべて削除 */

void mMimeListDelete_param(mList *list,void *param)
{
	mMimeListItem *pi,*next;

	for(pi = _ITEM(list->top); pi; pi = next)
	{
		next = _ITEM(pi->i.next);
		
		if(pi->param == param)
			mListDelete(list, MLISTITEM(pi));
	}
}

/** char* の配列で各タイプのリストを作成 */

char **mMimeListCreateNameList(mList *list)
{
	mMimeListItem *pi;
	char **buf,**pd,*ptr;

	//バッファ

	buf = (char **)mMalloc0(sizeof(char *) * (list->num + 1));
	if(!buf) return NULL;

	//セット

	pd = buf;

	for(pi = _ITEM(list->top); pi; pi = _ITEM(pi->i.next))
	{
		ptr = mStrdup(pi->name);
		if(!ptr) break;

		*(pd++) = ptr;
	}

	return buf;
}

/** char* 配列で各タイプのリストを作成 (param が一致するもののみ)
 *
 * return: 一つもない場合 NULL */

char **mMimeListCreateNameList_eq_param(mList *list,void *param)
{
	mMimeListItem *pi;
	char **buf,**pd,*ptr;
	int num;

	//個数

	num = 0;

	for(pi = _ITEM(list->top); pi; pi = _ITEM(pi->i.next))
	{
		if(pi->param == param)
			num++;
	}

	if(num == 0) return NULL;

	//バッファ

	buf = (char **)mMalloc0(sizeof(char *) * (num + 1));
	if(!buf) return NULL;

	//セット

	pd = buf;

	for(pi = _ITEM(list->top); pi; pi = _ITEM(pi->i.next))
	{
		if(pi->param == param)
		{
			ptr = mStrdup(pi->name);
			if(!ptr) break;

			*(pd++) = ptr;
		}
	}

	return buf;
}
