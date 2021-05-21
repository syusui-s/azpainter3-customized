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
 * mSysDlg ファイル選択 / mFileDialog
 *****************************************/

#include <string.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_sysdlg.h"
#include "mlk_filedialog.h"
#include "mlk_event.h"
#include "mlk_menu.h"

#include "mlk_label.h"
#include "mlk_button.h"
#include "mlk_lineedit.h"
#include "mlk_combobox.h"
#include "mlk_imgbutton.h"
#include "mlk_arrowbutton.h"
#include "mlk_filelistview.h"

#include "mlk_str.h"
#include "mlk_file.h"
#include "mlk_string.h"

#include "mlk_pv_gui.h"


/* 翻訳グループは、システムにセットされるが、ダイアログ終了時に元のグループに戻る。 */


//********************************
// mSysDlg 関数
//********************************


/**@ ファイルを開くダイアログ
 *
 * @d:翻訳グループは、内部で一時的にシステムにセットされるが、ダイアログ終了時には、
 *  ダイアログ開始前のグループに戻る。
 *
 * @p:filter リストに表示するファイルのフィルタ。\
 *  フィルタの表示名と、フィルタ文字列を２つセットにして、それぞれを '\\t' で区切って複数指定する。\
 *  フィルタ文字列は、複数ある場合、';' で区切る。\
 *  例: "image file\\t*.png;*.bmp\\tall file\\t*"
 * @p:def_filter 初期表示時に選択するフィルタのインデックス番号
 * @p:initdir 初期ディレクトリのパス。\
 *  NULL または空文字列の時、strdst にパスがあれば、そのディレクトリ。なければ、ホームディレクトリ。
 * @p:strdst 選択されたファイル名が入る。\
 *  単一選択時は、フルパス名。\
 *  複数選択時は、"directory\\tfile1\\tfile2...\\t" の文字列になる。 */

mlkbool mSysDlg_openFile(mWindow *parent,const char *filter,int def_filter,
	const char *initdir,uint32_t flags,mStr *strdst)
{
	mFileDialog *p;
	uint32_t fstyle = 0;

	if(flags & MSYSDLG_FILE_F_MULTI_SEL)
		fstyle |= MFILEDIALOG_S_MULTI_SEL;

	if(flags & MSYSDLG_FILE_F_DEFAULT_FILENAME)
		fstyle |= MFILEDIALOG_S_DEFAULT_FILENAME;

	//

	p = mFileDialogNew(parent, 0, fstyle, MFILEDIALOG_TYPE_OPENFILE);
	if(!p) return FALSE;

	mFileDialogInit(p, filter, def_filter, initdir, strdst);

	mFileDialogShow(p);

	return mDialogRun(MLK_DIALOG(p), TRUE);
}

/**@ ファイルを保存するダイアログ
 *
 * @d:フィルタで拡張子が指定されている場合、ファイル名指定でその拡張子が付いていなければ、自動で拡張子が付く。\
 *  拡張子が複数ある場合は、その中のいずれかの拡張子がついていない場合、最初の拡張子が付く。
 *
 * @p:filter フィルタ文字列。\
 *  表示名と、保存時の拡張子 ('.' は除く) を２つセットにして、それぞれを '\\t' で区切って複数指定する。\
 *  拡張子の指定が必要ない場合は、"*" にする。\
 *  拡張子は ';' で区切って複数指定可能。その場合、ファイルリストのフィルタに使われる。\
 *  例: "PNG file\\tpng\\tJPEG file\\tjpg;jpeg\\timage file\\t*"
 * @p:strdst 結果のフルパス名が入る。\
 *  初期表示時、空でなければ、この文字列がデフォルトのファイル名としてセットされる。
 * @p:pfiltertype NULL 以外で、選択されたフィルタのインデックス番号が入る。 */

mlkbool mSysDlg_saveFile(mWindow *parent,const char *filter,int def_filter,
	const char *initdir,uint32_t flags,mStr *strdst,int *pfiltertype)
{
	mFileDialog *p;
	uint32_t fstyle = 0;

	if(flags & MSYSDLG_FILE_F_NO_OVERWRITE_MES)
		fstyle |= MFILEDIALOG_S_NO_OVERWRITE_MES;

	if(flags & MSYSDLG_FILE_F_DEFAULT_FILENAME)
		fstyle |= MFILEDIALOG_S_DEFAULT_FILENAME;

	//

	p = mFileDialogNew(parent, 0, fstyle, MFILEDIALOG_TYPE_SAVEFILE);
	if(!p) return FALSE;

	p->fdlg.dst_filtertype = pfiltertype;

	mFileDialogInit(p, filter, def_filter, initdir, strdst);

	mFileDialogShow(p);

	return mDialogRun(MLK_DIALOG(p), TRUE);
}

/**@ ディレクトリ選択ダイアログ
 *
 * @p:strdst 結果のフルパス名が入る */

mlkbool mSysDlg_selectDir(mWindow *parent,const char *initdir,uint32_t flags,mStr *strdst)
{
	mFileDialog *p;

	p = mFileDialogNew(parent, 0, 0, MFILEDIALOG_TYPE_DIR);
	if(!p) return FALSE;

	mFileDialogInit(p, NULL, 0, initdir, strdst);

	mFileDialogShow(p);

	return mDialogRun(MLK_DIALOG(p), TRUE);
}


//********************************
// mFileDialog
//********************************


/*-----------------

filter: 確保されたフィルタ文字列。\t が 0 に変換されている。

-------------------*/

//ディレクトリ上へ移動ボタン
static const unsigned char g_bttimg_dirup[]={
0x80,0x00,0xc0,0x01,0xe0,0x03,0xf0,0x07,0xf8,0x0f,0xfc,0x1f,0xfe,0x3f,0xff,0x7f,
0xe0,0x03,0xe0,0x03,0xe0,0x03,0xe0,0x03,0xe0,0x03,0xe0,0x03,0xe0,0x03 };



//============================
// sub - 初期化
//============================


/* ファイル種類のコンボボックスセット
 *
 * アイテムの param には、フィルタ文字列の拡張子部分のポインタが入る
 *
 * filter: 元のフィルタ文字列を '\t'->0 にした状態 */

static void _set_combo_type(mComboBox *pcb,const char *filter,int def)
{
	const char *pc,*pc2,*set;

	if(!filter) return;

	for(pc = filter; *pc; )
	{
		//pc = 表示名
		//pc2 = 拡張子部分
		
		pc2 = pc + strlen(pc) + 1;
		if(*pc2 == 0) break;

		//"*" の場合は NULL にする
		// :ファイル保存時、拡張子が必要ない場合は空文字列にするべきだが、
		// :ヌル文字区切りで空文字列は指定できないため(終端とみなされる)。

		if(pc2[0] == '*' && pc2[1] == 0)
			set = NULL;
		else
			set = pc2;

		mComboBoxAddItem_static(pcb, pc, (intptr_t)set);

		pc = pc2 + strlen(pc2) + 1;
	}

	//選択

	mComboBoxSetSelItem_atIndex(pcb, def);
}

/* コンボボックスの選択から、フィルタ文字列セット */

static void _set_filter_to_list(mFileDialog *p)
{
	char *pc;

	pc = (char *)mComboBoxGetItemParam(p->fdlg.cbtype, -1);

	if(p->fdlg.type == MFILEDIALOG_TYPE_SAVEFILE)
		//保存時
		mFileListView_setFilter_ext(p->fdlg.flist, pc);
	else
		mFileListView_setFilter(p->fdlg.flist, pc);
}

/* メイン部分のウィジェット作成 */

static void _create_widget_main(mFileDialog *p)
{
	mWidget *cttop,*ct,*wg;
	mImgButton *imgbtt;
	uint32_t f;

	//垂直コンテナ

	cttop = mContainerCreateVert(MLK_WIDGET(p), 7, MLF_EXPAND_WH, 0);

	//------ ディレクトリ

	ct = mContainerCreateHorz(cttop, 4, MLF_EXPAND_W, 0);

	//ディレクトリエディット

	p->fdlg.edit_dir = mLineEditCreate(ct, 0,
		MLF_EXPAND_W | MLF_MIDDLE, 0, MLINEEDIT_S_NOTIFY_ENTER);

	//親へ移動するボタン

	imgbtt = mImgButtonCreate(ct, 0, MLF_MIDDLE, 0, 0);
	p->fdlg.btt_moveup = (mWidget *)imgbtt;
	
	mImgButton_setBitImage(imgbtt, MIMGBUTTON_TYPE_1BIT_TP_TEXT, g_bttimg_dirup, 15, 15);

	//メニューボタン

	p->fdlg.btt_menu = wg = (mWidget *)mArrowButtonCreate(ct, 0, MLF_MIDDLE, 0, MARROWBUTTON_S_DOWN);

	wg->hintRepW = wg->hintRepH = 15 + 8;
	
	//----- ファイルリスト

	f = 0;

	//ディレクトリのみ

	if(p->fdlg.type == MFILEDIALOG_TYPE_DIR)
		f |= MFILELISTVIEW_S_ONLY_DIR;

	//複数選択 (ファイル開く時)

	if(p->fdlg.type == MFILEDIALOG_TYPE_OPENFILE
		&& (p->fdlg.fstyle & MFILEDIALOG_S_MULTI_SEL))
		f |= MFILELISTVIEW_S_MULTI_SEL;

	//隠しファイル

	if(MLKAPP->filedlgconf.flags & MAPPBASE_FILEDIALOGCONF_F_SHOW_HIDDEN_FILES)
		f |= MFILELISTVIEW_S_SHOW_HIDDEN_FILES;

	//作成

	p->fdlg.flist = mFileListViewNew(cttop, 0, f);

	mFileListView_setSortType(p->fdlg.flist,
		MLKAPP->filedlgconf.sort_type,
		((MLKAPP->filedlgconf.flags & MAPPBASE_FILEDIALOGCONF_F_SORT_REV) != 0));

	//------ ファイル名・種類

	if(p->fdlg.type == MFILEDIALOG_TYPE_OPENFILE)
	{
		//[ファイル開く]

		p->fdlg.cbtype = mComboBoxCreate(cttop, 0, MLF_EXPAND_W, 0, 0);
	}
	else if(p->fdlg.type == MFILEDIALOG_TYPE_SAVEFILE)
	{
		//[ファイル保存]

		//ファイル名

		ct = mContainerCreateGrid(cttop, 2, 5, 6, MLF_EXPAND_W, 0);

		mLabelCreate(ct, MLF_RIGHT | MLF_MIDDLE, 0, 0, MLK_TR(MLK_TRSYS_FILENAME));

		p->fdlg.edit_name = mLineEditCreate(ct, 0, MLF_EXPAND_W, 0, 0);

		//種類

		mWidgetNew(ct, 0);

		p->fdlg.cbtype = mComboBoxCreate(ct, 0, MLF_EXPAND_W, 0, 0);
	}
}

/* ウィジェット作成 */

static void _create_widget(mFileDialog *p)
{
	mWidget *ct;
	mButton *btt;

	//トップレイアウト

	mContainerSetPadding_same(MLK_CONTAINER(p), 8);

	//メイン部分

	_create_widget_main(p);

	//---- ボタン

	ct = mContainerCreateHorz(MLK_WIDGET(p), 4, MLF_RIGHT, MLK_MAKE32_4(0,8,0,0));

	//開く or 保存

	btt = mButtonCreate(ct, MLK_WID_OK, 0, 0, 0,
		(p->fdlg.type == MFILEDIALOG_TYPE_SAVEFILE)? MLK_TR(MLK_TRSYS_SAVE): MLK_TR(MLK_TRSYS_OPEN));

	btt->wg.fstate |= MWIDGET_STATE_ENTER_SEND;

	//キャンセル

	mButtonCreate(ct, MLK_WID_CANCEL, 0, 0, 0, MLK_TR(MLK_TRSYS_CANCEL));
}


//============================
// main
//============================


/**@ mFileDialog データ解放 */

void mFileDialogDestroy(mWidget *wg)
{
	mFree(MLK_FILEDIALOG(wg)->fdlg.filter);
}

/**@ 作成
 *
 * @d:ウィジェットが作成される。\
 *  他に必要なウィジェットがあれば、この後にトップコンテナの子として作成すること。\
 *  トップコンテナのコンテナタイプは変更しても良い。\
 *  作成後は、初期化と表示を行った後、mDialogRun() を実行する。
 *
 * @p:fstyle mFileDialog のスタイル */

mFileDialog *mFileDialogNew(mWindow *parent,int size,uint32_t fstyle,int dlgtype)
{
	mFileDialog *p;
	int n;
	
	if(size < sizeof(mFileDialog))
		size = sizeof(mFileDialog);
	
	p = (mFileDialog *)mDialogNew(parent, size, MTOPLEVEL_S_DIALOG_NORMAL);
	if(!p) return NULL;
	
	p->wg.destroy = mFileDialogDestroy;
	p->wg.event = mFileDialogHandle_event;
	
	p->fdlg.fstyle = fstyle;
	p->fdlg.type = dlgtype;

	//翻訳グループ

	mGuiTransSaveGroup();

	MLK_TRGROUP(MLK_TRGROUP_ID_SYSTEM);

	//ウィジェット作成

	_create_widget(p);

	//タイトル

	switch(dlgtype)
	{
		case MFILEDIALOG_TYPE_SAVEFILE:
			n = MLK_TRSYS_SAVE_FILE;
			break;
		case MFILEDIALOG_TYPE_DIR:
			n = MLK_TRSYS_SELECT_DIRECTORY;
			break;
		default:
			n = MLK_TRSYS_OPEN_FILE;
			break;
	}

	mToplevelSetTitle(MLK_TOPLEVEL(p), MLK_TR(n));

	return p;
}

/**@ 初期化
 *
 * @d:各情報をセットし、ウィジェットに設定  */

void mFileDialogInit(mFileDialog *p,const char *filter,int def_filter,
	const char *initdir,mStr *strdst)
{
	mStr str = MSTR_INIT;

	p->fdlg.strdst = strdst;

	//ファイル名 & フォーカス

	if(p->fdlg.type == MFILEDIALOG_TYPE_SAVEFILE)
	{
		//ファイル保存時はファイル名入力部分へ

		mLineEditSetText(p->fdlg.edit_name, strdst->buf);

		mWidgetSetFocus(MLK_WIDGET(p->fdlg.edit_name));
	}
	else
		//それ以外はリストへ
		mWidgetSetFocus(MLK_WIDGET(p->fdlg.flist));

	//フィルタ

	if(p->fdlg.type != MFILEDIALOG_TYPE_DIR)
	{
		//フィルタ文字列、\t を 0 に変換

		p->fdlg.filter = mStringDup_replace0(filter, '\t');

		//mComboBox にセット

		_set_combo_type(p->fdlg.cbtype, p->fdlg.filter, def_filter);

		//リストにフィルタセット

		_set_filter_to_list(p);
	}

	//---- 初期ディレクトリセット

	if(!initdir || !(*initdir))
	{
		//NULL または空文字列の場合、strdst が空でなければ、そのファイルのパスを使う
		
		if(mStrIsnotEmpty(strdst))
		{
			mStrPathGetDir(&str, strdst->buf);
			initdir = str.buf;
		}
	}

	//ディレクトリが存在するか

	if(initdir && !mIsExistDir(initdir))
		initdir = NULL;
	
	mFileListView_setDirectory(p->fdlg.flist, initdir);

	mLineEditSetText(p->fdlg.edit_dir, (p->fdlg.flist)->flv.strDir.buf);

	//---- 初期ファイル名

	if(p->fdlg.edit_name && (p->fdlg.fstyle & MFILEDIALOG_S_DEFAULT_FILENAME))
	{
		mStrPathGetBasename(&str, strdst->buf);

		mLineEditSetText(p->fdlg.edit_name, str.buf);
	}

	//

	mStrFree(&str);
}

/**@ ウィンドウ表示
 *
 * @d:リサイズと表示が行われる。\
 *  前回のサイズがある場合は復元、なければデフォルトのサイズ。 */

void mFileDialogShow(mFileDialog *p)
{
	int w,h;

	w = MLKAPP->filedlgconf.width;
	h = MLKAPP->filedlgconf.height;

	if(w && h)
	{
		//サイズ指定あり
		mWidgetResize(MLK_WIDGET(p), w, h);
		mWidgetShow(MLK_WIDGET(p), 1);
	}
	else
	{
		//初期表示時、リストの初期サイズを指定して、推奨サイズで
		
		mWidgetSetInitSize_fontHeightUnit(MLK_WIDGET(p->fdlg.flist), 33, 23);
		mWindowResizeShow_initSize(MLK_WINDOW(p));
	}
}


//========================
// イベント
//========================


/* ディレクトリ変更時 */

static void _change_dir(mFileDialog *p)
{
	mLineEditSetText(p->fdlg.edit_dir, (p->fdlg.flist)->flv.strDir.buf);

	//常に右端が表示されるように
	mLineEditMoveCursor_toRight(p->fdlg.edit_dir);
}

/* ディレクトリエディット上での ENTER 押し時
 *
 * 存在しないディレクトリの場合は、空の状態となる。 */

static void _dir_enter(mFileDialog *p)
{
	mStr str = MSTR_INIT;

	mLineEditGetTextStr(p->fdlg.edit_dir, &str);

	mFileListView_setDirectory(p->fdlg.flist, str.buf);

	mStrFree(&str);

	_change_dir(p);
}

/* 選択ファイル変更時 */

static void _change_selfile(mFileDialog *p)
{
	int type;
	mStr str = MSTR_INIT;

	type = mFileListView_getSelectFileName(p->fdlg.flist, &str, FALSE);

	if(type == MFILELISTVIEW_FILETYPE_FILE)
	{
		//保存時、ファイル名セット
		
		if(p->fdlg.type == MFILEDIALOG_TYPE_SAVEFILE)
			mLineEditSetText(p->fdlg.edit_name, str.buf);

		//ハンドラ

		if(p->fdlg.on_selectfile)
		{
			mFileListView_getSelectFileName(p->fdlg.flist, &str, TRUE);

			(p->fdlg.on_selectfile)(p, str.buf);
		}
	}

	mStrFree(&str);
}

/* メニュー */

static void _run_menu(mFileDialog *p)
{
	mMenu *menu;
	mMenuItem *item;
	mBox box;
	int id;

	//---- メニュー
	//[!] 翻訳グループは変更しないこと

	menu = mMenuNew();
	if(!menu) return;

	//ホームディレクトリ
	mMenuAppendText(menu, 0, MLK_TR_SYS(MLK_TRSYS_HOME_DIRECTORY));

	mMenuAppendSep(menu);

	//隠しファイル表示
	mMenuAppendCheck(menu, 100, MLK_TR_SYS(MLK_TRSYS_SHOW_HIDDEN_FILES),
		mFileListView_isShowHiddenFiles(p->fdlg.flist));

	mWidgetGetBox_rel(p->fdlg.btt_menu, &box);

	item = mMenuPopup(menu, p->fdlg.btt_menu, 0, 0, &box,
		MPOPUP_F_RIGHT | MPOPUP_F_BOTTOM | MPOPUP_F_GRAVITY_LEFT | MPOPUP_F_GRAVITY_BOTTOM, NULL);

	id = (item)? mMenuItemGetID(item): -1;

	mMenuDestroy(menu);

	if(id == -1) return;

	//--------

	switch(id)
	{
		//ホームディレクトリ
		case 0:
			mFileListView_setDirectory(p->fdlg.flist, NULL);
			_change_dir(p);
			break;
		//隠しファイル表示
		case 100:
			mFileListView_setShowHiddenFiles(p->fdlg.flist, -1);
			break;
	}
}

/* ダイアログ終了 (共通) */

static void _dialog_end(mFileDialog *p,mlkbool ret)
{
	mFileListView *flv = p->fdlg.flist;
	mToplevelSaveState st;
	int f = 0;

	//---- 設定を保存

	//ソートタイプ

	MLKAPP->filedlgconf.sort_type = flv->flv.sort_type;

	//フラグ

	if(flv->flv.sort_rev)
		f |= MAPPBASE_FILEDIALOGCONF_F_SORT_REV;

	if(flv->flv.fstyle & MFILELISTVIEW_S_SHOW_HIDDEN_FILES)
		f |= MAPPBASE_FILEDIALOGCONF_F_SHOW_HIDDEN_FILES;

	MLKAPP->filedlgconf.flags = f;

	//ウィンドウサイズ

	mToplevelGetSaveState(MLK_TOPLEVEL(p), &st);

	MLKAPP->filedlgconf.width = st.w;
	MLKAPP->filedlgconf.height = st.h;

	//----- 終了

	//翻訳グループを元に戻す

	mGuiTransRestoreGroup();

	//

	mDialogEnd(MLK_DIALOG(p), ret);
}

/* 終了 (キャンセル) */

static void _end_cancel(mFileDialog *p)
{
	if(p->fdlg.on_okcancel)
	{
		if(!(p->fdlg.on_okcancel)(p, FALSE, NULL))
			return;
	}

	_dialog_end(p, FALSE);
}

/* OK 終了時 (保存)
 *
 * str: 結果のファイル名をセット
 * str2: 作業用
 * return: FALSE でキャンセル */

static mlkbool _end_ok_save(mFileDialog *p,mStr *str,mStr *str2)
{
	const char *pc,*end,*pctop;
	int flag;
	
	//str2 = 入力ファイル名
	
	mLineEditGetTextStr(p->fdlg.edit_name, str2);

	if(mStrIsEmpty(str2)) return FALSE;

	//拡張子
	// pctop = 元のフィルタ文字列の拡張子部分 (NULL の場合 '*' として扱う)

	pctop = (const char *)mComboBoxGetItemParam(p->fdlg.cbtype, -1);

	if(pctop)
	{
		//str = 入力ファイル名の拡張子

		mStrPathGetExt(str, str2->buf);

		//拡張子がフィルタ文字列のいずれかに一致した場合はそのまま。
		//それ以外の場合、先頭の拡張子を追加する。

		flag = TRUE;

		if(mStrIsnotEmpty(str))
		{
			pc = pctop;

			while(mStringGetNextSplit(&pc, &end, ';'))
			{
				if(strncasecmp(str->buf, pc, end - pc) == 0)
				{
					flag = FALSE;
					break;
				}

				pc = end;
			}
		}

		//拡張子追加

		if(flag)
		{
			pc = pctop;
			
			if(mStringGetNextSplit(&pc, &end, ';'))
			{
				mStrAppendChar(str2, '.');
				mStrAppendText_len(str2, pc, end - pc);
			}
		}
	}

	//ファイル名として無効な文字が含まれている

	if(!mStringIsValidFilename(str2->buf))
	{
		mMessageBoxErrTr(MLK_WINDOW(p), MLK_TRGROUP_ID_SYSTEM, MLK_TRSYS_MES_FILENAME_INVALID);
		return FALSE;
	}

	//str = 保存ファイルパス

	mStrCopy(str, &(p->fdlg.flist)->flv.strDir);
	mStrPathJoin(str, str2->buf);

	//ファイルが存在する場合、上書き確認

	if(!(p->fdlg.fstyle & MFILEDIALOG_S_NO_OVERWRITE_MES)
		&& mIsExistFile(str->buf))
	{
		mStrAppendText(str2, "\n\n");
		mStrAppendText(str2, MLK_TR_SYS(MLK_TRSYS_MES_OVERWRITE_FILE));
	
		if(mMessageBox(MLK_WINDOW(p), NULL, str2->buf,
			MMESBOX_YES | MMESBOX_NO,
			MMESBOX_NO) != MMESBOX_YES)
		{
			return FALSE;
		}
	}

	//ファイル種類のインデックスをセット

	if(p->fdlg.dst_filtertype)
		*(p->fdlg.dst_filtertype) = mComboBoxGetSelIndex(p->fdlg.cbtype);

	return TRUE;
}

/* 終了 (OK) */

static void _end_ok(mFileDialog *p)
{
	mStr str = MSTR_INIT,str2 = MSTR_INIT;
	int n,ret;

	//str に結果文字列をセットする。
	//ret = FALSE でキャンセル。

	ret = FALSE;

	switch(p->fdlg.type)
	{
		//ファイル開く
		case MFILEDIALOG_TYPE_OPENFILE:
			if(p->fdlg.fstyle & MFILEDIALOG_S_MULTI_SEL)
			{
				//複数選択
				n = mFileListView_getSelectMultiName(p->fdlg.flist, &str2);
				if(n == MFILELISTVIEW_FILETYPE_NONE) break;
			}
			else
			{
				//単体選択
				n = mFileListView_getSelectFileName(p->fdlg.flist, &str2, TRUE);
				if(n != MFILELISTVIEW_FILETYPE_FILE) break;
			}

			mStrCopy(&str, &str2);
			ret = TRUE;
			break;

		//保存
		case MFILEDIALOG_TYPE_SAVEFILE:
			ret = _end_ok_save(p, &str, &str2);
			break;

		//ディレクトリ選択
		case MFILEDIALOG_TYPE_DIR:
			n = mFileListView_getSelectFileName(p->fdlg.flist, &str, TRUE);

			//リスト内のディレクトリが選択されていない場合、現在のパスをセット
			if(n != MFILELISTVIEW_FILETYPE_DIR)
				mStrCopy(&str, &(p->fdlg.flist)->flv.strDir);

			ret = TRUE;
			break;
	}

	mStrFree(&str2);

	//キャンセル

	if(!ret)
	{
		mStrFree(&str);
		return;
	}

	//ハンドラ

	if(p->fdlg.on_okcancel)
	{
		if(!(p->fdlg.on_okcancel)(p, TRUE, str.buf))
		{
			mStrFree(&str);
			return;
		}
	}

	//結果のパスをセット

	mStrCopy(p->fdlg.strdst, &str);
	mStrFree(&str);

	//ダイアログ終了

	_dialog_end(p, TRUE);
}

/* 通知イベント */

static void _event_notify(mFileDialog *p,mEventNotify *ev)
{
	mWidget *from;
	int type;

	from = ev->widget_from;
	type = ev->notify_type;

	if(from == (mWidget *)p->fdlg.flist)
	{
		//ファイルリスト操作

		switch(type)
		{
			case MFILELISTVIEW_N_SELECT_FILE:
				_change_selfile(p);
				break;
			case MFILELISTVIEW_N_CHANGE_DIR:
				_change_dir(p);
				break;
			case MFILELISTVIEW_N_DBLCLK_FILE:
				_end_ok(p);
				break;
		}
	}
	else if(from == (mWidget *)p->fdlg.edit_dir)
	{
		//ディレクトリエディット

		if(type == MLINEEDIT_N_ENTER)
			_dir_enter(p);
	}
	else if(from == (mWidget *)p->fdlg.cbtype)
	{
		//種類

		if(type == MCOMBOBOX_N_CHANGE_SEL)
		{
			_set_filter_to_list(p);

			mFileListView_updateList(p->fdlg.flist);

			//種類変更時ハンドラ

			if(p->fdlg.on_changetype)
				(p->fdlg.on_changetype)(p, mComboBoxGetSelIndex(p->fdlg.cbtype));
		}
	}
	else if(from == p->fdlg.btt_moveup)
	{
		//ディレクトリ親へ

		mFileListView_changeDir_parent(p->fdlg.flist);
	}
	else if(from == p->fdlg.btt_menu)
	{
		//メニューボタン

		_run_menu(p);
	}
	else if(ev->id == MLK_WID_OK)
		//OK
		_end_ok(p);
	else if(ev->id == MLK_WID_CANCEL)
		//キャンセル
		_end_cancel(p);
}

/**@ event ハンドラ関数 */

int mFileDialogHandle_event(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
		//通知
		_event_notify(MLK_FILEDIALOG(wg), (mEventNotify *)ev);
	else if(ev->type == MEVENT_CLOSE)
	{
		//ウィンドウ閉じるボタン
		
		_end_cancel(MLK_FILEDIALOG(wg));
		return TRUE;
	}

	return mDialogEventDefault(wg, ev);
}

