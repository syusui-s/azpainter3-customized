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
 * フォントファイルの選択ダイアログ
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_font.h"
#include "mlk_str.h"

#include "mlk_filedialog.h"
#include "mlk_combobox.h"


//-------------------

typedef struct
{
	MLK_FILEDIALOG_DEF

	int *dst_index;
	mComboBox *cb_name;
}_dialog;

//-------------------


//******************************
// ダイアログ
//******************************


/* フォント名列挙関数 */

static void _func_collection_name(int index,const char *name,void *param)
{
	mStr str = MSTR_INIT;

	if(index >> 16)
		//バリアブルフォント
		mStrSetFormat(&str, "v%d: %s", (index >> 16) - 1, name);
	else
		mStrSetFormat(&str, "%d: %s", index, name);

	mComboBoxAddItem_copy((mComboBox *)param, str.buf, index);

	mStrFree(&str);
}

/* ファイル選択時 */

static void _on_selectfile(mFileDialog *fdlg,const char *path)
{
	_dialog *p = (_dialog *)fdlg;

	mComboBoxDeleteAllItem(p->cb_name);

	mFontGetListNames(mGuiGetFontSystem(), path, _func_collection_name, p->cb_name);

	mComboBoxSetSelItem_atIndex(p->cb_name, 0);
}

/* OK/キャンセル時 */

mlkbool _on_okcancel(mFileDialog *fdlg,mlkbool is_ok,const char *path)
{
	_dialog *p = (_dialog *)fdlg;

	if(is_ok)
	{
		//フォントファイルでない
		if(mComboBoxGetItemNum(p->cb_name) == 0) return FALSE;
	
		//選択がない場合 0
		*(p->dst_index) = mComboBoxGetItemParam(p->cb_name, -1);
	}

	return TRUE;
}


//******************************
// mSysDlg
//******************************


/**@ フォントファイルの選択ダイアログ
 *
 * @d:フォントファイルを直接指定して使う場合。
 *
 * @p:pindex ファイル内に複数のフォントが含まれる場合、選択されたインデックス番号が入る */

mlkbool mSysDlg_openFontFile(mWindow *parent,const char *filter,int def_filter,
	const char *initdir,mStr *strdst,int *pindex)
{
	_dialog *p;

	p = (_dialog *)mFileDialogNew(parent, sizeof(_dialog), 0, MFILEDIALOG_TYPE_OPENFILE);
	if(!p) return FALSE;

	p->fdlg.on_selectfile = _on_selectfile;
	p->fdlg.on_okcancel = _on_okcancel;

	p->dst_index = pindex;
	p->cb_name = mComboBoxCreate(MLK_WIDGET(p), 0, MLF_EXPAND_W, MLK_MAKE32_4(0,10,0,0), 0);

	//

	mFileDialogInit(MLK_FILEDIALOG(p), filter, def_filter, initdir, strdst);

	mFileDialogShow(MLK_FILEDIALOG(p));

	return mDialogRun(MLK_DIALOG(p), TRUE);
}

