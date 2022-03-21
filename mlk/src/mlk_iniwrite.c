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
 * mIniWrite
 *****************************************/

#include <stdio.h>

#include "mlk.h"
#include "mlk_str.h"
#include "mlk_stdio.h"
#include "mlk_util.h"



/**@ ファイル開く */

FILE *mIniWrite_openFile(const char *filename)
{
	return mFILEopen(filename, "wt");
}

/**@ ファイル開く
 *
 * @d:ディレクトリとファイル名指定。
 * @p:path ディレクトリパス。NULL でなし。 */

FILE *mIniWrite_openFile_join(const char *path,const char *filename)
{
	mStr str = MSTR_INIT;
	FILE *fp;

	if(path)
	{
		mStrSetText(&str, path);
		mStrPathJoin(&str, filename);
	}
	else
		mStrSetText(&str, filename);

	fp = mIniWrite_openFile(str.buf);

	mStrFree(&str);

	return fp;
}

/**@ グループ名出力 */

void mIniWrite_putGroup(FILE *fp,const char *name)
{
	fprintf(fp, "[%s]\n", name);
}

/**@ グループ名を番号から出力 */

void mIniWrite_putGroupInt(FILE *fp,int no)
{
	fprintf(fp, "[%d]\n", no);
}

/**@ int 値出力 */

void mIniWrite_putInt(FILE *fp,const char *key,int n)
{
	fprintf(fp, "%s=%d\n", key, n);
}

/**@ キー名を数値にして int 値出力 */

void mIniWrite_putInt_keyno(FILE *fp,int keyno,int n)
{
	fprintf(fp, "%d=%d\n", keyno, n);
}

/**@ 16進数値出力 */

void mIniWrite_putHex(FILE *fp,const char *key,uint32_t n)
{
	fprintf(fp, "%s=%x\n", key, n);
}

/**@ キー名を数値にして16進数値出力 */

void mIniWrite_putHex_keyno(FILE *fp,int keyno,uint32_t n)
{
	fprintf(fp, "%d=%x\n", keyno, n);
}

/**@ 文字列出力
 *
 * @p:text NULL で空文字列 */

void mIniWrite_putText(FILE *fp,const char *key,const char *text)
{
	fprintf(fp, "%s=%s\n", key, (text)? text: "");
}

/**@ キー名を数値にして文字列出力
 *
 * @p:text NULL で空文字列 */

void mIniWrite_putText_keyno(FILE *fp,int keyno,const char *text)
{
	fprintf(fp, "%d=%s\n", keyno, (text)? text: "");
}

/**@ mStr の文字列出力
 *
 * @p:str NULL または未確保の状態の場合、空文字列 */

void mIniWrite_putStr(FILE *fp,const char *key,mStr *str)
{
	fprintf(fp, "%s=%s\n", key, (str && str->buf)? str->buf: "");
}

/**@ mStr の配列から文字列出力
 *
 * @d:キーは数値の連番となる。\
 * 文字列が空の場合は、出力されない。
 * @p:keytop キー番号の先頭
 * @p:num 配列の数 */

void mIniWrite_putStrArray(FILE *fp,int keytop,mStr *array,int num)
{
	int i;

	for(i = 0; i < num; i++)
	{
		if(mStrIsnotEmpty(array + i))
			fprintf(fp, "%d=%s\n", keytop + i, array[i].buf);
	}
}

/**@ mPoint をカンマで区切って出力 */

void mIniWrite_putPoint(FILE *fp,const char *key,mPoint *pt)
{
	fprintf(fp, "%s=%d,%d\n", key, pt->x, pt->y);
}

/**@ mSize をカンマで区切って出力 */

void mIniWrite_putSize(FILE *fp,const char *key,mSize *size)
{
	fprintf(fp, "%s=%d,%d\n", key, size->w, size->h);
}

/**@ mBox をカンマで区切って出力 */

void mIniWrite_putBox(FILE *fp,const char *key,mBox *box)
{
	fprintf(fp, "%s=%d,%d,%d,%d\n",
		key, box->x, box->y, box->w, box->h);
}

/**@ 数値配列をカンマで区切って出力
 *
 * @d:値は符号なしとして扱う。
 * 
 * @p:buf 数値の配列バッファ。NULL で空。
 * @p:num 配列の数
 * @p:bytes 数値1つのバイト数 (1,2,4)
 * @p:hex 16進数で出力するか */

void mIniWrite_putNumbers(FILE *fp,const char *key,const void *buf,int num,int bytes,mlkbool hex)
{
	const uint8_t *ps = (const uint8_t *)buf;
	uint32_t n;

	//データがない場合も出力する

	fprintf(fp, "%s=", key);

	if(buf)
	{
		for(; num > 0; num--)
		{
			switch(bytes)
			{
				case 2:
					n = *((uint16_t *)ps);
					ps += 2;
					break;
				case 4:
					n = *((uint32_t *)ps);
					ps += 4;
					break;
				default:
					n = *(ps++);
					break;
			}

			if(hex)
				fprintf(fp, "%x", n);
			else
				fprintf(fp, "%u", n);

			if(num > 1) fputc(',', fp);
		}
	}

	fputc('\n', fp);
}

/**@ Base64 にエンコードして出力
 *
 * @d:"key=size:encode_text" として出力される。(size はエンコード前のサイズ)
 * @p:buf データのバッファ
 * @p:size データのサイズ */

void mIniWrite_putBase64(FILE *fp,const char *key,const void *buf,uint32_t size)
{
	int encsize;
	char *encbuf;

	if(!buf || size == 0) return;

	encsize = mGetBase64EncodeSize(size);

	encbuf = (char *)mMalloc(encsize + 1);
	if(!encbuf) return;

	mEncodeBase64(encbuf, encsize + 1, buf, size);

	fprintf(fp, "%s=%d:%s\n", key, size, encbuf);

	mFree(encbuf);
}

