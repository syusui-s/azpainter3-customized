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

/**********************************
 * フォント 文字列処理
 **********************************/

#include <stdlib.h>

#include "mlk.h"
#include "mlk_buf.h"
#include "mlk_list.h"
#include "mlk_unicode.h"

#include "pv_font.h"


//---------------------

typedef struct
{
	DrawFontString *dst;
	const char *str;
	int type;

	//ルビ
	int ruby_parent_pos,	//親文字列の開始位置 (バッファ位置)
		ruby_ruby_pos;		//ルビ文字の開始位置
}_workdat;

enum
{
	_TYPE_NONE,
	_TYPE_YOKO,
	_TYPE_RUBY1,
	_TYPE_RUBY2,
	_TYPE_DAKUTEN,
	_TYPE_HANDAKUTEN
};

//---------------------


//=============================
// sub
//=============================


/* 文字を追加 */

static void _add_char(_workdat *p,uint32_t c)
{
	if(p->type)
	{
		if(p->type == _TYPE_DAKUTEN)
			c |= CHAR_F_DAKUTEN;
		else if(p->type == _TYPE_HANDAKUTEN)
			c |= CHAR_F_HANDAKUTEN;
	}

	mBufAppend(&p->dst->buf, &c, 4);
}

/* 次の文字が指定文字ならスキップ
 *
 * return: スキップしたか */

static mlkbool _skip_next_char(_workdat *p,char c)
{
	if(*(p->str) == c)
	{
		(p->str)++;
		return TRUE;
	}
	else
		return FALSE;
}

/* {} で囲まれた数値を取得
 *
 * str: '{' の位置
 * return: -1 でエラー */

static int _get_kakko_code(_workdat *p,const char *str,mlkbool hex)
{
	char *pend;
	int n;

	if(*str != '{') return -1;

	n = strtol(str + 1, &pend, (hex)? 16: 10);

	if(n < 0 || *pend != '}' || str + 1 == pend)
		return -1;

	p->str = pend + 1;

	return n;
}


//=============================
//
//=============================


/* '\' の次の文字から処理
 *
 * return: FALSE で '\' を通常文字として追加する */

static mlkbool _proc_format(_workdat *p)
{
	char c;
	int n;

	c = *(p->str);

	switch(c)
	{
		case '\\':
		case '}':
			(p->str)++;
			_add_char(p, c);
			return TRUE;

		//\u{<Unicode(16進数)>}
		case 'u':
			n = _get_kakko_code(p, p->str + 1, TRUE);
			if(n < 0) return FALSE;

			if(n <= 0x10ffff)
				_add_char(p, n);
			return TRUE;

		//\c{<CID(10進数)>}
		case 'c':
			n = _get_kakko_code(p, p->str + 1, FALSE);
			if(n < 0) return FALSE;

			if(n <= 0xffff)
				_add_char(p, n | CHAR_F_CID);
			return TRUE;

		//\r{<親文字列>,<ルビ文字>}
		case 'r':
			if(p->str[1] != '{' || p->type) return FALSE;

			p->str += 2;

			_add_char(p, CHAR_F_COMMAND + CHAR_CMD_ID_RUBY_PARENT_START);

			p->type = _TYPE_RUBY1;
			p->ruby_parent_pos = p->dst->buf.cursize;
			p->ruby_ruby_pos = -1;
			return TRUE;

		//\dk,\dh{濁点,半濁点}
		case 'd':
			if(p->type) return FALSE;

			c = p->str[1];

			if((c == 'k' || c == 'h') && p->str[2] == '{')
			{
				p->str += 3;

				if(c == 'k')
					p->type = _TYPE_DAKUTEN;
				else
					p->type = _TYPE_HANDAKUTEN;
				
				return TRUE;
			}
			break;

		//\y,\y2,\y3{縦中横}
		case 'y':
			if(p->type) return FALSE;

			c = p->str[1];

			if(c == '{')
			{
				p->str += 2;
				n = 0;
			}
			else if(c == '2' && p->str[2] == '{')
			{
				//半角幅
				p->str += 3;
				n = 1;
			}
			else if(c == '3' && p->str[2] == '{')
			{
				//1/3幅
				p->str += 3;
				n = 2;
			}
			else
				return FALSE;

			_add_char(p, CHAR_F_COMMAND + CHAR_CMD_ID_YOKO1_START + n);
		
			p->type = _TYPE_YOKO;
			return TRUE;
	}

	return FALSE;
}

/* 終了の '}' が来た時 */

static void _finish_kakko(_workdat *p)
{
	RubyItem *pi;

	switch(p->type)
	{
		//縦中横
		case _TYPE_YOKO:
			_add_char(p, CHAR_F_COMMAND + CHAR_CMD_ID_YOKO_END);
			break;

		//ルビ
		case _TYPE_RUBY2:
			_add_char(p, CHAR_F_COMMAND + CHAR_CMD_ID_RUBY_END);

			pi = (RubyItem *)mListAppendNew(&p->dst->list_ruby, sizeof(RubyItem));
			if(pi)
			{
				pi->parent_pos = p->ruby_parent_pos;
				pi->ruby_pos = p->ruby_ruby_pos;
			}
			break;
	}

	p->type = 0;
}

/* メイン処理 */

static void _proc_main(_workdat *p,int enable_format)
{
	uint32_t c;
	int ret;

	while(*(p->str))
	{
		//1文字取得
		
		ret = mUTF8GetChar(&c, p->str, -1, &p->str);
		if(ret < 0)
			return;
		else if(ret > 0)
			continue;

		//

		if(p->type == _TYPE_RUBY1 && c == ',')
		{
			//ルビ親文字列中 -> ルビ文字切り替え

			_add_char(p, CHAR_F_COMMAND + CHAR_CMD_ID_RUBY_START);

			p->type = _TYPE_RUBY2;
			p->ruby_ruby_pos = p->dst->buf.cursize;
		}
		else if(p->type && c == '}')
		{
			//{} の終わり

			_finish_kakko(p);
		}
		else if(c == '\n')
		{
			_add_char(p, CHAR_CMDVAL_ENTER);
		}
		else if(c == '\r')
		{
			_add_char(p, CHAR_CMDVAL_ENTER);
			
			_skip_next_char(p, '\n');
		}
		else if(enable_format && c == '\\')
		{
			if(!_proc_format(p))
				_add_char(p, c);
		}
		else
			_add_char(p, c);
	}
}

/** UTF-8 から描画用文字情報を作成 */

void __DrawFont_getString(DrawFontString *dst,const char *text,int enable_format)
{
	_workdat w;

	mMemset0(dst, sizeof(DrawFontString));

	w.dst = dst;
	w.str = text;
	w.type = 0;

	if(!mBufAlloc(&dst->buf, 4 * 64, 4 * 128))
		return;

	_proc_main(&w, enable_format);
}

/** DrawFontString 解放 */

void __DrawFont_freeString(DrawFontString *p)
{
	mBufFree(&p->buf);

	mListDeleteAll(&p->list_ruby);
}

/** (描画中) ルビ親文字列の開始時、描画位置 (相対位置) をセット
 *
 * ptr: RUBY_PARENT_START のコマンドの次の位置 */

void __DrawFont_setRubyParentPos(DrawFontString *p,uint32_t *ptr,int x,int y)
{
	RubyItem *pi;
	int pos;

	pos = (uint8_t *)ptr - p->buf.buf;

	MLK_LIST_FOR(p->list_ruby, pi, RubyItem)
	{
		if(pi->parent_pos == pos)
		{
			pi->x = x;
			pi->y = y;
			break;
		}
	}
}

/** (描画中) ルビ文字開始時、親文字列の幅をセット
 *
 * ptr: RUBY_START のコマンドの次の位置 */

void __DrawFont_setRubyParentW(DrawFontString *p,uint32_t *ptr,int width)
{
	RubyItem *pi;
	int pos;

	pos = (uint8_t *)ptr - p->buf.buf;

	MLK_LIST_FOR(p->list_ruby, pi, RubyItem)
	{
		if(pi->ruby_pos == pos)
		{
			pi->parent_w = width;
			break;
		}
	}
}


