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
Copyright (c) 2020-2021 Azel

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
$*/

/***************************************************
 * mlktrconv.c
 * text -> *.mtr
 ***************************************************/

/*==========================

Convert text to mtr file.
mtr is binary translation file.

# Usage
----------------------

$ mlktrconv [-d OUTPUT_DIR] <input...>
$ mlktrconv -e INPUT OUTPUT

-d <OUTPUT_DIR>
	output directory

-e
	to output header file for embedding into executable file


# Text file format
----------------------

[GROUP_ID]
ID=TEXT
+=TEXT
...

- UTF-8
- group and string ID : 0..65535
- The beginning of line is not '[' or numbers or '+', Comments.
- '\\' => '\'
- '\n' => Enter (LF)
- '\t' => Tab
- '\0' => NULL
- '\' At the end of the line => Enter + next line


# Compile
------------------------

$ cc -O2 -o mlktrconv mlktrconv.c

============================*/


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <getopt.h>


//------------------

typedef struct _TextItem  TextItem;
typedef struct _GroupItem GroupItem;

/* 文字列アイテム */

struct _TextItem
{
	TextItem *next;

	char *text;		//文字列 (テキストバッファ内の位置)
	uint16_t id,	//文字列ID
		len;		//文字数 (NULL 文字含む)
};

/* グループアイテム */

struct _GroupItem
{
	GroupItem *next;	//次のグループ

	TextItem *item_top,	//文字列項目リスト
		*item_bottom;

	uint16_t id;	//グループID
	int item_num;	//文字列数
	uint32_t text_fullsize;	//グループ内の全ての文字列の総バイト数
};

/* 変換用データ */

typedef struct
{
	GroupItem *group_top,	//グループリスト先頭
		*group_bottom;		//グループリスト終端
	int group_num,		//グループ数
		last_itemid;	//+= 用。グループ内で一番最後の文字列のID (-1 で文字列がない)
	char *textbuf,		//入力テキストバッファ
		*output_name;	//出力ファイル名 (確保されたバッファ。NULL で埋め込み用)
}convdata;

//-----------------

convdata g_dat;
char *g_output_dir = NULL;	//出力ディレクトリ

//-----------------


//=========================
// sub
//=========================


/** リストデータ解放 */

static void _delete_list(void)
{
	GroupItem *pg,*pg_next;
	TextItem *pi,*pi_next;

	for(pg = g_dat.group_top; pg; pg = pg_next)
	{
		pg_next = pg->next;

		for(pi = pg->item_top; pi; pi = pi_next)
		{
			pi_next = pi->next;
			free(pi);
		}

		free(pg);
	}

	g_dat.group_top = g_dat.group_bottom = NULL;
}

/** エラー終了 */

static void _exit_err(const char *format,...)
{
	va_list arg;

	printf("error: ");

	va_start(arg, format);
	vprintf(format, arg);
	va_end(arg);

	printf("\n");

	//解放

	_delete_list();

	if(g_dat.textbuf) free(g_dat.textbuf);
	if(g_dat.output_name) free(g_dat.output_name);

	//

	exit(1);
}

/** メモリ不足によるエラー */

static void _exit_err_malloc(void)
{
	_exit_err("memory alloc");
}

/** ファイル読み込み */

static char *_read_file(char *filename)
{
	FILE *fp;
	long size;
	char *buf;

	fp = fopen(filename, "rb");
	if(!fp) _exit_err("open error '%s'", filename);

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	buf = (char *)malloc(size + 1);
	if(!buf)
	{
		fclose(fp);
		_exit_err_malloc();
	}

	fread(buf, 1, size, fp);

	*(buf + size) = 0;

	fclose(fp);

	return buf;
}

/** 次の行のポインタを取得 (改行を NULL に変換)
 *
 * return: 次の位置 */

static char *_get_nextline(char *buf)
{
	char *pc,*pc2,*pnext,*ptop;
	int len,flag;

	if(!(*buf)) return NULL;

	flag = 0;

	while(1)
	{
		//----- 一行分
		
		//改行か終端まで移動

		for(pc = buf; *pc && *pc != '\r' && *pc != '\n'; pc++);

		//行末の空白は削除

		for(pc2 = pc - 1; pc2 > buf && *pc2 == ' '; )
			*(pc2--) = 0;

		//改行をヌルに & 次行の位置

		if(*pc == '\r' && pc[1] == '\n')
		{
			*pc = 0, pc[1] = 0;
			pnext = pc + 2;
		}
		else if(*pc)
		{
			*pc = 0;
			pnext = pc + 1;
		}

		//-----------

		//ヌルを含まない文字長さ

		len = pc - buf;

		//前の行が '\' で終わっている場合、
		//'\' の位置に改行をセットして、それ以降に次行をセット

		if(flag)
		{
			*(ptop++) = '\n';
			
			memmove(ptop, buf, len + 1);

			pc = ptop + len;
		}

		//行末が '\' でなければ確定

		if(!(len && pc[-1] == '\\'))
			return pnext;

		//行末が '\' の場合、次の行へ続く

		buf = pnext;
		ptop = pc - 1; //'\' の位置
		flag = 1;
	}

	return NULL;
}

/** エスケープ文字の変換
 *
 * return: 文字長さ (NULL 含む) */

static int _text_escape(char *text)
{
	char c,*ps,*pd;

	ps = pd = text;

	while(*ps)
	{
		c = *(ps++);
	
		if(c == '\\')
		{
			c = *(ps++);

			if(c == 0)
				break;
			else if(c == 'n')
				c = 0x0a;
			else if(c == 't')
				c = '\t';
			else if(c == '0')
				c = 0;
		}

		*(pd++) = c;
	}

	*pd = 0;

	return (pd - text) + 1;
}

/** バッファに 16bit 値追加 (BE) */

static void _addbuf16(uint8_t **pp,uint16_t val)
{
	uint8_t *pd = *pp;

	pd[0] = (uint8_t)(val >> 8);
	pd[1] = (uint8_t)val;

	*pp += 2;
}

/** バッファに 32bit 値追加 (BE) */

static void _addbuf32(uint8_t **pp,uint32_t val)
{
	uint8_t *pd = *pp;

	pd[0] = (uint8_t)(val >> 24);
	pd[1] = (uint8_t)(val >> 16);
	pd[2] = (uint8_t)(val >> 8);
	pd[3] = (uint8_t)val;

	*pp += 4;
}


//============================
// テキストからリスト作成
//============================


/** グループ開始 */

static void _start_group(char *line)
{
	GroupItem *pi;
	int id;
	char *pc;

	//']' までが数字のみか

	for(pc = line + 1; *pc != ']' && *pc >= '0' && *pc <= '9'; pc++);

	if(*pc != ']')
		_exit_err("group name: %s", line);

	//グループID

	id = atoi(line + 1);

	if(id > 65535)
		_exit_err("group id: %s", line);

	//グループ追加

	pi = (GroupItem *)malloc(sizeof(GroupItem));
	if(!pi) _exit_err_malloc();

	memset(pi, 0, sizeof(GroupItem));

	pi->id = id;

	//リンク

	if(!g_dat.group_top)
		g_dat.group_top = pi;

	if(g_dat.group_bottom)
		g_dat.group_bottom->next = pi;

	g_dat.group_bottom = pi;

	//

	g_dat.group_num++;
	g_dat.last_itemid = -1;
}

/** 文字列項目の追加 */

static void _add_item(char *line)
{
	char *pc;
	TextItem *pi;
	GroupItem *pg;
	int id,len;

	//先頭文字列が正しいか。また、pc を値部分まで進める。

	pc = line;

	if(*pc == '+')
	{
		//"+=" : 直前の ID を +1 した値
		
		if(pc[1] != '=')
			_exit_err("invalid line: %s", line);

		pc += 2;
		id = ++(g_dat.last_itemid);
	}
	else
	{
		//ID 指定
		
		for(; *pc >= '0' && *pc <= '9'; pc++);

		if(*pc != '=')
			_exit_err("invalid line: %s", line);

		pc++;
		id = g_dat.last_itemid = atoi(line);
	}

	//'=' 以降の文字列調整

	len = _text_escape(pc);

	//アイテム追加

	pi = (TextItem *)malloc(sizeof(TextItem));
	if(!pi) _exit_err_malloc();

	pi->next = NULL;
	pi->id   = id;
	pi->text = pc;
	pi->len  = len;

	//リンク

	pg = g_dat.group_bottom;

	if(!pg->item_top)
		pg->item_top = pi;
	
	if(pg->item_bottom)
		pg->item_bottom->next = pi;

	pg->item_bottom = pi;

	//グループの項目数、文字列の総サイズ

	pg->item_num++;
	pg->text_fullsize += pi->len;
}

/** リスト作成 */

static void _create_list(void)
{
	char *pc,*next;

	for(pc = g_dat.textbuf; 1; pc = next)
	{
		//1行取得
		
		next = _get_nextline(pc);
		if(!next) break;

		//

		if(*pc == '[')
			//グループ開始
			_start_group(pc);

		else if(((*pc >= '0' && *pc <= '9') || *pc == '+')
			&& strchr(pc, '=')
			&& g_dat.group_bottom)
		{
			//文字列項目
			//先頭が [ 数字 or '+' ] で、'=' が存在し、グループが開始されていること

			_add_item(pc);
		}
	}
}


//=========================
// ファイルに出力
//=========================


/** mtr ファイルに出力 */

static int _write_binary(char *filename,uint8_t *buf,uint32_t size)
{
	FILE *fp;
	uint8_t ver = 0;

	fp = fopen(filename, "wb");
	if(!fp) return 1;

	fputs("MLKTR", fp);
	fwrite(&ver, 1, 1, fp);
	fwrite(buf, 1, size, fp);

	fclose(fp);

	return 0;
}

/** ヘッダファイルとして出力 */

static int _write_headerfile(char *filename,uint8_t *buf,uint32_t size)
{
	FILE *fp;
	int i,j,remain;
	uint8_t *ps;

	fp = fopen(filename, "wb");
	if(!fp) return 1;

	fputs("static const unsigned char g_deftransdat[] = {\n", fp);

	ps = buf;
	remain = size;

	for(i = (remain + 15) >> 4; i > 0; i--)
	{
		for(j = 0; j < 16 && remain; j++, remain--)
		{
			fprintf(fp, "%d", *(ps++));

			if(remain != 1) fputc(',', fp);
		}

		fputc('\n', fp);
	}

	fputs("};\n", fp);

	fclose(fp);

	return 0;
}


//=========================
// ソート
//=========================


/** グループソート */

static void _sort_group(void)
{
	GroupItem *pi,*pi2,*piprev,*pinext,*pimin,*pi2prev,*piminprev;
	uint16_t minid;

	piprev = NULL;

	for(pi = g_dat.group_top; pi; pi = pinext)
	{
		//ID 最小値取得
		
		pimin = NULL;
		minid = pi->id;
	
		for(pi2 = pi->next; pi2; pi2prev = pi2, pi2 = pi2->next)
		{
			if(pi2->id < minid)
			{
				pimin = pi2;
				piminprev = pi2prev;
				minid = pi2->id;
			}
		}

		//入れ替え

		pinext = pi->next;

		if(pimin)
		{
			if(piprev) piprev->next = pimin;
			pi->next = pimin->next;

			if(pinext == pimin)
				pimin->next = pinext = pi;
			else
			{
				pimin->next = pinext;
				piminprev->next = pi;
			}

			if(pi == g_dat.group_top)
				g_dat.group_top = pimin;
		}

		piprev = (pimin)? pimin: pi;
	}
}

/** 項目ソート */

static void _sort_item(void)
{
	GroupItem *group;
	TextItem *pi,*pi2,*piprev,*pinext,*pimin,*pi2prev,*piminprev;
	uint16_t minid;

	for(group = g_dat.group_top; group; group = group->next)
	{
		piprev = NULL;

		for(pi = group->item_top; pi; pi = pinext)
		{
			//ID 最小値取得
			
			pimin = NULL;
			minid = pi->id;
		
			for(pi2 = pi->next; pi2; pi2prev = pi2, pi2 = pi2->next)
			{
				if(pi2->id < minid)
				{
					pimin = pi2;
					piminprev = pi2prev;
					minid = pi2->id;
				}
			}

			//入れ替え

			pinext = pi->next;

			if(pimin)
			{
				if(piprev) piprev->next = pimin;
				pi->next = pimin->next;

				if(pinext == pimin)
					pimin->next = pinext = pi;
				else
				{
					pimin->next = pinext;
					piminprev->next = pi;
				}

				if(pi == group->item_top)
					group->item_top = pimin;
			}

			piprev = (pimin)? pimin: pi;
		}
	}
}


//=========================
// 変換
//=========================


/** メモリに全体データ出力 (BigEndian) */

static uint8_t *_create_buf(uint32_t *bufsize)
{
	GroupItem *pg;
	TextItem *pi;
	uint8_t *buf,*pd;
	uint32_t size,txtsize,itemnum,offset;

	//------- ソート

	_sort_group();
	_sort_item();

	//------- バッファ確保

	//全体のサイズ計算 (ヘッダは除く)

	txtsize = itemnum = 0;

	for(pg = g_dat.group_top; pg; pg = pg->next)
	{
		txtsize += pg->text_fullsize;
		itemnum += pg->item_num;
	}

	size = 4 * 3 + g_dat.group_num * 8 + itemnum * 6 + txtsize;

	//メモリ確保

	buf = (uint8_t *)malloc(size);
	if(!buf) _exit_err_malloc();

	*bufsize = size;

	//------- 出力

	pd = buf;

	//グループ数、項目全体数、全文字列サイズ

	_addbuf32(&pd, g_dat.group_num);
	_addbuf32(&pd, itemnum);
	_addbuf32(&pd, txtsize);

	//グループリスト

	offset = 0;

	for(pg = g_dat.group_top; pg; pg = pg->next)
	{
		_addbuf16(&pd, pg->id);       //ID
		_addbuf16(&pd, pg->item_num); //項目数
		_addbuf32(&pd, offset);       //項目リストのオフセット位置

		offset += pg->item_num * 6;
	}

	//文字列項目リスト

	offset = 0;

	for(pg = g_dat.group_top; pg; pg = pg->next)
	{
		for(pi = pg->item_top; pi; pi = pi->next)
		{
			_addbuf16(&pd, pi->id);  //ID
			_addbuf32(&pd, offset);  //文字列のオフセット位置

			offset += pi->len;
		}
	}

	//文字列データ (すべてまとめて)

	for(pg = g_dat.group_top; pg; pg = pg->next)
	{
		for(pi = pg->item_top; pi; pi = pi->next)
		{
			memcpy(pd, pi->text, pi->len);

			pd += pi->len;
		}
	}

	return buf;
}

/** ファイル変換
 *
 * embed: 0 で通常変換、1 で埋め込み用変換 */

static void _convert_file(char *inputname,char *outputname,int embed)
{
	uint8_t *datbuf;
	uint32_t bufsize;
	int ret;

	printf("%s -> %s\n", inputname, outputname);

	memset(&g_dat, 0, sizeof(convdata));

	if(!embed)
		g_dat.output_name = outputname;

	//テキスト読み込み

	g_dat.textbuf = _read_file(inputname);

	//リスト作成

	_create_list();

	//全体データをメモリに書き出し

	datbuf = _create_buf(&bufsize);

	//リスト、テキストバッファ解放

	_delete_list();

	free(g_dat.textbuf);
	g_dat.textbuf = NULL;

	//出力

	if(embed)
		ret = _write_headerfile(outputname, datbuf, bufsize);
	else
		ret = _write_binary(outputname, datbuf, bufsize);

	free(datbuf);

	//結果

	if(ret)
		_exit_err("open error '%s'", outputname);
}


//===========================
// main
//===========================


/** 出力ファイル名取得
 *
 * return: 確保された文字列 */

static char *_get_outputname(const char *input)
{
	char *path;
	int len_input,len_outdir,add;

	len_input = strlen(input);

	if(g_output_dir)
	{
		len_outdir = strlen(g_output_dir);
		if(len_outdir == 0) g_output_dir = NULL;
	}

	//

	if(!g_output_dir)
	{
		//出力ディレクトリ指定なし: 拡張子のみ追加

		path = (char *)malloc(len_input + 5);
		if(!path) return NULL;

		strcpy(path, input);
		strcpy(path + len_input, ".mtr");
	}
	else
	{
		//出力ディレクトリ指定あり
		//outdir + input + .mtr

		path = (char *)malloc(len_outdir + 1 + len_input + 5);
		if(!path) return NULL;

		strcpy(path, g_output_dir);

		if(g_output_dir[len_outdir - 1] != '/')
		{
			strcpy(path + len_outdir, "/");
			add = 1;
		}
		else
			add = 0;

		strcpy(path + len_outdir + add, input);
		strcpy(path + len_outdir + add + len_input, ".mtr");
	}

	return path;
}

/** コマンドライン引数取得
 *
 * return: 0 で mtr 出力、1 でヘッダファイル出力 */

static int _get_options(int argc,char **argv)
{
	int opt,embed = 0,help = 0;

	while(1)
	{
		opt = getopt(argc, argv, "ed:h");
		if(opt < 0) break;

		switch(opt)
		{
			case 'e':
				embed = 1;
				break;
			case 'd':
				g_output_dir = optarg;
				break;
			case 'h':
				help = 1;
				break;
		}
	}

	//ヘルプを表示して終了

	if(help
		|| (embed && argc - optind < 2)
		|| (!embed && argc - optind < 1))
	{
		printf("<Usage>\n"
			"%s [-d OUTPUT_DIR] INPUT...\n"
			"%s -e INPUT OUTPUT\n\n"
			"<Options>\n"
			"  -d DIR : output directory\n"
			"  -e     : to output header file for embedding into executable file\n"
			"  -h     : help\n"
			, argv[0], argv[0]);

		exit(0);
	}

	return embed;
}

/** メイン */

int main(int argc,char **argv)
{
	int i;
	char *outname;

	if(_get_options(argc, argv))
	{
		//ヘッダファイル出力

		_convert_file(argv[optind], argv[optind + 1], 1);
	}
	else
	{
		//各ファイル変換
		
		for(i = optind; i < argc; i++)
		{
			outname = _get_outputname(argv[i]);
			if(!outname) break;
			
			_convert_file(argv[i], outname, 0);

			free(outname);
		}
	}

	return 0;
}
