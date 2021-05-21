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

/******************************
 * 設定ファイル、ver 2->3 変換
 ******************************/

#include <stdio.h>

#include "mlk_gui.h"
#include "mlk_stdio.h"
#include "mlk_str.h"
#include "mlk_file.h"
#include "mlk_util.h"


/* ソースファイルと出力ファイルを開く */

static int _open_file(const char *dstname,const char *srcname,FILE **ppdst,FILE **ppsrc)
{
	mStr str = MSTR_INIT;
	FILE *fpsrc,*fpdst = NULL;

	//ソース

	mStrPathSetHome_join(&str, ".azpainter");
	mStrPathJoin(&str, srcname);

	fpsrc = mFILEopen(str.buf, "rb");

	//出力

	if(fpsrc)
	{
		mGuiGetPath_config(&str, dstname);

		fpdst = mFILEopen(str.buf, "wb");

		if(!fpdst)
		{
			fclose(fpsrc);
			fpsrc = NULL;
		}
	}

	mStrFree(&str);

	//

	if(fpsrc && fpdst)
	{
		*ppsrc = fpsrc;
		*ppdst = fpdst;
		return 0;
	}
	else
		return 1;
}

/* 出力ファイルを開く */

static FILE *_open_dstfile(const char *fname)
{
	mStr str = MSTR_INIT;
	FILE *fp;

	mGuiGetPath_config(&str, fname);

	fp = mFILEopen(str.buf, "wb");

	mStrFree(&str);

	return fp;
}

/* ファイルをコピーする */

static void _copyfile(const char *dstname,const char *srcname)
{
	mStr strsrc = MSTR_INIT, strdst = MSTR_INIT;

	mStrPathSetHome_join(&strsrc, ".azpainter");
	mStrPathJoin(&strsrc, srcname);

	mGuiGetPath_config(&strdst, dstname);

	mCopyFile(strsrc.buf, strdst.buf);

	mStrFree(&strdst);
	mStrFree(&strsrc);
}


//================================
// カラーパレット
//================================


/* カラーパレット変換 */

static void _convert_colorpalette(void)
{
	FILE *fpsrc,*fpdst;
	void *buf;
	uint8_t ver,lnum,dat[12];
	uint16_t pnum;
	int i,len,size;
	char m[16];

	if(_open_file("colpalette.dat", "palette-2.dat", &fpdst, &fpsrc))
		return;

	//読み込み

	if(mFILEreadStr_compare(fpsrc, "AZPLPLD")
		|| mFILEreadByte(fpsrc, &ver)
		|| ver != 1
		|| mFILEreadByte(fpsrc, &lnum))
		goto ERR;

	//書き込み

	fputs("AZPTCONFPAL", fpdst);

	mFILEwriteByte(fpdst, 0);
	mFILEwriteBE16(fpdst, lnum);

	//データ

	for(i = 0; i < lnum; i++)
	{
		if(mFILEreadLE16(fpsrc, &pnum)
			|| pnum == 0)
			break;

		size = pnum * 3;

		buf = mMalloc(size);
		if(!buf) break;

		if(mFILEreadOK(fpsrc, buf, size))
		{
			mFree(buf);
			break;
		}

		//書き込み

		len = snprintf(m, 16, "pal%d", i + 1);

		mSetBuf_format(dat, ">ihhhh",
			pnum, 16, 16, 0, len);

		mFILEwriteOK(fpdst, dat, 12);

		mFILEwriteOK(fpdst, m, len);

		mFILEwriteOK(fpdst, buf, size);

		mFree(buf);
	}

ERR:
	fclose(fpdst);
	fclose(fpsrc);
}


//================================
// ブラシ
//================================


/* サイズリスト書き込み */

static void _write_sizelist(FILE *fpsrc,int num)
{
	FILE *fpdst;
	void *buf;
	int size;

	size = num * 2;

	//---- 読み込み

	buf = mMalloc(size);
	if(!buf) return;

	if(mFILEreadOK(fpsrc, buf, size))
	{
		mFree(buf);
		return;
	}

	//--- 書き込み

	fpdst = _open_dstfile("brushsize.dat");
	if(!fpdst)
	{
		mFree(buf);
		fseek(fpsrc, size, SEEK_CUR);
		return;
	}

	fputs("AZPTBRSIZE", fpdst);

	mFILEwriteByte(fpdst, 0);
	mFILEwriteBE16(fpdst, num);
	mFILEwriteOK(fpdst, buf, size);

	//

	mFree(buf);

	fclose(fpdst);
}

/* ブラシアイテム処理 */

static int _proc_brushitem(FILE *fpsrc,FILE *fpdst)
{
	uint8_t flags,vs8[10],vd8[9];
	uint16_t exsize,vs16[12],vd16[12];
	uint32_t pressure_val;
	char *name,*texpath,*shapepath;
	int ret = 0,len_name,len_shape,len_tex;

	name = texpath = shapepath = NULL;

	//読み込み
	// :exsize 分は全体として seek されるので、移動しなくて良い。

	if(mFILEreadByte(fpsrc, &flags)
		|| mFILEreadBE16(fpsrc, &exsize)
		|| mFILEreadBE32(fpsrc, &pressure_val)
		|| mFILEreadArrayBE16(fpsrc, vs16, 12) != 12
		|| mFILEreadOK(fpsrc, vs8, 10)
		|| mFILEreadStr_lenBE16(fpsrc, &name) == -1
		|| mFILEreadStr_lenBE16(fpsrc, &shapepath) == -1
		|| mFILEreadStr_lenBE16(fpsrc, &texpath) == -1)
	{
		//error
		ret = 1;
	}

	//1pxドット線, 指先はスキップ

	if(vs8[0] >= 4) goto END;

	//書き込み

	if(!ret)
	{
		len_name = mStrlen(name);
		len_shape = mStrlen(shapepath);
		len_tex = mStrlen(texpath);

		//タイプとサイズ
		mFILEwriteByte(fpdst, 0);
		mFILEwriteBE32(fpdst, 1 + 2 + len_name + 65 + 2 + len_shape + 2 + len_tex);

		//アイテム共通
		mFILEwriteByte(fpdst, 0);
		mFILEwriteStr_lenBE16(fpdst, name, len_name);

		//ブラシ

		mFILEwriteStr_lenBE16(fpdst, shapepath, len_shape);
		mFILEwriteStr_lenBE16(fpdst, texpath, len_tex);

		vd8[0] = vs8[0]; //type
		vd8[1] = 0; //size unit = px
		vd8[2] = vs8[1]; //opacity
		vd8[3] = vs8[2]; //pixmode
		vd8[4] = (vs8[3] == 0)? 0: 1 + (3 - vs8[3]); //補正タイプ
		vd8[5] = vs8[4]; //補正値
		vd8[6] = vs8[5]; //形状硬さ
		vd8[7] = vs8[6]; //形状砂化
		vd8[8] = vs8[7]; //回転ランダム

		vd16[0] = vs16[0] * 2; //size
		vd16[1] = vs16[1] * 2; //size min
		vd16[2] = vs16[2] * 2; //size max
		vd16[3] = vs16[3]; //pressure size
		vd16[4] = vs16[4]; //pressure opacity
		vd16[5] = vs16[5]; //間隔
		vd16[6] = 1000 - vs16[6]; //ランダムサイズ
		vd16[7] = vs16[7]; //ランダム位置
		vd16[8] = vs16[8]; //角度
		vd16[9] = vs16[9]; //水彩:描画色
		vd16[10] = vs16[11]; //水彩:伸ばす
		vd16[11] = vs8[9]; //flags

		mFILEwriteOK(fpdst, vd8, 9);

		mFILEwriteArrayBE16(fpdst, vd16, 12);

		//pressure
		mFILEwriteBE32(fpdst, 0);
		mFILEwriteBE32(fpdst, ((1<<12)<<16) | (1<<12));
		mFILEwrite0(fpdst, (8-2) * 4);
	}

END:
	mFree(name);
	mFree(shapepath);
	mFree(texpath);

	return ret;
}

/* ブラシデータ処理 */

static void _proc_brushdat(FILE *fpsrc,FILE *fpdst)
{
	uint8_t type,flags,colnum;
	uint16_t size;
	long fpos;
	int len;
	char *name;

	while(mFILEreadByte(fpsrc, &type) == 0 && type != 255)
	{
		if(mFILEreadBE16(fpsrc, &size))
			break;

		fpos = ftell(fpsrc);

		if(type == 0)
		{
			//ブラシ

			if(_proc_brushitem(fpsrc, fpdst))
				break;
		}
		else if(type == 1)
		{
			//グループ

			if(mFILEreadByte(fpsrc, &flags)
				|| mFILEreadByte(fpsrc, &colnum))
				break;

			len = mFILEreadStr_lenBE16(fpsrc, &name);
			if(len == -1) break;

			mFILEwriteByte(fpdst, 254);
			mFILEwriteBE32(fpdst, 2 + 2 + len);

			mFILEwriteByte(fpdst, colnum);
			mFILEwriteByte(fpdst, flags & 1);
			mFILEwriteStr_lenBE16(fpdst, name, len);

			mFree(name);
		}

		fseek(fpsrc, fpos + size, SEEK_SET);
	}

	//終了
	mFILEwriteByte(fpdst, 255);
}

/* ブラシ変換 */

static void _convert_brush(void)
{
	FILE *fpsrc,*fpdst;
	uint16_t ver,sizenum;

	if(_open_file("toollist.dat", "brush-2.dat", &fpdst, &fpsrc))
		return;

	if(mFILEreadStr_compare(fpsrc, "AZPLBRD")
		|| mFILEreadBE16(fpsrc, &ver)
		|| ver != 0x0100
		|| mFILEreadBE16(fpsrc, &sizenum))
		goto ERR;

	//サイズリスト

	_write_sizelist(fpsrc, sizenum);

	//ブラシ

	fputs("AZPTTLIST", fpdst);

	mFILEwriteByte(fpdst, 0);

	_proc_brushdat(fpsrc, fpdst);

ERR:
	fclose(fpdst);
	fclose(fpsrc);
}


//================================
// 変換メイン
//================================


/** ver2->3 変換 */

void ConvertConfigFile(void)
{
	_convert_brush();
	_convert_colorpalette();

	//グラデーションファイルは旧バージョンも読み込み可

	_copyfile("grad.dat", "grad.dat");
}


