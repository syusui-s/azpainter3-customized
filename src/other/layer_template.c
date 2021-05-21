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
 * レイヤテンプレート
 *****************************************/

#include <stdio.h>

#include "mlk_gui.h"
#include "mlk_list.h"
#include "mlk_str.h"
#include "mlk_stdio.h"
#include "mlk_util.h"

#include "def_config.h"


//----------------

#define _FILENAME "layertempl.dat"

//----------------


/* ファイルを開く */

static FILE *_open_file(const char *mode)
{
	mStr str = MSTR_INIT;
	FILE *fp;

	mGuiGetPath_config(&str, _FILENAME);

	fp = mFILEopen(str.buf, mode);

	mStrFree(&str);

	return fp;
}

/** ファイルに保存 */

void LayerTemplate_saveFile(void)
{
	FILE *fp;
	mList *list = &APPCONF->list_layertempl;
	ConfigLayerTemplItem *pi;
	uint8_t buf[17];

	fp = _open_file("wb");
	if(!fp) return;

	fputs("AZPLYTM", fp);

	//ver
	mFILEwriteByte(fp, 0);

	//個数
	mFILEwriteBE16(fp, list->num);

	//

	for(pi = (ConfigLayerTemplItem *)list->top; pi; pi = (ConfigLayerTemplItem *)pi->i.next)
	{
		mSetBuf_format(buf, ">hhbbbibhhb",
			pi->name_len, pi->texture_len,
			pi->type, pi->blendmode, pi->opacity, pi->col,
			pi->flags, pi->tone_lines, pi->tone_angle, pi->tone_density);

		fwrite(buf, 1, 17, fp);

		if(pi->name_len)
			fwrite(pi->text, 1, pi->name_len, fp);

		if(pi->texture_len)
			fwrite(pi->text + pi->name_len + 1, 1, pi->texture_len, fp);
	}

	fclose(fp);
}

/* 文字列読み込み */

static int _read_text(FILE *fp,char *buf,int len)
{
	if(len)
	{
		if(fread(buf, 1, len, fp) != len)
			return 1;
	}

	return 0;
}

/** ファイルから読み込み */

void LayerTemplate_loadFile(void)
{
	FILE *fp;
	mList *list = &APPCONF->list_layertempl;
	ConfigLayerTemplItem *pi;
	uint8_t ver,buf[17];
	uint16_t num,name_len,tex_len;

	fp = _open_file("rb");
	if(!fp) return;

	//

	if(mFILEreadStr_compare(fp, "AZPLYTM")
		|| mFILEreadByte(fp, &ver)
		|| ver != 0
		|| mFILEreadBE16(fp, &num))
		goto END;

	//

	for(; num; num--)
	{
		if(fread(buf, 1, 17, fp) != 17)
			break;

		//文字数

		mGetBuf_format(buf, ">hh", &name_len, &tex_len);

		//リスト追加

		pi = (ConfigLayerTemplItem *)mListAppendNew(list,
			sizeof(ConfigLayerTemplItem) + name_len + tex_len + 1);

		if(!pi) break;

		//データ

		mGetBuf_format(buf + 4, ">bbbibhhb",
			&pi->type, &pi->blendmode, &pi->opacity, &pi->col,
			&pi->flags, &pi->tone_lines, &pi->tone_angle, &pi->tone_density);

		pi->name_len = name_len;
		pi->texture_len = tex_len;

		if(_read_text(fp, pi->text, name_len)
			|| _read_text(fp, pi->text + name_len + 1, tex_len))
			break;
	}

END:
	fclose(fp);
}


