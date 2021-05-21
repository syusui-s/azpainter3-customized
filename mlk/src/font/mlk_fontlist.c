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
 * フォントリスト
 *****************************************/

#include <string.h>
#include <fontconfig/fontconfig.h>

#include "mlk.h"
#include "mlk_list.h"
#include "mlk_fontlist.h"
#include "mlk_fontconfig.h"


//----------------

typedef struct
{
	mListItem i;
	int	len,		//名前の長さ
		weight,
		slant;
	char name[1];	//名前	
}_item;

//----------------


//=========================
// sub
//=========================


/* FcFontSet 作成
 *
 * family: NULL ですべてのフォント */

static FcFontSet *_create_fontset(const char *family)
{
	FcObjectSet *os;
	FcPattern *pat;
 	FcFontSet *set;

 	if(family)
 	{
		//スタイルリスト
		os = FcObjectSetBuild(FC_STYLE, FC_WEIGHT, FC_SLANT, (void *)0);
		pat = FcPatternBuild(0, FC_FAMILY, FcTypeString, family, (void *)0);
 	}
 	else
 	{
		//フォント名列挙時
		os = FcObjectSetBuild(FC_FAMILY, (void *)0);
		pat = FcPatternCreate();
 	}

	set = FcFontList(0, pat, os);

	FcPatternDestroy(pat);
	FcObjectSetDestroy(os);

	return set;
}

/* フォントを検索してリストを作成
 *
 * family: NULL でファミリ名検索
 * return: 0 で成功、それ以外で失敗 */

static int _create_list(mList *list,const char *family)
{
	FcFontSet *set;
	const char *object;
	char *name;
	_item *pi;
	int i,len,failed = FALSE;

	//FcFontSet 作成

	set = _create_fontset(family);
	if(!set) return 1;

	//リストに追加

	object = (family)? FC_STYLE: FC_FAMILY;

	for(i = 0; i < set->nfont; i++)
	{
		name = mFCPattern_getText(set->fonts[i], object);

		if(name)
		{
			len = strlen(name);
		
			pi = (_item *)mListAppendNew(list, sizeof(_item) + len);
			if(!pi)
			{
				failed = TRUE;
				break;
			}

			pi->len = len;
			memcpy(pi->name, name, len + 1);

			//スタイル時、太さと斜体 (ソート用)

			if(family)
			{
				pi->weight = mFCPattern_getInt_def(set->fonts[i], FC_WEIGHT, 0);
				pi->slant  = mFCPattern_getInt_def(set->fonts[i], FC_SLANT, 0);
			}
		}
	}

	//

	FcFontSetDestroy(set);

	if(failed)
	{
		mListDeleteAll(list);
		return 1;
	}

	return 0;
}

/* フォント名ソート関数 */

static int _comp_family(mListItem *item1,mListItem *item2,void *param)
{
	return strcmp(((_item *)item1)->name, ((_item *)item2)->name);
}

/* スタイル ソート関数 */

static int _comp_style(mListItem *item1,mListItem *item2,void *param)
{
	_item *p1,*p2;

	p1 = (_item *)item1;
	p2 = (_item *)item2;

	//斜体 > 太さの順

	if(p1->slant != p2->slant)
		return (p1->slant < p2->slant)? -1: 1;
	else if(p1->weight != p2->weight)
		return (p1->weight < p2->weight)? -1: 1;
	else
		return 0;
}


//=========================
// main
//=========================


/**@ 使用可能なすべてのファミリ名を列挙
 *
 * @p:func ファミリ名ごとに呼ばれる関数。\
 *  ファミリ名の文字列は UTF-8。\
 *  戻り値が 0 以外でエラー。中断させる。
 * @p:param 関数に渡されるパラメータ値
 * @r:成功時、ファミリの数。-1 でエラー。 */

int mFontList_getFamilies(mFuncFontListFamily func,void *param)
{
	mList list = MLIST_INIT;
	_item *pi,*next;
	int ret;

	//リスト作成

	if(_create_list(&list, NULL))
		return -1;

	mListSort(&list, _comp_family, NULL);

	//同名アイテム削除

	for(pi = (_item *)list.top; 1; pi = next)
	{
		next = (_item *)pi->i.next;
		if(!next) break;

		if(strcmp(pi->name, next->name) == 0)
			mListDelete(&list, MLISTITEM(pi));
	}

	ret = list.num;

	//関数呼び出し

	MLK_LIST_FOR(list, pi, _item)
	{
		if((func)(pi->name, param))
		{
			ret = -1;
			break;
		}
	}

	mListDeleteAll(&list);

	return ret;
}

/**@ 指定ファミリ名で使用可能なスタイルを列挙
 *
 * @p:family ファミリ名 (UTF-8)
 * @p:func スタイルごとに呼び出される関数。\
 *  戻り値が 0 以外でエラー。中断させる。
 * @r:-1 でエラーまたは中断。\
 *  0 で、Medium (100) より太いスタイルがある。\
 *  1 で、Medium より太いスタイルがない。\
 *  太字のスタイルがないフォントに対して、Bold スタイルを追加したい時に判断する。 */

int mFontList_getStyles(const char *family,mFuncFontListStyle func,void *param)
{
	mList list = MLIST_INIT;
	_item *pi;
	int ret,is_bold = 0;

	//リスト作成

	if(_create_list(&list, family))
		return -1;

	mListSort(&list, _comp_style, NULL);

	//関数呼び出し

	ret = 0;

	MLK_LIST_FOR(list, pi, _item)
	{
		if((func)(pi->name, pi->weight, pi->slant, param))
		{
			ret = -1;
			break;
		}

		//Medium 以上の太さがあるか
		if(pi->weight > 100) is_bold = 1;
	}

	mListDeleteAll(&list);

	//戻り値

	if(ret != -1)
		ret = (is_bold)? 0: 1;

	return ret;
}

