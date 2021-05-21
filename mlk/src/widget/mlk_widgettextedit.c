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
 * mWidgetTextEdit
 *
 * テキストエディタにおけるテキスト処理
 *****************************************/

#include "mlk_gui.h"
#include "mlk_key.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_font.h"
#include "mlk_str.h"
#include "mlk_clipboard.h"
#include "mlk_unicode.h"

#include "mlk_widgettextedit.h"

/*---------------

[mWidgetTextEdit] (mlk_widget_def.h)

	mWidget *widget;
	uint32_t *unibuf;	//テキストバッファ (UTF-32)
	uint16_t *wbuf;		//各文字のpx幅
	int textlen,		//テキスト長さ
		alloclen,		//テキストの確保文字数 (NULL 含む)
		curpos,			//カーソルの文字位置
		curx,			//カーソルの X 位置
		maxwidth,		//[single] テキスト全体の幅 [multi] 行の最大幅
		seltop,			//選択範囲の先頭位置 (-1 でなし)
		selend,			//選択範囲の終端 (+1)
		multi_curline,	//[multi] 現在行位置 (-1 で単一行)
		multi_maxlines,	//[multi] 最大行数
		is_multiline;	//TRUE で複数行

- バッファ確保失敗時は、unibuf = wbuf = NULL。


	MWIDGETTEXTEDIT_KEYPROC_CHANGE_CURSOR
		カーソル位置のみ変更時
	MWIDGETTEXTEDIT_KEYPROC_CHANGE_TEXT
		テキスト変更時
	MWIDGETTEXTEDIT_KEYPROC_UPDATE
		表示内容の更新のみ

-------------------*/

#define _INIT_BUF_LEN  32  //初期テキスト長さ


//===========================
// sub
//===========================


/** 文字列長さから、バッファ確保長さ取得 */

static int _calc_alloc_len(int len)
{
	int i;
	
	for(i = 32; i < (1<<28) && i <= len; i <<= 1);
	
	return i;
}

/** UTF-8 文字列から UTF-32 に変換
 * 
 * dst: NULL で文字数のみ計算
 * return: 文字数 */

static int _utf8_to_uni(const char *src,uint32_t *dst,mlkbool multi_line)
{
	const char *ps;
	mlkuchar uc;
	int ret,len = 0,last_cr = 0;
	
	for(ps = src; *ps; )
	{
		//1文字変換
		
		ret = mUTF8GetChar(&uc, ps, -1, &ps);
		
		if(ret < 0) break;
		else if(ret > 0) continue;

		//
		
		if(uc == '\r' || uc == '\n')
		{
			//改行 (複数行時のみ有効)
			
			if(!multi_line) continue;
			
			if(uc == '\r')
			{
				//'\n' に統一。
				//'\r\n' の場合、次の '\n' は除外する。
				
				uc = '\n';
				last_cr = 1;
			}
			else if(uc == '\n')
			{
				//前の文字が \r だったので、除外
				if(last_cr)
				{
					last_cr = 0;
					continue;
				}
			}
		}
		else
		{
			//その他 (ASCII 文字のうち、表示不可の文字は除く)
			
			last_cr = 0;
		
			if(uc < 128 && !(uc >= 32 && uc < 127))
				continue;
		}
		
		if(dst) *(dst++) = uc;
		len++;
	}
	
	return len;
}

/** 文字幅バッファに全ての文字の幅をセット */

static void _set_width_buf(mWidgetTextEdit *p)
{
	mFont *font;
	uint32_t *pt;
	uint16_t *pw;

	pt = p->unibuf;
	pw = p->wbuf;
	if(!pt) return;
	
	font = mWidgetGetFont(p->widget);

	for(; *pt; pt++, pw++)
	{
		if(*pt == '\n')
			*pw = 0;
		else
			*pw = mFontGetCharWidth_utf32(font, *pt);
	}
}

/** カーソル位置の変更時
 *
 * X位置/行位置を計算する */

static void _change_curpos(mWidgetTextEdit *p)
{
	uint32_t *pt;
	uint16_t *pw;
	int i,x,line;

	pt = p->unibuf;
	pw = p->wbuf;
	if(!pt) return;

	x = line = 0;

	for(i = p->curpos; i > 0; i--, pt++, pw++)
	{
		if(*pt == '\n')
		{
			line++;
			x = 0;
		}
		else
			x += *pw;
	}

	p->curx = x;
	p->multi_curline = line;
}

/** カーソル位置をセット
 *
 * (テキストの変更なしの場合。選択範囲は変わらない)
 *
 * return: 変更されたか */

static mlkbool _set_curpos(mWidgetTextEdit *p,int pos)
{
	if(pos == p->curpos)
		return FALSE;
	else
	{
		p->curpos = pos;

		_change_curpos(p);

		return TRUE;
	}
}

/** テキスト変更時、最大幅と最大行数を計算 */

static void _change_text(mWidgetTextEdit *p)
{
	uint32_t *pt;
	uint16_t *pw;
	int maxw,lines,linew;

	pt = p->unibuf;
	pw = p->wbuf;
	if(!pt) return;

	//

	lines = 1;
	linew = maxw = 0;

	for(; *pt; pt++, pw++)
	{
		if(*pt == '\n')
		{
			if(maxw < linew) maxw = linew;
			linew = 0;
			lines++;
		}
		else
			linew += *pw;
	}

	if(maxw < linew) maxw = linew;

	p->maxwidth = maxw;
	p->multi_maxlines = lines;
}

/** [multi] 指定範囲内の幅を取得 (改行は 4px)
 *
 * end: +1 */

static int _get_area_textwidth(mWidgetTextEdit *p,int top,int end)
{
	uint32_t *pt = p->unibuf + top;
	uint16_t *pw = p->wbuf + top;
	int w = 0;

	for(; top < end; top++, pt++, pw++)
		w += (*pt == '\n')? 4: *pw;

	return w;
}


//===========================
// main
//===========================


/** 解放 */

void mWidgetTextEdit_free(mWidgetTextEdit *p)
{
	mFree(p->unibuf);
	mFree(p->wbuf);
}

/** 初期化 */

void mWidgetTextEdit_init(mWidgetTextEdit *p,mWidget *widget,mlkbool multi_line)
{
	//テキストバッファ
	
	p->unibuf = (uint32_t *)mMalloc(sizeof(uint32_t) * _INIT_BUF_LEN);

	//文字幅バッファ

	p->wbuf = (uint16_t *)mMalloc(sizeof(uint16_t) * _INIT_BUF_LEN);
	if(!p->wbuf)
	{
		mFree(p->unibuf);
		p->unibuf = NULL;
	}

	//

	if(p->unibuf)
	{
		p->unibuf[0] = 0;
		p->alloclen = _INIT_BUF_LEN;
	}
	
	p->widget = widget;
	p->seltop = -1;
	p->is_multiline = multi_line;
}

/** バッファのリサイズ */

mlkbool mWidgetTextEdit_resize(mWidgetTextEdit *p,int len)
{
	void *buf;

	//確保されている

	if(len < p->alloclen) return TRUE;

	//確保サイズ
	
	len = _calc_alloc_len(len);

	//テキストバッファ
	
	buf = mRealloc(p->unibuf, sizeof(uint32_t) * len);
	if(!buf) goto ERR;
	
	p->unibuf = (uint32_t *)buf;

	//文字幅バッファ

	buf = mRealloc(p->wbuf, sizeof(uint16_t) * len);
	if(!buf) goto ERR;

	p->wbuf = (uint16_t *)buf;

	//
	
	p->alloclen = len;
	
	return TRUE;

	//エラー時は解放
ERR:
	mFree(p->unibuf);
	mFree(p->wbuf);

	p->unibuf = NULL,
	p->wbuf = NULL;

	mWidgetTextEdit_empty(p);

	return FALSE;
}

/** テキストを空にする */

void mWidgetTextEdit_empty(mWidgetTextEdit *p)
{
	if(p->unibuf)
		p->unibuf[0] = 0;

	p->textlen = 0;
	p->curpos = 0;
	p->curx = 0;
	p->seltop = -1;
	p->maxwidth = 0;

	p->multi_maxlines = 1;
	p->multi_curline = 0;
}

/** テキストが空かどうか */

mlkbool mWidgetTextEdit_isEmpty(mWidgetTextEdit *p)
{
	return (p->textlen == 0);
}

/** 指定文字位置が選択範囲内かどうか */

mlkbool mWidgetTextEdit_isSelected_atPos(mWidgetTextEdit *p,int pos)
{
	return (p->seltop != -1 && p->seltop <= pos && pos < p->selend);
}

/** カーソル位置を終端へ移動 */

void mWidgetTextEdit_moveCursor_toBottom(mWidgetTextEdit *p)
{
	_set_curpos(p, p->textlen);
}

/** [multi] ピクセル位置から文字位置を取得 */

int mWidgetTextEdit_multi_getPosAtPixel(mWidgetTextEdit *p,int px,int py,int lineh)
{
	uint32_t *pt;
	uint16_t *pw;
	int x,y,pos;

	if(px < 0) px = 0;
	if(py < 0) py = 0;

	pt = p->unibuf;
	pw = p->wbuf;

	if(!pt) return 0;

	x = y = pos = 0;

	for(; *pt; pt++, pw++, pos++)
	{
		//文字の範囲内
		
		if(y <= py && py < y + lineh && x <= px && px < x + *pw)
			break;

		//

		if(*pt == '\n')
		{
			if(py < y + lineh) break;
		
			x = 0;
			y += lineh;
		}
		else
			x += *pw;
	}

	return pos;
}

/** [multi] 描画時、1行の範囲内の選択状態を取得
 *
 * end: 次行先頭か終端位置
 * dst: 部分ごとに分けた情報が入る (最大3つ)
 * return: セットされた dst の数 */

int mWidgetTextEdit_getLineState(mWidgetTextEdit *p,int top,int end,mWidgetTextEditState *dst)
{
	int stop,send,tend;

	stop = p->seltop;
	send = p->selend; //+1

	//テキスト描画時に改行を除くための終端位置
	tend = (end && p->unibuf[end - 1] == '\n')? end - 1: end;

	if(stop < 0 || top >= send || end <= stop)
	{
		//選択範囲なし、
		//または、選択範囲はあるが、すべて範囲外の位置にある

		dst[0].text = p->unibuf + top;
		dst[0].len = tend - top;
		dst[0].width = _get_area_textwidth(p, top, end);
		dst[0].is_sel = 0;

		return 1;
	}
	else if(stop <= top && send >= end)
	{
		//選択範囲が指定範囲を覆っている

		dst[0].text = p->unibuf + top;
		dst[0].len = tend - top;
		dst[0].width = _get_area_textwidth(p, top, end);
		dst[0].is_sel = 1;
		
		return 1;
	}
	else if(stop > top && send < end)
	{
		//部分選択: 行内の一部が選択 (3つ)

		dst[0].text = p->unibuf + top;
		dst[0].len = stop - top;
		dst[0].width = _get_area_textwidth(p, top, stop);
		dst[0].is_sel = 0;

		dst[1].text = p->unibuf + stop;
		dst[1].len = send - stop;
		dst[1].width = _get_area_textwidth(p, stop, send);
		dst[1].is_sel = 1;
		
		dst[2].text = p->unibuf + send;
		dst[2].len = tend - send;
		dst[2].width = _get_area_textwidth(p, send, end);
		dst[2].is_sel = 0;

		return 3;
	}
	else if(stop <= top)
	{
		//部分選択: top〜send が選択 (2つ)

		dst[0].text = p->unibuf + top;
		dst[0].len = send - top;
		dst[0].width = _get_area_textwidth(p, top, send);
		dst[0].is_sel = 1;

		dst[1].text = p->unibuf + send;
		dst[1].len = tend - send;
		dst[1].width = _get_area_textwidth(p, send, end);
		dst[1].is_sel = 0;

		return 2;
	}
	else
	{
		//部分選択: stop〜end が選択 (2つ)

		dst[0].text = p->unibuf + top;
		dst[0].len = stop - top;
		dst[0].width = _get_area_textwidth(p, top, stop);
		dst[0].is_sel = 0;

		dst[1].text = p->unibuf + stop;
		dst[1].len = tend - stop;
		dst[1].width = _get_area_textwidth(p, stop, end);
		dst[1].is_sel = 1;

		return 2;
	}
}


//================================
// テキストセット
//================================


/** UTF-8 テキストセット
 * 
 * 単一行時、カーソル位置は終端へ。選択範囲はなしとなる。 */

void mWidgetTextEdit_setText(mWidgetTextEdit *p,const char *text)
{
	int len;

	//空にする

	mWidgetTextEdit_empty(p);

	if(!text) return;
	
	//文字数取得
	
	len = _utf8_to_uni(text, NULL, p->is_multiline);
	if(len == 0) return;

	//リサイズ
	
	if(!mWidgetTextEdit_resize(p, len)) return;
	
	//テキストセット
	
	_utf8_to_uni(text, p->unibuf, p->is_multiline);
	
	p->unibuf[len] = 0;
	p->textlen = len;

	//

	_set_width_buf(p);
	_change_text(p);

	//単一行時はカーソルを終端へ

	if(!p->is_multiline)
	{
		p->curpos = p->textlen;
		p->curx = p->maxwidth;
	}
}

/** 文字列または１文字を挿入
 * 
 * ch: 0 以外で文字を挿入、0 で text の文字列を挿入
 * return: 追加されたか */

mlkbool mWidgetTextEdit_insertText(mWidgetTextEdit *p,const char *text,char ch)
{
	int len;
	uint32_t *tsrc,*tdst;
	uint16_t *wsrc,*wdst;
	mFont *font;
	char m[2];
	int i;
	
	if(ch)
	{
		//文字挿入
		
		m[0] = ch;
		m[1] = 0;
		
		text = m;
	}
	else
	{
		if(!text) return FALSE;
	}
	
	//文字数
	
	len = _utf8_to_uni(text, NULL, p->is_multiline);
	if(len == 0) return FALSE;
	
	//選択範囲の文字列を削除
	
	mWidgetTextEdit_deleteSelText(p);

	//リサイズ
	
	if(!mWidgetTextEdit_resize(p, p->textlen + len))
		return FALSE;

	//挿入位置を空ける (テキスト)
	
	tsrc = p->unibuf + p->textlen;
	tdst = tsrc + len;
	
	for(i = p->textlen - p->curpos; i >= 0; i--)
		*(tdst--) = *(tsrc--);
	
	//テキストセット
	
	_utf8_to_uni(text, p->unibuf + p->curpos, p->is_multiline);

	//挿入位置を空ける (文字幅)
	
	wsrc = p->wbuf + p->textlen;
	wdst = wsrc + len;
	
	for(i = p->textlen - p->curpos; i >= 0; i--)
		*(wdst--) = *(wsrc--);

	//文字幅セット

	font = mWidgetGetFont(p->widget);

	tsrc = p->unibuf + p->curpos;
	wsrc = p->wbuf + p->curpos;

	for(i = len; i > 0; i--, tsrc++)
		*(wsrc++) = mFontGetCharWidth_utf32(font, *tsrc);
	
	//
	
	p->textlen += len;
	p->curpos += len;

	_change_text(p);
	_change_curpos(p);

	return TRUE;
}


//================================
// 処理
//================================


/** すべて選択
 *
 * return: テキストが空の場合 FALSE */

mlkbool mWidgetTextEdit_selectAll(mWidgetTextEdit *p)
{
	if(p->textlen == 0)
		return FALSE;
	else
	{
		p->seltop = 0;
		p->selend = p->textlen;
		return TRUE;
	}
}

/** 選択範囲の文字列を削除 */

mlkbool mWidgetTextEdit_deleteSelText(mWidgetTextEdit *p)
{
	if(p->seltop < 0)
		//選択範囲なし
		return FALSE;
	else
	{
		mWidgetTextEdit_deleteText(p, p->seltop, p->selend - p->seltop);
	
		p->seltop = -1;
	
		return TRUE;
	}
}

/** 指定範囲の文字列削除 */

void mWidgetTextEdit_deleteText(mWidgetTextEdit *p,int pos,int len)
{
	uint32_t *tsrc,*tdst;
	uint16_t *wsrc,*wdst;
	int i;

	if(!p->unibuf) return;
	
	//詰める (テキスト)
	
	tdst = p->unibuf + pos;
	tsrc = tdst + len;
		
	for(i = p->textlen - (pos + len); i > 0; i--)
		*(tdst++) = *(tsrc++);
	
	*tdst = 0;

	//詰める (文字幅)

	wdst = p->wbuf + pos;
	wsrc = wdst + len;
		
	for(i = p->textlen - (pos + len); i > 0; i--)
		*(wdst++) = *(wsrc++);

	//
	
	p->textlen -= len;
	p->curpos = pos;

	_change_text(p);
	_change_curpos(p);
}

/** カーソルを指定文字位置に移動
 *
 * select: 0 で選択は解除、0 以外で選択範囲の移動 */

void mWidgetTextEdit_moveCursorPos(mWidgetTextEdit *p,int pos,int select)
{
	if(select)
		mWidgetTextEdit_expandSelect(p, pos);
	else
		p->seltop = -1;

	_set_curpos(p, pos); 
}

/** 選択範囲拡張 (カーソル位置から指定位置まで) */

void mWidgetTextEdit_expandSelect(mWidgetTextEdit *p,int pos)
{
	if(p->seltop < 0)
	{
		//選択なし -> 選択開始
		
		if(pos < p->curpos)
			p->seltop = pos, p->selend = p->curpos;
		else
			p->seltop = p->curpos, p->selend = pos;
	}
	else
	{
		//選択あり -> 拡張
		
		if(pos < p->curpos)
		{
			if(pos < p->seltop)
				p->seltop = pos;
			else
				p->selend = pos;
		}
		else if(pos > p->curpos)
		{
			if(pos > p->selend)
				p->selend = pos;
			else
				p->seltop = pos;
		}
		
		if(p->seltop == p->selend)
			p->seltop = -1;
	}
}

/** ドラッグによる選択範囲拡張時 */

void mWidgetTextEdit_dragExpandSelect(mWidgetTextEdit *p,int pos)
{
	mWidgetTextEdit_expandSelect(p, pos);

	_set_curpos(p, pos);
}

/** 貼り付け */

mlkbool mWidgetTextEdit_paste(mWidgetTextEdit *p)
{
	mStr str = MSTR_INIT;

	if(mClipboardGetText(&str))
		return FALSE;
	else
	{
		mWidgetTextEdit_insertText(p, str.buf, 0);
	
		mStrFree(&str);
	
		return TRUE;
	}
}

/** コピー */

void mWidgetTextEdit_copy(mWidgetTextEdit *p)
{
	char *utf8;
	int len;

	if(p->seltop != -1)
	{
		utf8 = mUTF32toUTF8_alloc(p->unibuf + p->seltop,
			p->selend - p->seltop, &len);
	
		if(utf8)
		{
			mClipboardSetText(utf8, len);
			
			mFree(utf8);
		}
	}
}

/** 切り取り
 *
 * return: 選択がない場合 FALSE */

mlkbool mWidgetTextEdit_cut(mWidgetTextEdit *p)
{
	if(p->seltop == -1)
		return FALSE;
	else
	{
		mWidgetTextEdit_copy(p);
		mWidgetTextEdit_deleteSelText(p);
	
		return TRUE;
	}
}


//================================
// キー操作処理
//================================


/** [multi] カーソル上下移動時の移動先文字位置を取得 */

static int _get_move_updown_pos(mWidgetTextEdit *p,mlkbool up)
{
	uint32_t *pt;
	uint16_t *pw;
	int i,x,line,curx,newline;

	pt = p->unibuf;
	pw = p->wbuf;
	if(!pt) return 0;
	
	curx = p->curx;

	newline = p->multi_curline + (up? -1: 1);

	for(i = x = line = 0; *pt; pt++, pw++, i++)
	{
		//移動後の行 & 現在カーソルのX位置より右ならその位置
		
		if(line == newline)
		{
			if(up)
			{
				if(x + *pw >= curx) break;
			}
			else
			{
				if(x >= curx) break;
			}
		}

		//改行

		if(*pt == '\n')
		{
			if(line == newline) break;

			line++;
			x = 0;
		}
		else
			x += *pw;
	}

	return i;
}

/** カーソルを上下左右に移動
 * 
 * select: 選択範囲の移動
 * return: 移動したか */

static mlkbool _move_cursor_key(mWidgetTextEdit *p,uint32_t key,int select)
{
	int pos = 0;
	
	switch(key)
	{
		//左
		case MKEY_LEFT:
		case MKEY_KP_LEFT:
			if(!select && p->seltop != -1)
				//選択がある & 通常移動の場合、選択の先頭位置へ
				pos = p->seltop;
			else if(p->curpos)
				//左へ
				pos = p->curpos - 1;
			else
				return FALSE;
			break;

		//右
		case MKEY_RIGHT:
		case MKEY_KP_RIGHT:
			if(!select && p->seltop != -1)
				//選択の終端へ
				pos = p->selend;
			else if(p->curpos < p->textlen)
				pos = p->curpos + 1;
			else
				return FALSE;
			break;

		//上
		case MKEY_UP:
		case MKEY_KP_UP:
			if(!p->is_multiline || p->multi_curline == 0)
				return FALSE;
			else
				pos = _get_move_updown_pos(p, TRUE);
			break;

		//下
		case MKEY_DOWN:
		case MKEY_KP_DOWN:
			if(!p->is_multiline || p->multi_curline == p->multi_maxlines - 1)
				return FALSE;
			else
				pos = _get_move_updown_pos(p, FALSE);
			break;
	}
	
	mWidgetTextEdit_moveCursorPos(p, pos, select);

	return TRUE;
}

/** 先頭/終端へ移動
 *
 * +Shift の場合は、選択拡張 */

static mlkbool _move_cursor_home_end(mWidgetTextEdit *p,mlkbool home,int select)
{
	uint32_t *pt;
	int pos;

	if(!p->is_multiline)
	{
		//単一行

		pos = (home)? 0: p->textlen;
	}
	else
	{
		//複数行

		if(!p->unibuf) return FALSE;
		
		pt = p->unibuf + p->curpos;

		if(home)
		{
			//行頭
			
			for(pos = p->curpos; pos > 0; pos--, pt--)
			{
				if(pt[-1] == '\n') break;
			}
		}
		else
		{
			//行端
			
			for(; *pt && *pt != '\n'; pt++);
			
			pos = pt - p->unibuf;
		}
	}

	mWidgetTextEdit_moveCursorPos(p, pos, select);

	return TRUE;
}

/** バックスペース/削除 */

static int _key_delete(mWidgetTextEdit *p,int backspace)
{
	if(p->seltop >= 0)
	{
		//選択がある場合は、選択文字列を削除
	
		if(mWidgetTextEdit_deleteSelText(p))
			return MWIDGETTEXTEDIT_KEYPROC_CHANGE_TEXT;
	}
	else if(backspace)
	{
		//バックスペース : カーソルの一つ前の文字を削除
		
		if(p->curpos > 0)
		{
			mWidgetTextEdit_deleteText(p, p->curpos - 1, 1);
			return MWIDGETTEXTEDIT_KEYPROC_CHANGE_TEXT;
		}
	}
	else
	{
		//DELETE
		
		if(p->curpos < p->textlen)
		{
			mWidgetTextEdit_deleteText(p, p->curpos, 1);
			return MWIDGETTEXTEDIT_KEYPROC_CHANGE_TEXT;
		}
	}
	
	return 0;
}

/** キー処理
 *
 * enable_edit: TRUE で編集可能な状態 */

int mWidgetTextEdit_keyProc(mWidgetTextEdit *p,uint32_t key,uint32_t state,int enable_edit)
{
	int ret,ctrl,select;
	char m[2];
	
	ret = 0;
	ctrl = ((state & MLK_STATE_MASK_MODS) == MLK_STATE_CTRL);
	select = (state & MLK_STATE_SHIFT);
	
	switch(key)
	{
		//ENTER
		case MKEY_ENTER:
		case MKEY_KP_ENTER:
			if(p->is_multiline && enable_edit)
			{
				m[0] = '\n';
				m[1] = 0;

				if(mWidgetTextEdit_insertText(p, m, 0))
					ret = MWIDGETTEXTEDIT_KEYPROC_CHANGE_TEXT;
			}
			break;

		//上下左右
		case MKEY_LEFT:
		case MKEY_RIGHT:
		case MKEY_UP:
		case MKEY_DOWN:
		case MKEY_KP_LEFT:
		case MKEY_KP_RIGHT:
		case MKEY_KP_UP:
		case MKEY_KP_DOWN:
			if(_move_cursor_key(p, key, select))
				ret = MWIDGETTEXTEDIT_KEYPROC_CHANGE_CURSOR;
			break;

		//後退
		case MKEY_BACKSPACE:
			if(enable_edit)
				ret = _key_delete(p, TRUE);
			break;

		//削除
		case MKEY_DELETE:
			if(enable_edit)
				ret = _key_delete(p, FALSE);
			break;

		//HOME
		case MKEY_HOME:
		case MKEY_KP_HOME:
			if(_move_cursor_home_end(p, TRUE, select))
				ret = MWIDGETTEXTEDIT_KEYPROC_CHANGE_CURSOR;
			break;

		//END
		case MKEY_END:
		case MKEY_KP_END:
			if(_move_cursor_home_end(p, FALSE, select))
				ret = MWIDGETTEXTEDIT_KEYPROC_CHANGE_CURSOR;
			break;

		//Ctrl+V
		case 'v':
		case 'V':
			if(ctrl && enable_edit)
			{
				if(mWidgetTextEdit_paste(p))
					ret = MWIDGETTEXTEDIT_KEYPROC_CHANGE_TEXT;
			}
			break;

		//Ctrl+C
		case 'c':
		case 'C':
			if(ctrl)
				mWidgetTextEdit_copy(p);
			break;

		//Ctrl+X
		case 'x':
		case 'X':
			if(ctrl && enable_edit)
			{
				if(mWidgetTextEdit_cut(p))
					ret = MWIDGETTEXTEDIT_KEYPROC_CHANGE_TEXT;
			}
			break;

		//Ctrl+A
		case 'a':
		case 'A':
			if(ctrl)
			{
				if(mWidgetTextEdit_selectAll(p))
					ret = MWIDGETTEXTEDIT_KEYPROC_UPDATE;
			}
			break;
	}
	
	return ret;
}
