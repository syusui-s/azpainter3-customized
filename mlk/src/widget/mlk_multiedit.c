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
 * mMultiEdit
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_multiedit.h"
#include "mlk_scrollbar.h"
#include "mlk_event.h"
#include "mlk_str.h"
#include "mlk_unicode.h"
#include "mlk_clipboard.h"

#include "mlk_pv_widget.h"
#include "mlk_multieditpage.h"
#include "mlk_widgettextedit.h"



/* construct ハンドラ関数
 *
 * テキストが変化した時、スクロールバーを再構成 */

static void _construct_handle(mWidget *wg)
{
	mMultiEdit *p = MLK_MULTIEDIT(wg);

	mMultiEditPage_setScrollInfo(MLK_MULTIEDITPAGE(p->sv.page));
	
	mScrollViewLayout(MLK_SCROLLVIEW(p));
	mWidgetRedraw(MLK_WIDGET(p->sv.page));
}

/* IM 位置取得ハンドラ */

static void _get_inputmethod_pos_handle(mWidget *wg,mPoint *dst)
{
	mMultiEditPage_getIMPos((mMultiEditPage *)MLK_MULTIEDIT(wg)->sv.page, dst);
}


//===========================
// main
//===========================


/**@ mMultiEdit データ解放 */

void mMultiEditDestroy(mWidget *wg)
{
	mWidgetTextEdit_free(&MLK_MULTIEDIT(wg)->me.edit);
}

/**@ 作成 */

mMultiEdit *mMultiEditNew(mWidget *parent,int size,uint32_t fstyle)
{
	mMultiEdit *p;
	
	if(size < sizeof(mMultiEdit))
		size = sizeof(mMultiEdit);
	
	p = (mMultiEdit *)mScrollViewNew(parent, size, MSCROLLVIEW_S_HORZVERT | MSCROLLVIEW_S_FRAME);
	if(!p) return NULL;
	
	p->wg.destroy = mMultiEditDestroy;
	p->wg.event = mMultiEditHandle_event;
	p->wg.construct = _construct_handle;
	p->wg.get_inputmethod_pos = _get_inputmethod_pos_handle;
	
	p->wg.fstate |= MWIDGET_STATE_TAKE_FOCUS | MWIDGET_STATE_ENABLE_KEY_REPEAT | MWIDGET_STATE_ENABLE_INPUT_METHOD;
	p->wg.fevent |= MWIDGET_EVENT_KEY | MWIDGET_EVENT_CHAR | MWIDGET_EVENT_STRING;
	p->wg.facceptkey = MWIDGET_ACCEPTKEY_ENTER;

	p->me.fstyle = fstyle;

	//mWidgetTextEdit
	
	mWidgetTextEdit_init(&p->me.edit, MLK_WIDGET(p), TRUE);

	//mMultiEditPage

	p->sv.page = (mScrollViewPage *)mMultiEditPageNew(MLK_WIDGET(p), 0, &p->me.edit);
	
	return p;
}

/**@ 作成 */

mMultiEdit *mMultiEditCreate(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack,uint32_t fstyle)
{
	mMultiEdit *p;

	p = mMultiEditNew(parent, 0, fstyle);
	if(p)
		__mWidgetCreateInit(MLK_WIDGET(p), id, flayout, margin_pack);

	return p;
}

/**@ テキストセット */

void mMultiEditSetText(mMultiEdit *p,const char *text)
{
	mWidgetTextEdit_setText(&p->me.edit, text);

	//スクロール位置リセット

	mScrollBarSetPos(p->sv.scrh, 0);
	mScrollBarSetPos(p->sv.scrv, 0);

	mMultiEditPage_onChangeCursorPos(MLK_MULTIEDITPAGE(p->sv.page));

	mWidgetSetConstruct(MLK_WIDGET(p));
}

/**@ テキストを取得 */

void mMultiEditGetTextStr(mMultiEdit *p,mStr *str)
{
	mStrSetText_utf32(str, p->me.edit.unibuf, p->me.edit.textlen);
}

/**@ テキストの長さを取得 */

int mMultiEditGetTextLen(mMultiEdit *p)
{
	return p->me.edit.textlen;
}

/**@ テキストを全て選択 */

void mMultiEditSelectAll(mMultiEdit *p)
{
	if(mWidgetTextEdit_selectAll(&p->me.edit))
		mWidgetRedraw(MLK_WIDGET(p->sv.page));
}

/**@ 現在のカーソル位置にテキストを挿入
 *
 * @d:テキスト変更時は通知が送られる。 */

void mMultiEditInsertText(mMultiEdit *p,const char *text)
{
	if(mWidgetTextEdit_insertText(&p->me.edit, text, 0))
		mMultiEditPage_onChangeText(MLK_MULTIEDITPAGE(p->sv.page));
}

/**@ すべてのテキストをクリップボードにコピー
 *
 * @r:コピーしたか */

mlkbool mMultiEditCopyText_full(mMultiEdit *p)
{
	char *utf8;
	int len;

	if(p->me.edit.textlen)
	{
		utf8 = mUTF32toUTF8_alloc(p->me.edit.unibuf, p->me.edit.textlen, &len);

		if(utf8)
		{
			mClipboardSetText(utf8, len);
			
			mFree(utf8);

			return TRUE;
		}
	}

	return FALSE;
}

/**@ 内容を消去して、クリップボードから貼り付け
 *
 * @d:テキストが取得できなかった場合、何もしない。
 *
 * @r:貼り付けられたか */

mlkbool mMultiEditPaste_full(mMultiEdit *p)
{
	mStr str = MSTR_INIT;

	if(mClipboardGetText(&str))
		return FALSE;
	else
	{
		mMultiEditSetText(p, str.buf);
	
		mStrFree(&str);

		return TRUE;
	}
}


//========================
// ハンドラ
//========================


/**@ event ハンドラ関数 */

int mMultiEditHandle_event(mWidget *wg,mEvent *ev)
{
	mMultiEdit *p = MLK_MULTIEDIT(wg);

	switch(ev->type)
	{
		//キーイベントは mMultiEditPage へ送る
		case MEVENT_KEYDOWN:
		case MEVENT_CHAR:
		case MEVENT_STRING:
			((p->sv.page)->wg.event)(MLK_WIDGET(p->sv.page), ev);
			break;

		//フォーカス
		case MEVENT_FOCUS:
			if(ev->focus.is_out)
			{
				//[out] (選択解除)
				//ウィンドウによるフォーカス変更時は選択を残す

				if(ev->focus.from != MEVENT_FOCUS_FROM_WINDOW_FOCUS
					&& ev->focus.from != MEVENT_FOCUS_FROM_FORCE_UNGRAB)
					p->me.edit.seltop = -1;
			}
			else
			{
				//[in]
				//ウィンドウのフォーカス変更 (復元時は除く)、
				//または、キーによるフォーカス移動の場合は、全選択
				
				if(ev->focus.from == MEVENT_FOCUS_FROM_WINDOW_FOCUS
					|| ev->focus.from == MEVENT_FOCUS_FROM_KEY_MOVE)
					mWidgetTextEdit_selectAll(&p->me.edit);
			}
			
			mWidgetRedraw(MLK_WIDGET(p->sv.page));
			break;
		default:
			return FALSE;
	}

	return TRUE;
}

