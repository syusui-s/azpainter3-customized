/*$
 Copyright (C) 2013-2020 Azel.

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
 * mAccelerator [アクセラレータ]
 *****************************************/

#include "mDef.h"

#include "mAccelerator.h"

#include "mGui.h"
#include "mList.h"
#include "mEvent.h"
#include "mWidget.h"
#include "mStr.h"


//-------------------------

#define _ITEM(p)  ((mAccelItem *)(p))

struct _mAccelerator
{
	mList list;
	mWidget *defaultWidget;
};

typedef struct
{
	mListItem i;
	
	int cmdid;
	uint32_t key;
	mWidget *widget;
}mAccelItem;

//-------------------------


/***********************//**

@defgroup accelerator mAccelerator
@brief アクセラレータ
@ingroup group_data

@details
- アクセラレータはトップレベルウィンドウに関連付けることでキーが処理される。\n
ウィンドウに関連付けるには、mWindowData::accelerator にアクセラレータのポインタをセットする。
- 同じアクセラレータを複数のウィンドウにセット可能。
- アクセラレータは自動で削除されないので、明示的に解放する必要がある。
- 定義されたキーが押されたら、\b MEVENT_COMMAND イベントが追加される。
- メニュー項目と同じ ID を使う場合、メニュー項目が無効状態でもイベントが送られるので注意。

@{
@file mAccelerator.h

****************************/


/** アクセラレータ作成 */

mAccelerator *mAcceleratorNew(void)
{
	return (mAccelerator *)mMalloc(sizeof(mAccelerator), TRUE);
}

/** 削除 */

void mAcceleratorDestroy(mAccelerator *accel)
{
	if(accel)
	{
		mListDeleteAll(&accel->list);
		
		mFree(accel);
	}
}

/** デフォルトウィジェットセット */

void mAcceleratorSetDefaultWidget(mAccelerator *accel,mWidget *wg)
{
	accel->defaultWidget = wg;
}

/** 追加
 * 
 * @param key MKEY_* の仮想キーコード。装飾キーは MACCKEY_* で OR 指定。
 * @param wg  イベントを送るウィジェット。NULL でアクセラレータ指定のデフォルトウィジェット。 */

void mAcceleratorAdd(mAccelerator *accel,int cmdid,uint32_t key,mWidget *wg)
{
	mAccelItem *pi;

	if(key == 0) return;
	
	pi = (mAccelItem *)mListAppendNew(&accel->list, sizeof(mAccelItem), NULL);
	if(!pi) return;
	
	pi->cmdid = cmdid;
	pi->key = key;
	pi->widget = wg;
}

/** すべてのキー設定を削除 */

void mAcceleratorDeleteAll(mAccelerator *accel)
{
	mListDeleteAll(&accel->list);
}

/** コマンドIDから設定されているキーを取得
 *
 * @return 見つからなかった場合、0 */

uint32_t mAcceleratorGetKeyByID(mAccelerator *p,int cmdid)
{
	mAccelItem *pi;

	for(pi = _ITEM(p->list.top); pi; pi = _ITEM(pi->i.next))
	{
		if(pi->cmdid == cmdid)
			return pi->key;
	}

	return 0;
}

/** キーコードから文字列取得
 *
 * @return 確保された文字列のポインタ */

char *mAcceleratorGetKeyText(uint32_t key)
{
	mStr str = MSTR_INIT;
	char name[32],*buf;

	if(key & MACCKEY_SHIFT)
		mStrAppendText(&str, "Shift+");

	if(key & MACCKEY_CTRL)
		mStrAppendText(&str, "Ctrl+");

	if(key & MACCKEY_ALT)
		mStrAppendText(&str, "Alt+");

	mKeyCodeToName(key & MACCKEY_KEYMASK, name, 32);
	mStrAppendText(&str, name);

	buf = mStrdup(str.buf);

	mStrFree(&str);

	return buf;
}

/** キーイベントからアクセラレータのキーを取得 */

uint32_t mAcceleratorGetKeyFromEvent(mEvent *ev)
{
	uint32_t key;

	key = ev->key.code;

	if(ev->key.state & M_MODS_SHIFT) key |= MACCKEY_SHIFT;
	if(ev->key.state & M_MODS_CTRL)  key |= MACCKEY_CTRL;
	if(ev->key.state & M_MODS_ALT)   key |= MACCKEY_ALT;

	return key;
}

/** @} */


//==================


/** トップウィンドウのキー処理 */

mBool __mAccelerator_keyevent(mAccelerator *accel,uint32_t key,uint32_t state,int press)
{
	mAccelItem *pi;
	
	if(state & M_MODS_SHIFT) key |= MACCKEY_SHIFT;
	if(state & M_MODS_CTRL)  key |= MACCKEY_CTRL;
	if(state & M_MODS_ALT)   key |= MACCKEY_ALT;
	
	//キーから検索
	
	for(pi = _ITEM(accel->list.top); pi; pi = _ITEM(pi->i.next))
	{
		if(pi->key == key) break;
	}
	
	if(!pi) return FALSE;
	
	//MEVENT_COMMAND
	
	if(press)
	{
		mWidgetAppendEvent_command(
			(pi->widget)? pi->widget: accel->defaultWidget,
			pi->cmdid, 0, MEVENT_COMMAND_BY_ACCEL);
	}

	return TRUE;
}
