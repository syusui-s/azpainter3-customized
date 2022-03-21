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
 * コマンドライン引数操作
 *****************************************/

#include <string.h>
#include <stdio.h>

#include "mlk.h"
#include "mlk_argparse.h"


//--------------------

typedef struct
{
	mArgParse *ap;

	char *optch,	//短い名前の時、見つかった位置。長い名前の場合 NULL
		*optval;	//-opt=VAL の場合、'=' の位置。NULL で '=' がない

	int	curindex,	//現在位置
		argindex,	//オプション以外の引数の先頭位置 (-1 でなし)
		fendopt;	//オプションが終了したか
}_data;

//--------------------



/** str を長い名前としてオプション検索
 *
 * str: -- は除く */

static mArgParseOpt *_search_opt_long(_data *p,char *str)
{
	mArgParseOpt *opt;
	char *pcend;
	int len;

	//str 側の長さ
	// : '=' を含む場合は、その前まで

	pcend = strchr(str, '=');
	if(pcend)
		len = pcend - str;
	else
		len = strlen(str);

	if(len == 0) return NULL;

	//

	for(opt = p->ap->opts; opt->longopt || opt->opt; opt++)
	{
		if(opt->longopt
			&& strncmp(str, opt->longopt, len) == 0
			&& opt->longopt[len] == 0)
		{
			p->optch = NULL;
			p->optval = pcend;
			return opt;
		}
	}

	return NULL;
}

/** str を短い名前としてオプション検索
 *
 * -abc など、複数のオプションを連続指定可能なため、
 * 最初の文字から順に検索していく。
 *
 * str: 最初の '-' は除く。先頭の文字のみ検索する。 */

static mArgParseOpt *_search_opt_short(_data *p,char *str)
{
	mArgParseOpt *opt;
	char c;

	c = *str;

	for(opt = p->ap->opts; opt->longopt || opt->opt; opt++)
	{
		if(c == opt->opt)
		{
			p->optch = str;
			p->optval = strchr(str, '=');
			return opt;
		}
	}

	return NULL;
}

/** 関数実行 */

static void _run_func(_data *p,mArgParseOpt *o,char *arg)
{
	if(o->func)
	{
		p->ap->curopt = o;

		(o->func)(p->ap, arg);
	}
}

/** オプションとして処理
 *
 * return: 0 でオプション文字列のみ処理、1 で次の引数を使った。
 * -1 でエラー */

static int _run_option(_data *p,char *str,mArgParseOpt *o)
{
	//オプション処理フラグ

	o->flags |= MARGPARSEOPT_F_PROCESSED;

	//引数がなければ、OK

	if(!(o->flags & MARGPARSEOPT_F_HAVE_ARG))
	{
		_run_func(p, o, NULL);
		return 0;
	}

	//----- 引数あり

	//オプション文字列内に '=' がある場合

	if(p->optval)
	{
		_run_func(p, o, p->optval + 1);
		return 0;
	}

	//----- 次の引数が値

	//短い名前で、次が終了でない場合
	//(-abc で b に引数がある場合など)

	if(p->optch && p->optch[1])
	{
		fprintf(stderr, "option error: %s\n", str);
		return -1;
	}

	//次の引数を値として使う

	if(p->curindex + 1 >= p->ap->argc)
	{
		fprintf(stderr, "option requires value: %s\n", str);
		return -1;
	}

	_run_func(p, o, p->ap->argv[p->curindex + 1]);

	return 1;
}
	
/** 先頭が '-' の場合の処理
 *
 * return: 0 で通常引数として扱う。
 *  1 でオプションとして処理した。
 *  -1 でエラー終了。 */

static int _proc_option(_data *p,char *str)
{
	mArgParseOpt *o;
	char *pc;
	int ret,loop = 1;

	//オプションが終了している場合、常に通常引数として扱う
	
	if(p->fendopt) return 0;
	
	//オプションを終了

	if(strcmp(str, "--") == 0)
	{
		p->curindex++;
		p->fendopt = 1;
		return 1;
	}

	//-------------

	pc = str + 1;

	while(loop)
	{
		//オプションを検索

		if(str[1] == '-')
		{
			//長い名前
			
			o = _search_opt_long(p, str + 2);
			loop = 0;
		}
		else
		{
			//短い名前 (先頭文字から順に検索)

			if(*pc == 0 || *pc == '=')
			{
				p->curindex++;
				break;
			}
			
			o = _search_opt_short(p, pc);

			pc++;
		}

		//定義されていないオプション

		if(!o)
		{
			if(p->ap->flags & MARGPARSE_FLAGS_UNKNOWN_IS_ARG)
			{
				//非定義のオプションは通常引数として扱う
				return 0;
			}
			else
			{
				fprintf(stderr, "unknown option: %s\n", str);
				return -1;
			}
		}

		//処理

		ret = _run_option(p, str, o);

		if(ret == -1)
			return -1;
		else if(ret == 1)
		{
			//次の引数を値として使った

			p->curindex += 2;
			break;
		}
		else
		{
			//-opt=VAL または -abc
			//長い名前 (--opt=VAL) の場合、次へ

			if(!p->optch)
				p->curindex++;
		}
	}

	return 1;
}

/** オプション引数を、通常引数の前に挿入 */

static void _move_option(_data *p,int last)
{
	char **argv,*opt1,*opt2;
	int i,top,from,num;

	argv = p->ap->argv;
	num = p->curindex - last;

	//オプションのポインタを保存

	opt1 = argv[last];
	if(num == 2) opt2 = argv[last + 1];

	//argindex 〜 last - 1 までを下にオプション分ずらす

	top = p->argindex + num;
	from = last - 1;

	for(i = p->curindex - 1; i >= top; i--, from--)
		argv[i] = argv[from];

	//オプションのポインタを挿入

	i = p->argindex;

	argv[i] = opt1;
	if(num == 2) argv[i + 1] = opt2;

	//

	p->argindex = top;
}


//============================
// main
//============================


/**@ 引数からオプションを解析
 *
 * @d:argv 内のポインタは、先頭にオプション、後ろに通常の引数が並ぶように
 * 入れ替えられる。\
 *
 * @r:オプション以外の引数の先頭インデックス位置。\
 * argc と同じなら、以降に引数はない。\
 * -1 で解析エラー。 */

int mArgParseRun(mArgParse *p)
{
	_data dat;
	char *pc;
	int ret,last,argc;

	dat.ap = p;
	dat.curindex = 1;
	dat.argindex = -1;
	dat.fendopt = 0;

	//

	argc = p->argc;

	while(dat.curindex < argc)
	{
		last = dat.curindex;
		pc = p->argv[last];

		if(*pc == '-')
		{
			ret = _proc_option(&dat, pc);
			if(ret == -1)
			{
				//エラー

				fflush(stderr);
				return -1;
			}
			else if(ret)
			{
				/* オプションとして処理した場合、
				 * オプションの前に通常引数があれば、
				 * 通常引数が後ろに並ぶようにずらす */

				if(dat.argindex != -1)
					_move_option(&dat, last);
				
				continue;
			}
		}

		//先頭が '-' でない or ret = 0: 通常引数

		if(dat.argindex == -1)
			dat.argindex = dat.curindex;
		
		dat.curindex++;
	}

	return (dat.argindex == -1)? argc: dat.argindex;
}

