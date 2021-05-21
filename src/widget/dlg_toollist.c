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
 * ツールリスト関連のダイアログ
 *
 * [グループ設定] [ブラシサイズ設定]
 * [ツールアイテムの設定表示]
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_label.h"
#include "mlk_lineedit.h"
#include "mlk_event.h"
#include "mlk_sysdlg.h"
#include "mlk_str.h"
#include "mlk_string.h"

#include "def_tool.h"
#include "def_tool_option.h"

#include "toollist.h"

#include "widget_func.h"
#include "dlg_toollist.h"

#include "trid.h"
#include "trid_tooloption.h"


//------------------

enum
{
	TRID_TITLE_NEW_GROUP,
	TRID_TITLE_EDIT_GROUP,
	TRID_TITLE_BRUSHSIZE_OPT,
	TRID_TITLE_TOOLOPTVIEW,

	TRID_COLUMN_NUM = 100,
	TRID_MIN,
	TRID_MAX
};

//------------------


//************************************
// グループ設定
//************************************


typedef struct
{
	MLK_DIALOG_DEF

	mLineEdit *edit_name,
		*edit_col;
}_dlg_groupopt;


/* ダイアログ作成 */

static _dlg_groupopt *_create_groupopt(mWindow *parent,ToolListDlg_groupOpt *dst,mlkbool is_edit)
{
	_dlg_groupopt *p;

	MLK_TRGROUP(TRGROUP_DLG_TOOLLIST);

	p = (_dlg_groupopt *)widget_createDialog(parent, sizeof(_dlg_groupopt),
		MLK_TR(is_edit? TRID_TITLE_EDIT_GROUP: TRID_TITLE_NEW_GROUP),
		mDialogEventDefault_okcancel);

	if(!p) return NULL;

	mContainerSetType_vert(MLK_CONTAINER(p), 6);

	//---- 名前

	mLabelCreate(MLK_WIDGET(p), 0, 0, 0, MLK_TR2(TRGROUP_WORD, TRID_WORD_NAME));

	p->edit_name = mLineEditCreate(MLK_WIDGET(p), 0, MLF_EXPAND_W, 0, 0);

	mLineEditSetText(p->edit_name, dst->strname.buf);

	mWidgetSetInitSize_fontHeightUnit(MLK_WIDGET(p->edit_name), 20, 0);

	//---- 横に並べる数

	mLabelCreate(MLK_WIDGET(p), 0, 0, 0, MLK_TR(TRID_COLUMN_NUM));

	p->edit_col = widget_createEdit_num(MLK_WIDGET(p), 5, 1, 10, 0, dst->colnum);

	//-----

	mContainerCreateButtons_okcancel(MLK_WIDGET(p), MLK_MAKE32_4(0,10,0,0));

	return p;
}

/** グループ設定ダイアログ実行
 *
 * dst: 初期値を入れておく
 * is_edit: TRUE で編集、FALSE で新規 */

mlkbool ToolListDialog_groupOption(mWindow *parent,ToolListDlg_groupOpt *dst,mlkbool is_edit)
{
	_dlg_groupopt *p;
	int ret;

	p = _create_groupopt(parent, dst, is_edit);
	if(!p) return FALSE;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	ret = mDialogRun(MLK_DIALOG(p), FALSE);

	if(ret)
	{
		mLineEditGetTextStr(p->edit_name, &dst->strname);

		//文字列サイズを制限 (ファイル保存時にサイズが大きすぎないように)
		mStrSetLen_bytes(&dst->strname, 2048);

		dst->colnum = mLineEditGetNum(p->edit_col);
	}

	mWidgetDestroy(MLK_WIDGET(p));

	return ret;
}


//************************************
// ブラシサイズ設定
//************************************


typedef struct
{
	MLK_DIALOG_DEF

	mLineEdit *edit[2];
}_dlg_sizeopt;


/* ダイアログ作成 */

static _dlg_sizeopt *_create_sizeopt(mWindow *parent,int minval,int maxval)
{
	_dlg_sizeopt *p;
	mWidget *ct;
	int i;

	MLK_TRGROUP(TRGROUP_DLG_TOOLLIST);

	p = (_dlg_sizeopt *)widget_createDialog(parent, sizeof(_dlg_sizeopt),
		MLK_TR(TRID_TITLE_BRUSHSIZE_OPT), mDialogEventDefault_okcancel);

	if(!p) return NULL;

	//-----

	ct = mContainerCreateGrid(MLK_WIDGET(p), 2, 6, 7, 0, 0);

	for(i = 0; i < 2; i++)
	{
		mLabelCreate(ct, MLF_MIDDLE, 0, 0, MLK_TR(TRID_MIN + i));

		p->edit[i] = widget_createEdit_num(ct, 8, BRUSH_SIZE_MIN, BRUSH_SIZE_MAX, 1,
			(i == 0)? minval: maxval);
	}

	//-----

	mContainerCreateButtons_okcancel(MLK_WIDGET(p), MLK_MAKE32_4(0,18,0,0));

	return p;
}

/** ブラシサイズ設定ダイアログ実行 */

mlkbool ToolListDialog_brushSizeOpt(mWindow *parent,int *dst_min,int *dst_max)
{
	_dlg_sizeopt *p;
	int ret;

	p = _create_sizeopt(parent, *dst_min, *dst_max);
	if(!p) return FALSE;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	ret = mDialogRun(MLK_DIALOG(p), FALSE);

	if(ret)
	{
		*dst_min = mLineEditGetNum(p->edit[0]);
		*dst_max = mLineEditGetNum(p->edit[1]);
	}

	mWidgetDestroy(MLK_WIDGET(p));

	return ret;
}


//************************************
// ツールアイテムの設定表示
//************************************


/* ドットペン */

static void _toolview_dotpen(mStr *str,uint32_t v)
{
	int n;
	
	//サイズ
	mStrAppendFormat(str, "%s: %d\n",
		MLK_TR2(TRGROUP_WORD, TRID_WORD_SIZE), TOOLOPT_DOTPEN_GET_SIZE(v));

	//濃度
	mStrAppendFormat(str, "%s: %d\n",
		MLK_TR2(TRGROUP_WORD, TRID_WORD_DENSITY), TOOLOPT_DOTPEN_GET_DENSITY(v));

	//塗りタイプ
	n = TOOLOPT_DOTPEN_GET_PIXMODE(v);
	
	mStrAppendFormat(str, "%s: %s\n",
		MLK_TR2(TRGROUP_WORD, TRID_WORD_PIXELMODE), MLK_TR2(TRGROUP_PIXELMODE_BRUSH, n));

	//形状
	n = TOOLOPT_DOTPEN_GET_SHAPE(v);
	
	mStrAppendFormat(str, "%s: %s\n",
		MLK_TR(TRID_TOOLOPT_SHAPE),
		mStringGetNullSplitText(MLK_TR2(TRGROUP_WORD, TRID_WORD_DOTPEN_SHAPE), n));

	//細線
	if(TOOLOPT_DOTPEN_IS_SLIM(v))
		mStrAppendFormat(str, "<%s>", MLK_TR(TRID_TOOLOPT_SLIM));
}

/* 指先 */

static void _toolview_finger(mStr *str,uint32_t v)
{
	//サイズ
	mStrAppendFormat(str, "%s: %d\n",
		MLK_TR2(TRGROUP_WORD, TRID_WORD_SIZE), TOOLOPT_DOTPEN_GET_SIZE(v));

	//強さ
	mStrAppendFormat(str, "%s: %d\n",
		MLK_TR(TRID_TOOLOPT_STRONG), TOOLOPT_DOTPEN_GET_DENSITY(v));
}

/* 図形塗りつぶし */

static void _toolview_fillpoly(mStr *str,uint32_t v,mlkbool erase)
{
	int n;
	
	//濃度
	mStrAppendFormat(str, "%s: %d\n",
		MLK_TR2(TRGROUP_WORD, TRID_WORD_DENSITY), TOOLOPT_FILLPOLY_GET_DENSITY(v));

	//塗りタイプ
	if(!erase)
	{
		n = TOOLOPT_FILLPOLY_GET_PIXMODE(v);
		
		mStrAppendFormat(str, "%s: %s\n",
			MLK_TR2(TRGROUP_WORD, TRID_WORD_PIXELMODE), MLK_TR2(TRGROUP_PIXELMODE_TOOL, n));
	}

	//アンチエイリアス
	if(TOOLOPT_FILLPOLY_IS_ANTIALIAS(v))
		mStrAppendFormat(str, "<%s>", MLK_TR2(TRGROUP_WORD, TRID_WORD_ANTIALIAS));
}

/* 塗りつぶし/自動選択 */

static void _toolview_fill(mStr *str,uint32_t v,mlkbool fill)
{
	int n;

	//塗りつぶす範囲
	n = TOOLOPT_FILL_GET_TYPE(v);

	mStrAppendFormat(str, "%s: %s\n",
		MLK_TR(TRID_TOOLOPT_FILL_TYPE),
		mStringGetNullSplitText(MLK_TR(TRID_TOOLOPT_FILL_TYPE_LIST), n));

	//許容誤差
	mStrAppendFormat(str, "%s: %d\n",
		MLK_TR(TRID_TOOLOPT_FILL_DIFF), TOOLOPT_FILL_GET_DIFF(v));

	//濃度
	if(fill)
	{
		mStrAppendFormat(str, "%s: %d\n",
			MLK_TR2(TRGROUP_WORD, TRID_WORD_DENSITY), TOOLOPT_FILL_GET_DENSITY(v));
	}

	//参照レイヤ
	n = TOOLOPT_FILL_GET_LAYER(v);
	
	mStrAppendFormat(str, "%s: %s\n",
		MLK_TR(TRID_TOOLOPT_FILL_LAYER),
		mStringGetNullSplitText(MLK_TR(TRID_TOOLOPT_FILL_LAYER_LIST), n));

	//塗りタイプ
	if(fill)
	{
		n = TOOLOPT_FILL_GET_PIXMODE(v);
		
		mStrAppendFormat(str, "%s: %s\n",
			MLK_TR2(TRGROUP_WORD, TRID_WORD_PIXELMODE), MLK_TR2(TRGROUP_PIXELMODE_TOOL, n));
	}
}

/* グラデーション */

static void _toolview_grad(mStr *str,uint32_t v)
{
	int n;

	//タイプ
	n = TOOLOPT_GRAD_GET_COLTYPE(v);
	
	mStrAppendFormat(str, "type: %s\n",
		mStringGetNullSplitText(MLK_TR(TRID_TOOLOPT_GRAD_COLTYPE_LIST), n));

	//濃度
	mStrAppendFormat(str, "%s: %d\n",
		MLK_TR2(TRGROUP_WORD, TRID_WORD_DENSITY), TOOLOPT_GRAD_GET_DENSITY(v));

	//塗りタイプ
	n = TOOLOPT_GRAD_GET_PIXMODE(v);
	
	mStrAppendFormat(str, "%s: %s\n",
		MLK_TR2(TRGROUP_WORD, TRID_WORD_PIXELMODE), MLK_TR2(TRGROUP_PIXELMODE_TOOL, n));

	//反転
	if(TOOLOPT_GRAD_IS_REVERSE(v))
		mStrAppendFormat(str, "<%s>\n", MLK_TR(TRID_TOOLOPT_GRAD_REVERSE));

	//繰り返す
	if(TOOLOPT_GRAD_IS_LOOP(v))
		mStrAppendFormat(str, "<%s>\n", MLK_TR(TRID_TOOLOPT_GRAD_LOOP));
}

/** ツールの設定表示 */

void ToolListDialog_toolOptView(mWindow *parent,ToolListItem_tool *item)
{
	mStr str = MSTR_INIT;
	int tool;
	uint32_t opt;

	tool = item->toolno;
	opt = item->toolopt;

	//ツール名

	mStrSetFormat(&str, "[%s]\n", MLK_TR2(TRGROUP_TOOL, tool));

	//サブタイプ

	if(item->subno != 255)
		mStrAppendFormat(&str, "subno: %d\n", item->subno);

	//各ツール

	mStrAppendChar(&str, '\n');

	MLK_TRGROUP(TRGROUP_PANEL_OPTION_TOOL);

	switch(tool)
	{
		case TOOL_DOTPEN:
		case TOOL_DOTPEN_ERASE:
			_toolview_dotpen(&str, opt);
			break;
		case TOOL_FINGER:
			_toolview_finger(&str, opt);
			break;
		case TOOL_FILL_POLYGON:
		case TOOL_FILL_POLYGON_ERASE:
			_toolview_fillpoly(&str, opt, (tool == TOOL_FILL_POLYGON_ERASE));
			break;
		case TOOL_FILL:
		case TOOL_SELECT_FILL:
			_toolview_fill(&str, opt, (tool == TOOL_FILL));
			break;
		case TOOL_GRADATION:
			_toolview_grad(&str, opt);
			break;
	}

	//

	mMessageBox(parent,
		MLK_TR2(TRGROUP_DLG_TOOLLIST, TRID_TITLE_TOOLOPTVIEW),
		str.buf,
		MMESBOX_OK, MMESBOX_OK);

	mStrFree(&str);
}


