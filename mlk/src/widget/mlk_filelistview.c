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
 * mFileListView
 *****************************************/

#include <string.h>
#include <time.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_filelistview.h"
#include "mlk_listheader.h"
#include "mlk_listviewpage.h"
#include "mlk_columnitem.h"
#include "mlk_imagelist.h"
#include "mlk_font.h"
#include "mlk_event.h"

#include "mlk_str.h"
#include "mlk_util.h"
#include "mlk_string.h"
#include "mlk_dir.h"
#include "mlk_filestat.h"

#include "mlk_columnitem_manager.h"


//----------------------

//mFileListView アイテム用 mColumnItem

typedef struct
{
	mColumnItem i;
	uint64_t filesize,	//ファイルサイズ
		modify;			//更新日時
}_itemex;

//アイテムの param 値

#define _FILETYPE_PARENT  0
#define _FILETYPE_DIR     1
#define _FILETYPE_FILE    2

//アイコン番号

#define _ICONNO_FILE  0
#define _ICONNO_DIRECTORY 1

//----------------------


//リストのアイコン PNG データ (16x16 x 2 [file,directory])
static const unsigned char g_icon_pngdat[163] = {
0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a,0x00,0x00,0x00,0x0d,0x49,0x48,0x44,0x52,
0x00,0x00,0x00,0x20,0x00,0x00,0x00,0x10,0x08,0x02,0x00,0x00,0x00,0xf8,0x62,0xea,
0x0e,0x00,0x00,0x00,0x6a,0x49,0x44,0x41,0x54,0x18,0x95,0xed,0xcf,0xb1,0x09,0x00,
0x21,0x10,0x44,0xd1,0x29,0xe0,0xaa,0xb3,0x4a,0x1b,0xb1,0x86,0xad,0xc3,0xc4,0xd4,
0x0b,0x84,0x63,0x59,0x75,0x5c,0xf0,0xcc,0xfc,0x0c,0x08,0x9b,0x3c,0x04,0x2a,0xce,
0xae,0x3d,0xcf,0xbc,0xdf,0x80,0xda,0xd5,0x8e,0x03,0xa3,0x6f,0x07,0x18,0x18,0x40,
0xc9,0x51,0x8f,0x19,0x1e,0xc0,0x1a,0x80,0xa4,0x60,0x36,0xfd,0x16,0x07,0x74,0x1c,
0x18,0x93,0x1c,0x10,0x95,0x01,0x4a,0x8e,0x7c,0x2e,0x40,0x4b,0x17,0xb8,0x80,0x07,
0x10,0x9a,0x01,0x24,0x85,0xe5,0x2c,0xb0,0x4c,0x03,0xde,0x3e,0xe0,0xdc,0x5e,0x68,
0x63,0x17,0x50,0xa9,0x16,0x4e,0x55,0x00,0x00,0x00,0x00,0x49,0x45,0x4e,0x44,0xae,
0x42,0x60,0x82 };

//----------------------



//=============================
// sub
//=============================


/* ソート関数 */

static int _sortfunc_file(mListItem *item1,mListItem *item2,void *param_ptr)
{
	_itemex *p1,*p2;
	char *text1,*text2;
	int type,rev,n,param;

	p1 = (_itemex *)item1;
	p2 = (_itemex *)item2;
	param = (intptr_t)param_ptr; //ソートパラメータ値

	if(p1->i.param < p2->i.param)
		//タイプ
		return -1;
	else if(p1->i.param > p2->i.param)
		//タイプ
		return 1;
	else
	{
		type = param & 0xff;
		rev = param >> 8;
		n = -2;

		//ファイルサイズ

		if(type == MFILELISTVIEW_SORTTYPE_FILESIZE
			&& p1->i.param == _FILETYPE_FILE
			&& p1->filesize != p2->filesize)
		{
			n = (p1->filesize < p2->filesize)? -1: 1;
			if(rev) n = -n;
		}

		//更新日時
		
		if(type == MFILELISTVIEW_SORTTYPE_MODIFY
			&& p1->modify != p2->modify)
		{
			n = (p1->modify < p2->modify)? -1: 1;
			if(rev) n = -n;
		}

		//ファイル名
		//(ファイル名以外のソートで、上記の比較が行われなかった場合は、常にファイル名:降順)

		if(n == -2)
		{
			mColumnItem_getColText(MLK_COLUMNITEM(p1), 0, &text1);
			mColumnItem_getColText(MLK_COLUMNITEM(p2), 0, &text2);

			n = mStringCompare_number(text1, text2);

			if(rev && type == MFILELISTVIEW_SORTTYPE_FILENAME)
				n = -n;
		}

		return n;
	}
}

/* アイテムをソートする */

static void _sortitem(mFileListView *p)
{
	mListViewSortItem(MLK_LISTVIEW(p), _sortfunc_file,
		(void *)(intptr_t)((p->flv.sort_rev << 8) | p->flv.sort_type) );
}

/* アイテム追加
 *
 * str: 作業用 */

static void _add_listitem(mFileListView *p,const char *name,mFileStat *st,mStr *str)
{
	_itemex *pi;
	time_t t;
	char m[32];
	int directory;

	directory = ((st->flags & MFILESTAT_F_DIRECTORY) != 0);

	//ファイル名

	mStrSetText(str, name);
	mStrAppendChar(str, '\t');

	//サイズ

	if(!directory)
	{
		if(st->size < 1024)
			mStrAppendFormat(str, "%d byte", st->size);
		else if(st->size < 1024 * 1024)
			mStrAppendFormat(str, "%.1F KiB", st->size * 10 / 1024);
		else if(st->size < 1024 * 1024 * 1024)
			mStrAppendFormat(str, "%.1F MiB", st->size * 10 / (1024 * 1024));
		else
			mStrAppendFormat(str, "%.1F GiB", st->size * 10 / (1024 * 1024 * 1024));
	}

	mStrAppendChar(str, '\t');

	//更新日時

	t = st->time_modify;
	strftime(m, 32, "%Y/%m/%d %H:%M", localtime(&t));
	
	mStrAppendText(str, m);

	//追加
	
	pi = (_itemex *)mListViewAddItem_size(MLK_LISTVIEW(p), sizeof(_itemex),
		str->buf,
		(directory)? _ICONNO_DIRECTORY: _ICONNO_FILE, 0,
		(directory)? _FILETYPE_DIR: _FILETYPE_FILE);

	if(pi)
	{
		pi->filesize = st->size;
		pi->modify = st->time_modify;
	}
}

/* ファイルリストセット */

static void _set_filelist(mFileListView *p)
{
	mDir *dir;
	const char *fname;
	mStr str = MSTR_INIT;
	mFileStat st;
	mlkbool hide_hidden_files;

	//クリア

	mListViewDeleteAllItem(MLK_LISTVIEW(p));

	//親に戻る

	if(!mStrPathIsTop(&p->flv.strDir))
		mListViewAddItem(MLK_LISTVIEW(p), "..", _ICONNO_DIRECTORY, 0, _FILETYPE_PARENT);

	//------ ファイル

	hide_hidden_files = !(p->flv.fstyle & MFILELISTVIEW_S_SHOW_HIDDEN_FILES);

	dir = mDirOpen(p->flv.strDir.buf);
	if(!dir) return;

	while(mDirNext(dir))
	{
		if(mDirIsSpecName(dir)) continue;

		//隠しファイル除外

		if(hide_hidden_files && mDirIsHiddenFile(dir))
			continue;

		//
		
		fname = mDirGetFilename(dir);

		mDirGetStat(dir, &st);

		if(st.flags & MFILESTAT_F_DIRECTORY)
		{
			//ディレクトリ
			_add_listitem(p, fname, &st, &str);
		}
		else
		{
			//--- ファイル

			//ディレクトリのみ時

			if(p->flv.fstyle & MFILELISTVIEW_S_ONLY_DIR)
				continue;

			//フィルタ適用

			if(mStringMatch_sum(fname, p->flv.strFilter.buf, TRUE) == -1)
				continue;

			//追加

			_add_listitem(p, fname, &st, &str);
		}
	}

	mStrFree(&str);

	mDirClose(dir);

	//ソート

	_sortitem(p);
}

/* 通知 */

static void _notify(mFileListView *p,int type)
{
	mWidgetEventAdd_notify(MLK_WIDGET(p),
		MWIDGET_EVENT_ADD_NOTIFY_SEND_RAW, type, 0, 0);
}

/* mStr に指定アイテムのファイル名を追加
 *
 * addsep: 現在の str の最後にパス区切り文字を追加 */

static void _append_itemname(mStr *str,mColumnItem *pi,mlkbool addsep)
{
	char *top;
	int len;

	len = mColumnItem_getColText(pi, 0, &top);

	if(len)
	{
		if(addsep)
			mStrPathAppendDirSep(str);

		mStrAppendText_len(str, top, len);
	}
}


//=============================
// main
//=============================


/**@ mFileListView データ解放 */

void mFileListViewDestroy(mWidget *wg)
{
	mFileListView *p = MLK_FILELISTVIEW(wg);

	mStrFree(&p->flv.strDir);
	mStrFree(&p->flv.strFilter);

	//リストビュー

	mListViewDestroy(wg);
}

/**@ 作成 */

mFileListView *mFileListViewNew(mWidget *parent,int size,uint32_t fstyle)
{
	mFileListView *p;
	mImageList *img;
	mListHeader *header;
	mFont *font;

	//mListView
	
	if(size < sizeof(mFileListView))
		size = sizeof(mFileListView);
	
	p = (mFileListView *)mListViewNew(parent, size,
			((fstyle & MFILELISTVIEW_S_MULTI_SEL)? MLISTVIEW_S_MULTI_SEL: 0)
			 | MLISTVIEW_S_DESTROY_IMAGELIST | MLISTVIEW_S_GRID_COL
			 | MLISTVIEW_S_MULTI_COLUMN | MLISTVIEW_S_HAVE_HEADER | MLISTVIEW_S_HEADER_SORT,
			MSCROLLVIEW_S_HORZVERT_FRAME);

	if(!p) return NULL;

	p->wg.destroy = mFileListViewDestroy;
	p->wg.event = mFileListViewHandle_event;
	p->wg.flayout = MLF_EXPAND_WH;
	p->wg.notify_to_rep = MWIDGET_NOTIFYTOREP_SELF; //自身を送信元とする通知は、自身で受ける

	p->flv.fstyle = fstyle;

	//アイテム高さ

	font = mWidgetGetFont(MLK_WIDGET(p));

	mListViewSetItemHeight_min(MLK_LISTVIEW(p), mFontGetHeight(font) + 2);

	//mListHeader アイテム

	header = mListViewGetHeaderWidget(MLK_LISTVIEW(p));

	MLK_TRGROUP(MLK_TRGROUP_ID_SYSTEM);

	mListHeaderAddItem(header, MLK_TR(MLK_TRSYS_FILENAME), 100,
		MLISTHEADER_ITEM_F_EXPAND, MFILELISTVIEW_SORTTYPE_FILENAME);

	mListHeaderAddItem(header, MLK_TR(MLK_TRSYS_FILESIZE),
		mFontGetTextWidth(font, "9999.9 GiB", -1) + MLK_LISTVIEWPAGE_ITEM_PADDING_X * 2,
		MLISTHEADER_ITEM_F_RIGHT, MFILELISTVIEW_SORTTYPE_FILESIZE);

	mListHeaderAddItem(header, MLK_TR(MLK_TRSYS_FILEMODIFY),
		mFontGetTextWidth(font, "9999/99/99 99:99", -1) + MLK_LISTVIEWPAGE_ITEM_PADDING_X * 2, 0,
		MFILELISTVIEW_SORTTYPE_MODIFY);

	mListHeaderSetSort(header, 0, FALSE);

	//アイコン

	img = mImageListLoadPNG_buf(g_icon_pngdat, sizeof(g_icon_pngdat), 16);
	if(img)
	{
		mImageListSetTransparentColor(img, 0x00ff00);

		mListViewSetImageList(MLK_LISTVIEW(p), img);
	}

	//パス

	mStrPathSetHome(&p->flv.strDir);

	mFileListView_setFilter(p, NULL);
	
	return p;
}

/**@ リストの現在のディレクトリをセット
 *
 * @p:path NULL または空文字列で、ホームディレクトリ */

void mFileListView_setDirectory(mFileListView *p,const char *path)
{
	if(!path || !(*path))
		mStrPathSetHome(&p->flv.strDir);
	else
	{
		mStrSetText(&p->flv.strDir, path);
		mStrPathRemoveBottomDirSep(&p->flv.strDir);
	}

	_set_filelist(p);
}

/**@ ファイルのフィルタ文字列をセット
 *
 * @p:filter ワイルドカード有効。複数ある場合は、';' で区切る。\
 *  例: "*.png;*.bmp"\
 *  NULL の場合、すべてという意味の "*" にセットされる。 */

void mFileListView_setFilter(mFileListView *p,const char *filter)
{
	if(!filter)
		mStrSetChar(&p->flv.strFilter, '*');
	else
	{
		mStrSetText(&p->flv.strFilter, filter);
		mStrReplaceChar_null(&p->flv.strFilter, ';');
	}

	//終端は NULL が2つ必要
	mStrAppendChar(&p->flv.strFilter, 0);
}

/**@ フィルタの文字列をセット (拡張子のみ指定)
 *
 * @d:主に保存時用に使う。
 * 
 * @p:ext 拡張子の文字列。'.' は含まない。';' で区切って複数指定可。\
 *  NULL または空文字列で、"*" となる。 */

void mFileListView_setFilter_ext(mFileListView *p,const char *ext)
{
	mStr *str = &p->flv.strFilter;
	const char *pc,*end;

	if(!ext || !(*ext) || (ext[0] == '*' && ext[1] == 0))
	{
		//'*'

		mStrSetChar(str, '*');
		mStrAppendChar(str, 0);
	}
	else
	{
		mStrEmpty(str);
		
		pc = ext;

		while(mStringGetNextSplit(&pc, &end, ';'))
		{
			mStrAppendText(str, "*.");
			mStrAppendText_len(str, pc, end - pc);
			mStrAppendChar(str, 0);
	
			pc = end;
		}

		mStrAppendChar(str, 0);
	}
}

/**@ 隠しファイルの表示を変更
 *
 * @d:type 0=表示しない、正=表示する、負=切り替え */

void mFileListView_setShowHiddenFiles(mFileListView *p,int type)
{
	if(mIsChangeFlagState(type, p->flv.fstyle & MFILELISTVIEW_S_SHOW_HIDDEN_FILES))
	{
		p->flv.fstyle ^= MFILELISTVIEW_S_SHOW_HIDDEN_FILES;

		_set_filelist(p);
	}
}

/**@ ソートタイプを変更
 *
 * @p:type 0=ファイル名、1=ファイルサイズ、2=更新日時 */

void mFileListView_setSortType(mFileListView *p,int type,mlkbool rev)
{
	p->flv.sort_type = type;
	p->flv.sort_rev = rev;

	mListHeaderSetSort(mListViewGetHeaderWidget(MLK_LISTVIEW(p)), type, rev);

	_sortitem(p);
}

/**@ 隠しファイルを表示するかの状態を取得 */

mlkbool mFileListView_isShowHiddenFiles(mFileListView *p)
{
	return ((p->flv.fstyle & MFILELISTVIEW_S_SHOW_HIDDEN_FILES) != 0);
}

/**@ ファイルリストを更新 */

void mFileListView_updateList(mFileListView *p)
{
	_set_filelist(p);
}

/**@ ディレクトリを一つ上の親に移動 */

void mFileListView_changeDir_parent(mFileListView *p)
{
	mStrPathRemoveBasename(&p->flv.strDir);

	_set_filelist(p);
	_notify(p, MFILELISTVIEW_N_CHANGE_DIR);
}

/**@ 現在のフォーカスアイテムのパスを取得
 *
 * @p:fullpath TRUE でフルパスで取得。FALSE でパスを含まない。
 * @r:セットされたパスのタイプ。MFILELISTVIEW_FILETYPE_*。\
 *  選択されていない、または親へ移動するアイテムの場合、NONE。 */

int mFileListView_getSelectFileName(mFileListView *p,mStr *str,mlkbool fullpath)
{
	mColumnItem *pi;

	mStrEmpty(str);

	pi = p->lv.manager.item_focus;

	if(!pi || pi->param == _FILETYPE_PARENT)
		return MFILELISTVIEW_FILETYPE_NONE;
	else
	{
		//パスセット

		if(fullpath)
			mStrCopy(str, &p->flv.strDir);

		_append_itemname(str, pi, fullpath);

		return (pi->param == _FILETYPE_FILE)?
			MFILELISTVIEW_FILETYPE_FILE: MFILELISTVIEW_FILETYPE_DIR;
	}
}

/**@ 複数選択有効時、すべてのパスを取得
 *
 * @p:str タブ文字で区切られている。先頭がディレクトリパスで、以降がファイル名。\
 *  "directory\\tfile1\\tfile2\\t...\\t"
 * @r:セットされたファイル数 */

int mFileListView_getSelectMultiName(mFileListView *p,mStr *str)
{
	mColumnItem *pi;
	int num = 0;

	//ディレクトリ

	mStrCopy(str, &p->flv.strDir);
	mStrAppendChar(str, '\t');

	//ファイル

	for(pi = MLK_COLUMNITEM(p->lv.manager.list.top); pi; pi = MLK_COLUMNITEM(pi->i.next))
	{
		if((pi->flags & MCOLUMNITEM_F_SELECTED) && pi->param == _FILETYPE_FILE)
		{
			_append_itemname(str, pi, FALSE);
			mStrAppendChar(str, '\t');

			num++;
		}
	}

	return num;
}


//========================
// ハンドラ
//========================


/* ダブルクリック時 */

static void _dblclk(mFileListView *p)
{
	mColumnItem *pi;

	pi = p->lv.manager.item_focus;
	if(!pi) return;

	switch(pi->param)
	{
		//親へ
		case _FILETYPE_PARENT:
			mFileListView_changeDir_parent(p);
			break;
		//ディレクトリ
		case _FILETYPE_DIR:
			//パス追加
			_append_itemname(&p->flv.strDir, pi, TRUE);

			_set_filelist(p);
			_notify(p, MFILELISTVIEW_N_CHANGE_DIR);
			break;
		//ファイル
		case _FILETYPE_FILE:
			_notify(p, MFILELISTVIEW_N_DBLCLK_FILE);
			break;
	}
}

/**@ event ハンドラ関数 */

int mFileListViewHandle_event(mWidget *wg,mEvent *ev)
{
	mFileListView *p = MLK_FILELISTVIEW(wg);
	mColumnItem *pi;

	//mListView の通知イベント

	if(ev->type == MEVENT_NOTIFY
		&& ev->notify.widget_from == wg)
	{
		switch(ev->notify.notify_type)
		{
			//フォーカスアイテム変更
			case MLISTVIEW_N_CHANGE_FOCUS:
				pi = p->lv.manager.item_focus;

				if(pi && pi->param == _FILETYPE_FILE)
					_notify(p, MFILELISTVIEW_N_SELECT_FILE);
				break;
			//ダブルクリック
			case MLISTVIEW_N_ITEM_L_DBLCLK:
				_dblclk(p);
				break;
			//ソート情報変更
			case MLISTVIEW_N_CHANGE_SORT:
				p->flv.sort_type = MLK_LISTHEADER_ITEM(ev->notify.param1)->param;
				p->flv.sort_rev = ev->notify.param2;

				_sortitem(p);
				break;
		}
	
		return TRUE;
	}

	return mListViewHandle_event(wg, ev);
}

