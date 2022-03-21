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
 * mFileInput
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_fileinput.h"
#include "mlk_lineedit.h"
#include "mlk_button.h"
#include "mlk_sysdlg.h"
#include "mlk_event.h"
#include "mlk_str.h"

#include "mlk_pv_widget.h"


//=============================
// main
//=============================


/**@ mFileInput データ解放 */

void mFileInputDestroy(mWidget *wg)
{
	mFileInput *p = MLK_FILEINPUT(wg);

	mStrFree(&p->fi.str_filter);
	mStrFree(&p->fi.str_initdir);
}

/**@ 作成 */

mFileInput *mFileInputNew(mWidget *parent,int size,uint32_t fstyle)
{
	mFileInput *p;
	
	if(size < sizeof(mFileInput))
		size = sizeof(mFileInput);
	
	p = (mFileInput *)mContainerNew(parent, size);
	if(!p) return NULL;

	mContainerSetType_horz(MLK_CONTAINER(p), 5);

	p->wg.destroy = mFileInputDestroy;
	p->wg.event = mFileInputHandle_event;
	p->wg.flayout = MLF_EXPAND_W;

	p->fi.fstyle = fstyle;
	
	//mLineEdit

	p->fi.edit = mLineEditCreate(MLK_WIDGET(p), 0, MLF_EXPAND_W | MLF_MIDDLE, 0,
		(fstyle & MFILEINPUT_S_READ_ONLY)? MLINEEDIT_S_READ_ONLY: 0);

	(p->fi.edit)->wg.notify_to = MWIDGET_NOTIFYTO_PARENT;

	//mButton

	p->fi.btt = mButtonCreate(MLK_WIDGET(p), 0, MLF_MIDDLE, 0, MBUTTON_S_REAL_W, "...");

	(p->fi.btt)->wg.notify_to = MWIDGET_NOTIFYTO_PARENT;
	
	return p;
}

/**@ 作成 (ファイル扱い) */

mFileInput *mFileInputCreate_file(mWidget *parent,int id,
	uint32_t flayout,uint32_t margin_pack,uint32_t fstyle,
	const char *filter,int filter_def,const char *initdir)
{
	mFileInput *p;

	p = mFileInputNew(parent, 0, fstyle);
	if(p)
	{
		__mWidgetCreateInit(MLK_WIDGET(p), id, flayout, margin_pack);

		mFileInputSetFilter(p, filter, filter_def);
		mFileInputSetInitDir(p, initdir);
	}

	return p;
}

/**@ 作成 (ディレクトリ扱い) */

mFileInput *mFileInputCreate_dir(mWidget *parent,int id,
	uint32_t flayout,uint32_t margin_pack,uint32_t fstyle,const char *path)
{
	mFileInput *p;

	p = mFileInputNew(parent, 0, fstyle | MFILEINPUT_S_DIRECTORY);
	if(p)
	{
		__mWidgetCreateInit(MLK_WIDGET(p), id, flayout, margin_pack);

		mLineEditSetText(p->fi.edit, path);
	}

	return p;
}


/**@ ファイルのフィルタをセット */

void mFileInputSetFilter(mFileInput *p,const char *filter,int def)
{
	mStrSetText(&p->fi.str_filter, filter);

	p->fi.filter_def = def;
}

/**@ ファイルの初期ディレクトリをセット
 *
 * @d:空の場合、セットされているファイル名と同じディレクトリになる。 */

void mFileInputSetInitDir(mFileInput *p,const char *dir)
{
	mStrSetText(&p->fi.str_initdir, dir);
}

/**@ パス名セット */

void mFileInputSetPath(mFileInput *p,const char *path)
{
	mLineEditSetText(p->fi.edit, path);
}

/**@ パス名取得 */

void mFileInputGetPath(mFileInput *p,mStr *str)
{
	mLineEditGetTextStr(p->fi.edit, str);
}


//=======================


/* ファイル参照 */

static void _select_file(mFileInput *p)
{
	mStr str = MSTR_INIT;
	int ret;

	mLineEditGetTextStr(p->fi.edit, &str);

	if(p->fi.fstyle & MFILEINPUT_S_DIRECTORY)
	{
		//ディレクトリ

		ret = mSysDlg_selectDir(p->wg.toplevel, str.buf, 0, &str);
	}
	else
	{
		//ファイル
		
		ret = mSysDlg_openFile(p->wg.toplevel,
			p->fi.str_filter.buf, p->fi.filter_def, p->fi.str_initdir.buf,
			0, &str);
	}

	if(ret)
	{
		mLineEditSetText(p->fi.edit, str.buf);

		//通知

		mWidgetEventAdd_notify(MLK_WIDGET(p), NULL,
			MFILEINPUT_N_CHANGE, 0, 0);
	}

	mStrFree(&str);
}

/**@ event ハンドラ関数
 *
 * @d:参照ボタンが押された時、ファイルを選択。 */

int mFileInputHandle_event(mWidget *wg,mEvent *ev)
{
	mFileInput *p = MLK_FILEINPUT(wg);

	switch(ev->type)
	{
		case MEVENT_NOTIFY:
			//ファイル参照
			if(ev->notify.widget_from == (mWidget *)p->fi.btt)
				_select_file(p);
			break;
		
		default:
			return FALSE;
	}

	return TRUE;
}
