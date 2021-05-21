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
 * mMessageBox
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_sysdlg.h"
#include "mlk_label.h"
#include "mlk_button.h"
#include "mlk_checkbutton.h"
#include "mlk_str.h"
#include "mlk_event.h"
#include "mlk_key.h"

#include "mlk_pv_window.h"


//------------------------

#define _MESBOX(p)  ((mMesBox *)(p))

typedef struct
{
	MLK_DIALOG_DEF

	mCheckButton *ck_notshow;
	mWidget *wg_press;	//ショートカットキーが押された時、対応するボタンウィジェット
	int press_key;	//押されたショートカットキーの rawcode
	uint32_t fbtts;	//表示するボタンのフラグ
}mMesBox;

//各ボタンに対応するショートカットキー
//[!] ボタンのフラグは、24bit 未満であること

static const uint32_t g_sckey_val[] = {
	('O' << 24) | MMESBOX_OK,
	('C' << 24) | MMESBOX_CANCEL,
	('Y' << 24) | MMESBOX_YES,
	('N' << 24) | MMESBOX_NO,
	('S' << 24) | MMESBOX_SAVE,
	('U' << 24) | MMESBOX_NOTSAVE,
	('A' << 24) | MMESBOX_ABORT,
	0
};

//------------------------

/* ショートカットキーが押されている状態で、
 * ポインタ操作により別のボタンが押された場合、ポインタ操作の方が優先される。
 */


//=================================
// mMesBox ダイアログ
//=================================


/** ダイアログ終了 */

static void _end_dialog(mMesBox *p,uint32_t btt)
{
	if(p->ck_notshow
		&& mCheckButtonIsChecked(p->ck_notshow))
		btt |= MMESBOX_NOTSHOW;

	mDialogEnd(MLK_DIALOG(p), btt);
}

/** キー押し時 */

static void _event_keydown(mMesBox *p,uint32_t key,int rawcode)
{
	mWidget *wg;
	const uint32_t *pv;
	uint32_t btt = 0;

	//小文字は大文字にする

	if(key >= 'a' && key <= 'z') key -= 0x20;

	//A-Z のみ

	if(key < 'A' || key > 'Z') return;

	//キーに対応するボタンフラグを取得

	for(pv = g_sckey_val; *pv; pv++)
	{
		if(key == (*pv >> 24))
		{
			btt = *pv & 0xffffff;
			break;
		}
	}

	//ボタンを、押された状態にする

	if(p->fbtts & btt)
	{
		wg = mWidgetFindFromID(MLK_WIDGET(p), btt);

		p->wg_press = wg;
		p->press_key = rawcode;

		mButtonSetState(MLK_BUTTON(wg), 1);
	}
}

/** イベントハンドラ */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	mMesBox *p = _MESBOX(wg);

	switch(ev->type)
	{
		//ボタン押し
		case MEVENT_NOTIFY:
			if(ev->notify.id != MMESBOX_NOTSHOW)
				_end_dialog(p, ev->notify.id);
			break;

		//キー押し (ショートカットキー処理)
		case MEVENT_KEYDOWN:
			if(!ev->key.is_grab_pointer && !p->wg_press)
				_event_keydown(p, ev->key.key, ev->key.raw_code);
			break;

		//キー離し
		//(ショートカットキーが離された場合)
		case MEVENT_KEYUP:
			if(p->wg_press && p->press_key == ev->key.raw_code)
				_end_dialog(p, p->wg_press->id);
			break;

		//フォーカスアウト
		//(ショートカットキー押し時は解除)
		case MEVENT_FOCUS:
			if(ev->focus.is_out && p->wg_press)
			{
				mButtonSetState(MLK_BUTTON(p->wg_press), 0);
				p->wg_press = NULL;
			}
			break;
	}

	return mDialogEventDefault(wg, ev);
}

/** 各ボタンのイベント */

static int _button_event_handle(mWidget *wg,mEvent *ev)
{
	if((ev->type == MEVENT_KEYDOWN || ev->type == MEVENT_KEYUP)
		&& mKeyIsAlphabet(ev->key.key))
	{
		//[キー押し/離し]
		//アルファベットのキーはショートカットキーとして扱うので、
		//ウィンドウへ送る
	
		_event_handle(MLK_WIDGET(wg->toplevel), ev);
		return 1;
	}

	return mButtonHandle_event(wg, ev);
}


//-----------------


/** ボタン作成 */

static void _create_btt(mWidget *parent,int wgid,uint16_t strid,char key)
{
	mStr str = MSTR_INIT;
	mWidget *wg;

	mStrSetFormat(&str, "%s(%c)", MLK_TR_SYS(strid), key);

	wg = (mWidget *)mButtonCreate(parent, wgid, 0, 0, MBUTTON_S_COPYTEXT, str.buf);
	
	wg->event = _button_event_handle;
	wg->hintMinW = 80;
	wg->hintMinH = 25;

	mStrFree(&str);
}

/** ウィジェット作成 */

static void _create_widget(mMesBox *p,const char *message,uint32_t defbtt)
{
	mWidget *parent,*wg;
	uint32_t bt;

	parent = MLK_WIDGET(p);
	bt = p->fbtts;

	//メッセージ

	mLabelCreate(parent, MLF_EXPAND_XY | MLF_CENTER_XY, 0, 0, message);

	//"このメッセージを表示しない"

	if(bt & MMESBOX_NOTSHOW)
	{
		p->ck_notshow = mCheckButtonCreate(parent, MMESBOX_NOTSHOW,
			MLF_CENTER, MLK_MAKE32_4(0,4,0,0), 0,
			MLK_TR_SYS(MLK_TRSYS_NOTSHOW_THIS_MES), FALSE);
	}

	//ボタン

	wg = mContainerCreateHorz(parent, 5, MLF_EXPAND_X | MLF_CENTER, 0);
	wg->margin.top = 6;

	if(bt & MMESBOX_SAVE) _create_btt(wg, MMESBOX_SAVE, MLK_TRSYS_SAVE, 'S');
	if(bt & MMESBOX_NOTSAVE) _create_btt(wg, MMESBOX_NOTSAVE, MLK_TRSYS_NOTSAVE, 'U');
	if(bt & MMESBOX_YES) _create_btt(wg, MMESBOX_YES, MLK_TRSYS_YES, 'Y');
	if(bt & MMESBOX_NO) _create_btt(wg, MMESBOX_NO, MLK_TRSYS_NO, 'N');
	if(bt & MMESBOX_OK) _create_btt(wg, MMESBOX_OK, MLK_TRSYS_OK, 'O');
	if(bt & MMESBOX_CANCEL) _create_btt(wg, MMESBOX_CANCEL, MLK_TRSYS_CANCEL, 'C');
	if(bt & MMESBOX_ABORT) _create_btt(wg, MMESBOX_ABORT, MLK_TRSYS_ABORT, 'A');

	//デフォルトボタンにフォーカスセット

	if(defbtt)
	{
		wg = mWidgetFindFromID(wg, defbtt);
		if(wg)
			mWidgetSetFocus(wg);
	}
}

/** ダイアログ作成 */

static mMesBox *_mesbox_new(mWindow *parent,const char *title,const char *message,
	uint32_t btts,uint32_t defbtt)
{
	mMesBox *p;
	
	p = (mMesBox *)mDialogNew(parent, sizeof(mMesBox),
			MTOPLEVEL_S_DIALOG_NORMAL | MTOPLEVEL_S_NO_INPUT_METHOD);

	if(!p) return NULL;
	
	p->wg.event = _event_handle;
	p->ct.sep = 10;
	p->fbtts = btts;

	mContainerSetPadding_same(MLK_CONTAINER(p), 10);

	mToplevelSetTitle(MLK_TOPLEVEL(p), title);

	//ウィジェット作成

	_create_widget(p, message, defbtt);

	//表示

	mWindowResizeShow_initSize(MLK_WINDOW(p));
	
	return p;
}


//=================================
// mMessageBox 関数
//=================================


/**@ メッセージボックスの実行
 *
 * @p:title ウィンドウのタイトルバーのテキスト
 * @p:message 表示するテキスト
 * @p:btts 表示するボタンを、フラグで複数指定
 * @p:defbtt btts の中から、デフォルトのボタンを一つ指定。0 でなし。
 * @r:押されたボタンのフラグが返る。\
 *  チェックボタンが ON になっている場合、ボタンに加えて、そのフラグも追加される。\
 *  0 で、閉じるボタンや ESC キーによってキャンセルされた。 */

uint32_t mMessageBox(mWindow *parent,const char *title,const char *message,
	uint32_t btts,uint32_t defbtt)
{
	mMesBox *p;

	p = _mesbox_new(parent, title, message, btts, defbtt);
	if(!p) return 0;

	return mDialogRun(MLK_DIALOG(p), TRUE);
}

/**@ メッセージボックス:OK ボタンのみ
 *
 * @d:タイトルは "message" で、OK ボタンのみ表示 */

void mMessageBoxOK(mWindow *parent,const char *message)
{
	mMessageBox(parent, "message", message, MMESBOX_OK, MMESBOX_OK);
}

/**@ メッセージボックス:エラー表示
 *
 * @d:タイトルは "error" で、OK ボタンのみ表示 */

void mMessageBoxErr(mWindow *parent,const char *message)
{
	mMessageBox(parent, "error", message, MMESBOX_OK, MMESBOX_OK);
}

/**@ メッセージボックス:エラー表示 (翻訳文字列から) */

void mMessageBoxErrTr(mWindow *parent,uint16_t groupid,uint16_t strid)
{
	mMessageBox(parent, "error", MLK_TR2(groupid, strid), MMESBOX_OK, MMESBOX_OK);
}

