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
 * ダイアログ: ほか
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_label.h"
#include "mlk_checkbutton.h"
#include "mlk_colorbutton.h"
#include "mlk_event.h"

#include "def_draw.h"

#include "dialogs.h"
#include "widget_func.h"

#include "trid.h"


//***********************************
// イメージの設定
//***********************************

typedef struct
{
	MLK_DIALOG_DEF

	mCheckButton *ck_8bit;
	mColorButton *colbtt;
}_dlg_imgopt;


/* ダイアログ作成 */

static _dlg_imgopt *_imgopt_create(mWindow *parent)
{
	_dlg_imgopt *p;
	mWidget *ct;

	//作成

	p = (_dlg_imgopt *)widget_createDialog(parent, sizeof(_dlg_imgopt),
		MLK_TR2(TRGROUP_DLG_IMAGEOPT, 0), mDialogEventDefault_okcancel);
	
	if(!p) return NULL;
	
	//------

	ct = mContainerCreateGrid(MLK_WIDGET(p), 2, 7, 8, 0, 0);

	//カラー

	p->ck_8bit = widget_createColorBits(ct, (APPDRAW->imgbits == 8));

	//背景色

	p->colbtt = widget_createLabelColorButton(ct,
		MLK_TR2(TRGROUP_WORD, TRID_WORD_BKGND_COLOR), 0, RGBcombo_to_32bit(&APPDRAW->imgbkcol));

	//

	mContainerCreateButtons_okcancel(MLK_WIDGET(p), MLK_MAKE32_4(0,15,0,0));

	return p;
}

/* OK 時 */

static int _imgopt_ok(_dlg_imgopt *p)
{
	uint32_t col;
	int bits,ret = 0;

	//背景色

	col = mColorButtonGetColor(p->colbtt);

	if(col != RGBcombo_to_32bit(&APPDRAW->imgbkcol))
	{
		RGB32bit_to_RGBcombo(&APPDRAW->imgbkcol, col);

		ret |= IMAGEOPTDLG_F_BKGND_COL;
	}

	//ビット数

	bits = (mCheckButtonIsChecked(p->ck_8bit))? 8: 16;

	if(bits != APPDRAW->imgbits)
		ret |= IMAGEOPTDLG_F_BITS;

	return ret;
}

/** イメージの設定ダイアログ実行 */

int ImageOptionDlg_run(mWindow *parent)
{
	_dlg_imgopt *p;
	int ret;

	p = _imgopt_create(parent);
	if(!p) return FALSE;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	ret = mDialogRun(MLK_DIALOG(p), FALSE);

	if(ret)
		ret = _imgopt_ok(p);

	mWidgetDestroy(MLK_WIDGET(p));

	return ret;
}

