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
 * mAccelerator
 *****************************************/

#include "mlk_gui.h"
#include "mlk_accelerator.h"
#include "mlk_list.h"
#include "mlk_event.h"
#include "mlk_str.h"
#include "mlk_key.h"

#include "mlk_pv_gui.h"


//-------------------------

struct _mAccelerator
{
	mList list;
	mWidget *widget_def;	//デフォルトのイベント送り先
};

typedef struct
{
	mListItem i;
	
	int cmdid;
	uint32_t key;
	mWidget *widget;
}_item;

#define _ITEM(p)  ((_item *)(p))

//-------------------------



/** キーがアルファベットの場合、大文字に変換 */

static uint32_t _conv_key(uint32_t key)
{
	uint32_t k;

	k = key & MLK_ACCELKEY_MASK_KEY;

	if(k >= 'a' && k <= 'z')
		return (k - 0x20) | (key & MLK_ACCELKEY_MASK_STATE);
	else
		return key;
}

/** キーイベントの state をアクセラレータキー用に変換 */

static uint32_t _conv_state(uint32_t state)
{
	uint32_t ret = 0;

	if(state & MLK_STATE_SHIFT) ret |= MLK_ACCELKEY_SHIFT;
	if(state & MLK_STATE_CTRL)  ret |= MLK_ACCELKEY_CTRL;
	if(state & MLK_STATE_ALT)   ret |= MLK_ACCELKEY_ALT;
	if(state & MLK_STATE_LOGO)  ret |= MLK_ACCELKEY_LOGO;

	return ret;
}


//======================


/**@ アクセラレータ作成 */

mAccelerator *mAcceleratorNew(void)
{
	return (mAccelerator *)mMalloc0(sizeof(mAccelerator));
}

/**@ 削除 */

void mAcceleratorDestroy(mAccelerator *p)
{
	if(p)
	{
		mListDeleteAll(&p->list);
		
		mFree(p);
	}
}

/**@ デフォルトのイベント送り先ウィジェットをセット */

void mAcceleratorSetDefaultWidget(mAccelerator *p,mWidget *wg)
{
	p->widget_def = wg;
}

/**@ 追加
 * 
 * @p:key MKEY_* のキーコード。0 の場合は、追加しない。\
 *  装飾キーの指定は MACCELKEY_* を使い、OR 指定する。\
 *  アルファベットキーの場合は、常に大文字に変換される。
 * @p:wg キーが押された時にイベントを送るウィジェット。\
 *  NULL で、アクセラレータ指定のデフォルトウィジェット。 */

void mAcceleratorAdd(mAccelerator *p,int cmdid,uint32_t key,mWidget *wg)
{
	_item *pi;

	if(key == 0) return;
	
	pi = (_item *)mListAppendNew(&p->list, sizeof(_item));
	if(pi)
	{
		pi->cmdid = cmdid;
		pi->key = _conv_key(key);
		pi->widget = wg;
	}
}

/**@ すべてのキー設定を削除 */

void mAcceleratorClear(mAccelerator *p)
{
	mListDeleteAll(&p->list);
}

/**@ コマンドIDから検索し、設定されているキーを取得
 *
 * @r:見つからなかった場合、0 */

uint32_t mAcceleratorGetKey(mAccelerator *p,int cmdid)
{
	_item *pi;

	for(pi = _ITEM(p->list.top); pi; pi = _ITEM(pi->i.next))
	{
		if(pi->cmdid == cmdid)
			return pi->key;
	}

	return 0;
}

/**@ キーコードを元に、キーを表す文字列を取得
 *
 * @d:メニューでショートカットキーを表示する時などに使う。\
 * 装飾キーがある場合、"Shift+*" というような文字列となる。
 *
 * @p:key 0 の場合、空文字列 */

void mAcceleratorGetKeyText(mStr *str,uint32_t key)
{
	mStr strname = MSTR_INIT;

	mStrEmpty(str);

	if(key == 0) return;

	//装飾キー

	if(key & MLK_ACCELKEY_SHIFT)
		mStrAppendText(str, "Shift+");

	if(key & MLK_ACCELKEY_CTRL)
		mStrAppendText(str, "Ctrl+");

	if(key & MLK_ACCELKEY_ALT)
		mStrAppendText(str, "Alt+");

	if(key & MLK_ACCELKEY_LOGO)
		mStrAppendText(str, "Logo+");

	//キー名

	mKeycodeGetName(&strname, key & MLK_ACCELKEY_MASK_KEY);
	mStrAppendStr(str, &strname);

	mStrFree(&strname);
}

/**@ KEYDOWN イベントの情報から、アクセラレータ用のキーを取得
 *
 * @d:アクセラレータキーの入力ウィジェットで使われる */

uint32_t mAcceleratorGetAccelKey_keyevent(mEvent *ev)
{
	return _conv_key(ev->key.key | _conv_state(ev->key.state));
}


//===========================
// 内部で使う
//===========================


/** キー押し時の処理
 *
 * return: TRUE で、アクセラレータキーとして処理した */

mlkbool __mAcceleratorProcKeyEvent(mAccelerator *accel,uint32_t key,uint32_t state)
{
	_item *pi;
	mEventCommand *ev;

	if(key == 0) return FALSE;

	key |= _conv_state(state);
	key = _conv_key(key);

	//キーから検索
		
	for(pi = _ITEM(accel->list.top); pi; pi = _ITEM(pi->i.next))
	{
		if(pi->key == key) break;
	}

	if(!pi) return FALSE;

	//イベント追加

	ev = (mEventCommand *)mEventListAdd(
		(pi->widget)? pi->widget: accel->widget_def,
		MEVENT_COMMAND, sizeof(mEventCommand));

	if(ev)
	{
		ev->id = pi->cmdid;
		ev->from = MEVENT_COMMAND_FROM_ACCELERATOR;
		ev->from_ptr = accel;
	}

	return TRUE;
}
