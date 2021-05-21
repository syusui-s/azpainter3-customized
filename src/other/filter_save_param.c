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

/***************************************
 * フィルタのパラメータ値保存管理
 ***************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mlk.h"
#include "mlk_list.h"
#include "mlk_iniread.h"
#include "mlk_iniwrite.h"


/*
 * "a100b4c400..."
 * というように、英小文字を識別子として扱い、その後に数値が続く。
 */

//----------------

typedef struct
{
	mListItem i;

	int cmdid;	//フィルタのコマンドID
	char text[1];
}_item;

//----------------


/* コマンドIDからアイテム検索 */

static _item *_find_id(mList *list,int cmdid)
{
	_item *pi;

	MLK_LIST_FOR(*list, pi, _item)
	{
		if(cmdid == pi->cmdid) return pi;
	}

	return NULL;
}

/* アイテム追加 */

static _item *_add_item(mList *list,int cmdid,const char *text)
{
	_item *pi = NULL;
	int len;
	
	if(text)
	{
		len = strlen(text);
		
		pi = (_item *)mListAppendNew(list, sizeof(_item) + len);
		if(pi)
		{
			pi->cmdid = cmdid;

			memcpy(pi->text, text, len);
		}
	}

	return pi;
}


/** 設定ファイルからデータ読み込み */

void FilterParam_getConfig(mList *list,mIniRead *ini)
{
	const char *key,*txt;

	if(mIniRead_setGroup(ini, "filter_param"))
	{
		while(mIniRead_getNextItem(ini, &key, &txt))
		{
			_add_item(list, atoi(key), txt);
		}
	}
}

/** 設定ファイルに書き込み */

void FilterParam_setConfig(mList *list,void *fp_ptr)
{
	FILE *fp = (FILE *)fp_ptr;
	_item *pi;

	if(!list->top) return;

	mIniWrite_putGroup(fp, "filter_param");

	MLK_LIST_FOR(*list, pi, _item)
	{
		mIniWrite_putText_keyno(fp, pi->cmdid, pi->text);
	}
}

/** 指定コマンドIDのテキストを取得 */

char *FilterParam_getText(mList *list,int cmdid)
{
	_item *pi = _find_id(list, cmdid);

	return (pi)? pi->text: NULL;
}

/** データを追加または変更 */

void FilterParam_setData(mList *list,int cmdid,const char *text)
{
	_item *pi = _find_id(list, cmdid);

	//存在する場合、削除

	if(pi)
		mListDelete(list, MLISTITEM(pi));

	//追加

	_add_item(list, cmdid, text);
}

/** テキストから、項目の値を取得
 *
 * return: TRUE で取得できた */

mlkbool FilterParam_getValue(const char *text,char id,int *dst)
{
	const char *p;

	if(!text) return FALSE;

	for(p = text; *p; )
	{
		if(*p == id)
		{
			//値取得

			*dst = atoi(p + 1);

			return TRUE;
		}
		
		//次へ (英小文字以外をスキップ)

		for(p++; *p && (*p < 'a' || *p > 'z'); p++);
	}

	return FALSE;
}

