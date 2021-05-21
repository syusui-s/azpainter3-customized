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
 * UndoItem
 * 基本部分 (確保と読み書き)
 *****************************************/

#include <stdio.h>
#include <string.h>	//memcpy

#include "mlk_gui.h"
#include "mlk_str.h"
#include "mlk_undo.h"
#include "mlk_file.h"
#include "mlk_stdio.h"

#include "undo.h"
#include "undoitem.h"
#include "pv_undo.h"

#include "def_config.h"


//------------------

#define _PUT_DEBUG  0

enum
{
	_WRITETYPE_FILE,
	_WRITETYPE_MEM_FIX,
	_WRITETYPE_MEM_VARIABLE
};

//------------------



//=========================
// sub
//=========================


/* バッファ確保 */

static mlkerr _alloc_buf(UndoItem *p,int size)
{
	p->buf = (uint8_t *)mMalloc(size);
	if(!p->buf) return MLKERR_ALLOC;

	p->size = size;
	p->fileno = -1;

	APPUNDO->used_bufsize += size;

#if _PUT_DEBUG
	mDebug("mem:+%d -> %u\n", size, APPUNDO->used_bufsize);
#endif

	return MLKERR_OK;
}

/* ファイル確保 (ファイル番号のみセット) */

static mlkerr _alloc_file(UndoItem *p)
{
	//空文字列で、作業用ディレクトリ使用不可

	if(mStrIsEmpty(&APPCONF->strTempDirProc))
		return MLKERR_EMPTY;

	//ファイル番号

	p->fileno = APPUNDO->cur_fileno;

	APPUNDO->cur_fileno = (APPUNDO->cur_fileno + 1) & 0xffff;

#if _PUT_DEBUG
	mDebug("file:%d\n", p->fileno);
#endif

	return MLKERR_OK;
}

/* ファイル名取得 */

static void _get_filename(mStr *str,int no)
{
	char m[16];

	snprintf(m, 16, "undo%04x", no);

	mStrCopy(str, &APPCONF->strTempDirProc);
	mStrPathJoin(str, m);
}

/* ファイル時:書き込み開く */

static mlkerr _openfile_write(UndoItem *p)
{
	mStr str = MSTR_INIT;

	_get_filename(&str, p->fileno);

	APPUNDO->writefp = mFILEopen(str.buf, "wb");

	mStrFree(&str);

	return (APPUNDO->writefp)? MLKERR_OK: MLKERR_OPEN;
}

/* 可変サイズ時、バッファからファイル出力に切り替え */

static mlkerr _change_to_file(UndoItem *p,const void *buf,int size)
{
	mlkerr ret;

	APPUNDO->write_type = _WRITETYPE_FILE;

	//ファイル確保

	ret = _alloc_file(p);
	if(ret) return ret;

	//開く
	
	ret = _openfile_write(p);
	if(ret) return ret;

	//今までの一時バッファの内容を書き込み

	if(fwrite(APPUNDO->writetmpbuf, 1, APPUNDO->write_tmpsize, APPUNDO->writefp) != APPUNDO->write_tmpsize)
		return MLKERR_IO;

	//データ書き込み

	if(fwrite(buf, 1, size, APPUNDO->writefp) != size)
		return MLKERR_IO;

	return MLKERR_OK;
}


//=========================
// main
//=========================


/** 解放 */

void UndoItem_free(UndoItem *p)
{
	//[!] データは val の値のみの場合があるので、
	//    p->buf の値で判定せずに fileno で判定する。

	if(p->fileno == -1)
	{
		//バッファ
		
		if(p->buf)
		{
			mFree(p->buf);

			APPUNDO->used_bufsize -= p->size;
		}
	}
	else
	{
		//ファイル削除

		mStr str = MSTR_INIT;

		_get_filename(&str, p->fileno);

		mDeleteFile(str.buf);

		mStrFree(&str);
	}
}

/** バッファまたはファイルの確保
 *
 * size: 1 以上でメモリ優先固定サイズ、-1 で常にファイル、-2 でメモリ優先で可変
 *
 * 可変時でも openWrite は常に実行する。
 * (アンドゥバッファが足りない場合、ファイル出力になるため) */

mlkerr UndoItem_alloc(UndoItem *p,int size)
{
	int type,remain;

	//アンドゥバッファの残りサイズ
	// :undo_maxbufsize = 0 で、常にファイルに出力。
	// :環境設定でバッファサイズを変更した後は、
	// :使用済みサイズの方が大きい場合があるので注意。

	if(APPCONF->undo_maxbufsize <= APPUNDO->used_bufsize)
		remain = 0;
	else
		remain = APPCONF->undo_maxbufsize - APPUNDO->used_bufsize;

	//出力タイプ

	if(size == UNDO_ALLOC_FILE || remain == 0)
		//ファイル
		type = _WRITETYPE_FILE;
	else if(size > 0)
	{
		//サイズ固定時
		// :バッファに収まる場合はメモリ確保、超えればファイル

		type = (size <= remain)? _WRITETYPE_MEM_FIX: _WRITETYPE_FILE;
	}
	else
	{
		//可変サイズ
		// :一時バッファに出力し、バッファに出力可能なサイズを超えた時点で、
		// :ファイル出力に切り替え。

		type = _WRITETYPE_MEM_VARIABLE;

		APPUNDO->write_tmpsize = 0;
		APPUNDO->write_remain = (remain < UNDO_WRITETEMP_BUFSIZE)? remain: UNDO_WRITETEMP_BUFSIZE;
	}

	APPUNDO->write_type = type;

	//確保

	if(type == _WRITETYPE_FILE)
		return _alloc_file(p);
	else if(type == _WRITETYPE_MEM_FIX)
		return _alloc_buf(p, size);
	else
		//可変サイズ
		return MLKERR_OK;
}


//===========================
// 読み書き
//===========================


/** 固定サイズ書き込み処理 */

mlkerr UndoItem_writeFixSize(UndoItem *p,int size,mlkerr (*func)(UndoItem *,void *),void *param)
{
	mlkerr ret;

	ret = UndoItem_alloc(p, size);
	if(ret) return ret;
	
	ret = UndoItem_openWrite(p);
	if(ret) return ret;

	ret = (func)(p, param);

	UndoItem_closeWrite(p);

	return ret;
}

/** バッファから全体を書き込み */

mlkerr UndoItem_writeFull_buf(UndoItem *p,void *buf,int size)
{
	mlkerr ret;

	ret = UndoItem_alloc(p, size);
	if(ret) return ret;
	
	ret = UndoItem_openWrite(p);
	if(ret) return ret;

	ret = UndoItem_write(p, buf, size);

	UndoItem_closeWrite(p);

	return ret;
}

/** 先頭から指定サイズデータ読み込み
 *
 * ppdst: size 分確保されたバッファ */

mlkerr UndoItem_readTop(UndoItem *p,int size,void **ppdst)
{
	void *buf;
	mlkerr ret;

	buf = mMalloc(size);
	if(!buf) return MLKERR_ALLOC;

	//読み込み

	ret = UndoItem_openRead(p);
	if(ret) goto END;
	
	ret = UndoItem_read(p, buf, size);
	if(ret) goto END;

	UndoItem_closeRead(p);

END:
	if(ret)
		mFree(buf);
	else
		*ppdst = buf;

	return ret;
}

/** データをコピー
 *
 * - データサイズ固定時。
 * - val データのみの場合は除く。 */

mlkerr UndoItem_copyData(UndoItem *src,UndoItem *dst,int size)
{
	uint8_t *buf;
	mlkerr ret;

	ret = UndoItem_alloc(dst, size);
	if(ret) return ret;

	//コピー

	if(dst->buf && src->buf)
	{
		//両方バッファ
		
		memcpy(dst->buf, src->buf, size);
		return MLKERR_OK;
	}
	else
	{
		//一方または両方がファイルの場合

		buf = (uint8_t *)mMalloc(size);
		if(!buf) return MLKERR_ALLOC;

		ret = UndoItem_openRead(src);
		if(ret) goto END;
		
		ret = UndoItem_openWrite(dst);
		if(ret) goto END;
		
		ret = UndoItem_read(src, buf, size);
		if(ret) goto END;
		
		ret = UndoItem_write(dst, buf, size);
		if(ret) goto END;

	END:
		UndoItem_closeRead(src);
		UndoItem_closeWrite(dst);

		mFree(buf);

		return ret;
	}
}


//=========================
// 書き込み
//=========================


/** 可変サイズで書き込み開始 */

mlkerr UndoItem_allocOpenWrite_variable(UndoItem *p)
{
	mlkerr ret;

	ret = UndoItem_alloc(p, UNDO_ALLOC_MEM_VARIABLE);
	if(ret) return ret;

	return UndoItem_openWrite(p);
}

/** 可変サイズの書き込み終了
 *
 * err: 最後の読み書き時のエラー */

mlkerr UndoItem_closeWrite_variable(UndoItem *p,mlkerr err)
{
	if(err == MLKERR_OK)
		return UndoItem_closeWrite(p);
	else
	{
		//書き込み時にエラーがあった場合、そのエラーを返す
		
		UndoItem_closeWrite(p);

		return err;
	}
}

/** 書き込み開く */

mlkerr UndoItem_openWrite(UndoItem *p)
{
	switch(APPUNDO->write_type)
	{
		//バッファ
		case _WRITETYPE_MEM_FIX:
			APPUNDO->write_dst = p->buf;
			break;

		//ファイル
		case _WRITETYPE_FILE:
			return _openfile_write(p);
	}

	return MLKERR_OK;
}

/** 書き込み */

mlkerr UndoItem_write(UndoItem *p,const void *buf,int size)
{
	//データなし
	
	if(!buf || size <= 0) return MLKERR_OK;

	//

	switch(APPUNDO->write_type)
	{
		//バッファ
		case _WRITETYPE_MEM_FIX:
			memcpy(APPUNDO->write_dst, buf, size);

			APPUNDO->write_dst += size;
			break;

		//ファイル
		case _WRITETYPE_FILE:
			if(fwrite(buf, 1, size, APPUNDO->writefp) != size)
				return MLKERR_IO;
			break;

		//可変
		case _WRITETYPE_MEM_VARIABLE:
			if(size <= APPUNDO->write_remain)
			{
				//残りサイズ内に収まる場合は、一時バッファに

				memcpy(APPUNDO->writetmpbuf + APPUNDO->write_tmpsize, buf, size);

				APPUNDO->write_tmpsize += size;
				APPUNDO->write_remain -= size;
			}
			else
			{
				//一時バッファを超える場合は、ファイル出力に切り替え

				return _change_to_file(p, buf, size);
			}
			break;
	}

	return MLKERR_OK;
}

/** 書き込み閉じる */

mlkerr UndoItem_closeWrite(UndoItem *p)
{
	mlkerr ret;

	switch(APPUNDO->write_type)
	{
		//ファイル
		case _WRITETYPE_FILE:
			if(APPUNDO->writefp)
			{
				fclose(APPUNDO->writefp);
				APPUNDO->writefp = NULL;
			}
			break;

		//可変サイズ
		// :終了時点でファイル出力に切り替わっていない場合は、
		// :一時バッファからメモリ出力。
		case _WRITETYPE_MEM_VARIABLE:
			ret = _alloc_buf(p, APPUNDO->write_tmpsize);
			if(ret) return ret;

			memcpy(p->buf, APPUNDO->writetmpbuf, APPUNDO->write_tmpsize);
			break;
	}

	return MLKERR_OK;
}

/** ファイル書き込み時、現在位置保存+圧縮サイズ(仮)書き込み */

mlkerr UndoItem_writeEncSize_temp(void)
{
	uint32_t v = 0;

	APPUNDO->writefpos = ftell(APPUNDO->writefp);

	if(fwrite(&v, 1, 4, APPUNDO->writefp) != 4)
		return MLKERR_IO;
	else
		return MLKERR_OK;
}

/** ファイル書き込み時、実際の圧縮サイズ書き込み */

void UndoItem_writeEncSize_real(uint32_t size)
{
	fseek(APPUNDO->writefp, APPUNDO->writefpos, SEEK_SET);

	fwrite(&size, 4, 1, APPUNDO->writefp);

	fseek(APPUNDO->writefp, 0, SEEK_END);
}


//=========================
// 読み込み
//=========================


/** 読み込み開く */

mlkerr UndoItem_openRead(UndoItem *p)
{
	if(p->fileno == -1)
	{
		//バッファ

		APPUNDO->read_dst = p->buf;
	}
	else
	{
		//ファイル

		mStr str = MSTR_INIT;

		_get_filename(&str, p->fileno);

		APPUNDO->readfp = mFILEopen(str.buf, "rb");

		mStrFree(&str);

		if(!APPUNDO->readfp) return MLKERR_OPEN;
	}

	return MLKERR_OK;
}

/** 読み込み */

mlkerr UndoItem_read(UndoItem *p,void *buf,int size)
{
	if(p->fileno == -1)
	{
		memcpy(buf, APPUNDO->read_dst, size);

		APPUNDO->read_dst += size;
	}
	else
	{
		if(fread(buf, 1, size, APPUNDO->readfp) != size)
			return MLKERR_IO;
	}

	return MLKERR_OK;
}

/** 読み込みシーク */

void UndoItem_readSeek(UndoItem *p,int seek)
{
	if(p->fileno == -1)
		APPUNDO->read_dst += seek;
	else
		fseek(APPUNDO->readfp, seek, SEEK_CUR);
}

/** 読み込み閉じる */

void UndoItem_closeRead(UndoItem *p)
{
	if(APPUNDO->readfp)
	{
		fclose(APPUNDO->readfp);
		APPUNDO->readfp = NULL;
	}
}

