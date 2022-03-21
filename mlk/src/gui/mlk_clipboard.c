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
 * mClipboard
 * 
 * バックエンド共通のクリップボード処理
 *****************************************/

#include <string.h>

#include "mlk_gui.h"
#include "mlk_clipboard.h"

#include "mlk_pv_gui.h"


/**@ クリップボードの所有権を解放
 *
 * @d:自身がクリップボードデータを持っている場合、
 *  そのデータと、クリップボードの所有権を解放する。
 *
 * @r:所有権を解放した場合、TRUE */

mlkbool mClipboardRelease(void)
{
	return (MLKAPP->bkend.clipboard_release)();
}

/**@ クリップボードに任意のデータをセット
 *
 * @d:正常にセットされると、自身がクリップボードの所有権を持つ。
 *
 * @p:type データのタイプ。
 * @p:mimetypes 送信可能な MIME タイプ文字列のリスト。\
 *  複数の形式に対応する場合は、タブ文字で区切る。\
 *  NULL で、type に合わせて、バックエンドごとのデフォルトの MIME タイプをセット。
 * @p:handle データを送信する時のハンドラ。\
 *  NULL で、type に合わせてデフォルト処理を行う。\
 *  データを送信するタイミングが来たら呼ばれるので、
 *  関数内ですべてのデータを mClipboardSend() を使って送信する。\
 *  戻り値は、RET_OK でデータを正常に送信した。RET_NONE で何も送らなかった。
 *  RET_SEND_RAW で、セットされているデータをそのまま生で送信させる (関数内では何も送信しない)。
 * @r:成功したか */

mlkbool mClipboardSetData(int type,const void *buf,uint32_t size,
	const char *mimetypes,mFuncClipboardSend handle)
{
	return (MLKAPP->bkend.clipboard_setdata)(type, buf, size, mimetypes, (mFuncEmpty)handle);
}

/**@ クリップボードにテキストデータをセット
 *
 * @p:text UTF-8
 * @p:len  文字列長さ。負の値で自動。 */

mlkbool mClipboardSetText(const char *text,int len)
{
	if(len < 0)
		len = strlen(text);

	return (MLKAPP->bkend.clipboard_setdata)(MCLIPBOARD_TYPE_TEXT, text, len, NULL, NULL);
}

/**@ データの送信
 *
 * @d:mFuncClipboardSend のハンドラ関数内で、データを送る時に使う。\
 *  複数回呼んで少しずつ送信しても良い。 */

mlkbool mClipboardSend(const void *buf,int size)
{
	return (MLKAPP->bkend.clipboard_send)(buf, size);
}

/**@ クリップボードからテキストを取得
 *
 * @r:MCLIPBOARD_RET_* */

int mClipboardGetText(mStr *str)
{
	return (MLKAPP->bkend.clipboard_gettext)(str);
}

/**@ クリップボードから指定タイプのデータ取得
 *
 * @p:mimetype データの MIME タイプ
 * @p:handle データ受信関数
 * @p:param 受信関数に渡すパラメータ値
 * @r:MCLIPBOARD_RET_* */

int mClipboardGetData(const char *mimetype,mFuncClipboardRecv handle,void *param)
{
	return (MLKAPP->bkend.clipboard_getdata)(mimetype, (mFuncEmpty)handle, param);
}

/**@ 現在取得可能なクリップボードデータが対応する MIME タイプの文字列リストを取得
 *
 * @r:確保された char * の配列。NULL でなし。\
 *  配列の各ポインタと、配列バッファを解放すること。\
 *  配列の値が 0 で終端。 */

char **mClipboardGetMimeTypeList(void)
{
	return (MLKAPP->bkend.clipboard_getmimetypelist)();
}

/**@ MIME タイプ文字列のリストから、実際に取得可能な MIME タイプの文字列を一つ取得
 *
 * @p:mime_types 検査する MIME タイプ文字列のリスト。\
 *  ヌル文字で区切る (終端はヌル文字を２つ並べる)。\
 *  先頭にあるタイプほど優先される。
 * @r:取得可能な最初の MIME タイプの文字列ポインタが返る (mime_types 内の位置)。\
 *  NULL で見つからなかった。 */

char *mClipboardFindMimeType(const char *mime_types)
{
	return (MLKAPP->bkend.clipboard_findmimetype)(mime_types);
}

