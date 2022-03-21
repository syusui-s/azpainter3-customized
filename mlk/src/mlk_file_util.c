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
 * mFile を使ったファイル操作
 *****************************************/

#include "mlk.h"
#include "mlk_file.h"
#include "mlk_string.h"


//==============================
// 単体関数
//==============================


/**@ ファイルの内容を、確保したメモリ上にすべて読み込む
 *
 * @g:単体関数
 *
 * @d:ファイルが空の場合、デフォルトでエラーになる。\
 * 空ファイルを許可して空だった場合は、常に終端に 0 (1byte) が追加される。\
 * \
 * また、ファイルサイズが INT32_MAX (2GB) を超える場合もエラーになる。
 *
 * @p:filename ファイル名 (UTF-8)
 * @p:flags フラグ。\
 *  \
 *  @tbl>
 *  |||MREADFILEFULL_APPEND_0||終端に 1byte の 0 を追加する||
 *  |||MREADFILEFULL_ACCEPT_EMPTY||空ファイルをエラーにせず受け入れる。ファイルが存在しない場合は除く。||
 *  @tbl<
 * @p:ppdst 確保されたバッファのポインタが入る。
 * @p:psize NULL 以外で、確保したバッファのサイズが入る。\
 *  終端に 0 を追加した場合、それも含む。
 * @r:OK/OPEN/ALLOC/EMPTY/MAX_SIZE/NEED_MORE */

mlkerr mReadFileFull_alloc(const char *filename,uint32_t flags,uint8_t **ppdst,int32_t *psize)
{
	mFile file;
	mlkfoff fsize;
	uint8_t *buf = NULL,fappend = FALSE;
	int32_t bufsize;
	mlkerr ret;

	*ppdst = NULL;

	//開く

	ret = mFileOpen_read(&file, filename);
	if(ret) return ret;

	//ファイルサイズ

	fsize = mFileGetSize(file);
	
	if(fsize == 0 && !(flags & MREADFILEFULL_ACCEPT_EMPTY))
	{
		//空は許可しない
		ret = MLKERR_EMPTY;
		goto ERR;
	}
	else if(fsize > INT32_MAX)
	{
		//サイズが大きすぎる
		ret = MLKERR_MAX_SIZE;
		goto ERR;
	}

	//バッファサイズ

	bufsize = fsize;

	if(fsize == 0 || (flags & MREADFILEFULL_APPEND_0))
	{
		bufsize++;
		fappend = TRUE;
	}

	//確保

	buf = (uint8_t *)mMalloc(bufsize);
	if(!buf)
	{
		ret = MLKERR_ALLOC;
		goto ERR;
	}

	//読み込み

	ret = mFileRead_full(file, buf, fsize);
	if(ret) goto ERR;

	//0 追加

	if(fappend)
		buf[fsize] = 0;

	mFileClose(file);

	//

	*ppdst = buf;

	if(psize) *psize = bufsize;

	return MLKERR_OK;

ERR:
	mFree(buf);
	mFileClose(file);
	return ret;
}

/**@ ファイルの先頭から指定サイズを読み込み
 *
 * @p:buf 読み込み先
 * @p:size 読み込むサイズ */

mlkerr mReadFileHead(const char *filename,void *buf,int32_t size)
{
	mFile file;
	mlkerr ret;

	ret = mFileOpen_read(&file, filename);
	if(ret) return ret;

	ret = mFileRead_full(file, buf, size);

	mFileClose(file);

	return ret;
}

/**@ ファイルをコピー
 *
 * @d:ファイルを読み込みつつ書き込んでいく。 */

mlkerr mCopyFile(const char *srcpath,const char *dstpath)
{
	mFile fsrc,fdst;
	uint8_t *buf;
	int32_t size;
	mlkerr ret;

	//バッファ確保

	buf = (uint8_t *)mMalloc(16 * 1024);
	if(!buf) return MLKERR_ALLOC;

	//開く

	fsrc = fdst = MFILE_NONE;

	ret = mFileOpen_read(&fsrc, srcpath);
	if(ret) goto ERR;
	
	ret = mFileOpen_write(&fdst, dstpath, -1);
	if(ret) goto ERR;

	//コピー

	while(1)
	{
		size = mFileRead(fsrc, buf, 16 * 1024);

		if(size > 0)
		{
			//書き込み
			ret = mFileWrite_full(fdst, buf, size);
			if(ret) break;
		}
		else if(size == 0)
		{
			//終端
			ret = MLKERR_OK;
			break;
		}
		else
		{
			//エラー
			ret = MLKERR_IO;
			break;
		}
	}

ERR:
	mFileClose(fdst);
	mFileClose(fsrc);
	mFree(buf);

	return ret;
}

/**@ バッファの内容をファイルに書き込み */

mlkerr mWriteFile_fromBuf(const char *filename,const void *buf,int32_t size)
{
	mFile file;
	mlkerr ret;

	ret = mFileOpen_write(&file, filename, -1);
	if(ret) return ret;

	ret = mFileWrite_full(file, buf, size);

	mFileClose(file);

	return ret;
}


//==============================
// テキスト読み込み
//==============================


struct _mFileText
{
	char *buf,
		*cur,		//現在位置
		*next;		//次の行の位置 (NULL で終了)
	int32_t size;	//バッファサイズ
};


/**@ ファイルから読み込んで作成
 *
 * @g:テキスト読み込み
 *
 * @d:ファイル内容はすべてメモリに読み込まれる。
 *
 * @p:dst 読み込み成功時、mFileText のポインタが入る。 */

mlkerr mFileText_readFile(mFileText **dst,const char *filename)
{
	mFileText *p;
	uint8_t *buf;
	int32_t size;
	mlkerr ret;

	*dst = NULL;

	//読み込み

	ret = mReadFileFull_alloc(filename,
		MREADFILEFULL_ACCEPT_EMPTY | MREADFILEFULL_APPEND_0,
		&buf, &size);

	if(ret) return ret;

	//確保

	p = (mFileText *)mMalloc0(sizeof(mFileText));
	if(!p)
	{
		mFree(buf);
		return MLKERR_ALLOC;
	}

	//

	*dst = p;

	p->buf = p->cur = p->next = (char *)buf;
	p->size = size;

	return MLKERR_OK;
}

/**@ 終了 */

void mFileText_end(mFileText *p)
{
	if(p)
	{
		mFree(p->buf);
		mFree(p);
	}
}

/**@ 次の一行を取得
 *
 * @d:改行文字を 0 に変換して、行頭のポインタ (読み込んだバッファ内) を返す。\
 * 戻り値のポインタからヌル文字までの範囲は、内容を変更しても構わない。
 * 
 * @r:NULL で終了 */

char *mFileText_nextLine(mFileText *p)
{
	if(!p->next)
		return NULL;
	else
	{
		p->cur = p->next;
		p->next = mStringGetNextLine(p->cur, TRUE);

		return p->cur;
	}
}

/**@ 次の一行を取得 (空行はスキップ)
 *
 * @d:すべての空行をスキップして、文字がある次の行を取得する。 */

char *mFileText_nextLine_skip(mFileText *p)
{
	while(p->next)
	{
		p->cur = p->next;
		p->next = mStringGetNextLine(p->cur, TRUE);

		//行頭に文字がある
		if(*(p->cur)) break;
	}

	return (p->next)? p->cur: NULL;
}
