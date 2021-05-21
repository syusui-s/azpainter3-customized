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
 * GUI スタイル設定
 *****************************************/

#include <stdio.h>
#include <string.h>

#include "mlk_gui.h"
#include "mlk_guicol.h"
#include "mlk_str.h"
#include "mlk_iniread.h"
#include "mlk_iniwrite.h"
#include "mlk_file.h"
#include "mlk_fontinfo.h"
#include "mlk_guistyle.h"
#include "mlk_dir.h"

#include "mlk_pv_gui.h"


//----------------------

static const char *g_color_name[] = {
	"face", "face_disable", "face_select", "face_select_unfocus", "face_select_light",
	"face_textbox", "face_dark", "frame", "frame_box", "frame_focus", "frame_disable",
	"grid", "text", "text_disable", "text_select", "button_face", "button_face_press",
	"button_face_default", "scrollbar_face", "scrollbar_grip",
	"menu_face", "menu_frame", "menu_sep", "menu_select",
	"menu_text", "menu_text_disable", "menu_text_shortcut",
	"tooltip_face", "tooltip_frame", "tooltip_text"
};

//----------------------


#if !defined(MLK_NO_ICONTHEME)

/* 指定ディレクトリがアイコンテーマか */

static mlkbool _is_dir_icontheme(const char *path)
{
	mIniRead *ini;
	mlkbool ret = FALSE;

	if(mIniRead_loadFile_join(&ini, path, "index.theme") == MLKERR_OK)
	{
		ret = (mIniRead_setGroup(ini, "icon theme")
			&& mIniRead_getText(ini, "directories", NULL));
	}

	mIniRead_end(ini);

	return ret;
}

/* ディレクトリから、テーマを検索 */

static mlkbool _find_icontheme(mAppBase *p,const char *path,mlkbool ishome)
{
	mDir *dir;
	const char *name;
	mStr str = MSTR_INIT;
	mlkbool ret = FALSE;

	if(ishome)
		mStrPathSetHome_join(&str, path);

	//

	dir = mDirOpen((ishome)? str.buf: path);
	if(dir)
	{
		while(mDirNext(dir))
		{
			if(mDirIsSpecName(dir)
				|| !mDirIsDirectory(dir))
				continue;

			name = mDirGetFilename(dir);

			if(strcmp(name, "hicolor") == 0)
				continue;

			mDirGetFilename_str(dir, &str, TRUE);

			if(_is_dir_icontheme(str.buf))
			{
				p->style.icontheme = mStrdup(name);
				ret = TRUE;
				break;
			}
		}

		mDirClose(dir);
	}

	mStrFree(&str);

	return ret;
}

/* デフォルトのアイコンテーマを検索 */

static void _find_default_icontheme(mAppBase *p)
{
	if(_find_icontheme(p, "/usr/share/icons", FALSE)) return;

	if(_find_icontheme(p, "/usr/local/share/icons", FALSE)) return;

	if(_find_icontheme(p, ".local/share/icons", TRUE)) return;

	_find_icontheme(p, ".icons", TRUE);
}

#endif //MLK_NO_ICONTHEME

/* GTK+3 のアイコンテーマを読み込み */

/*
static void _read_icontheme_gtk3(mAppBase *p)
{
	mIniRead *ini;
	mStr str = MSTR_INIT;

	mStrPathSetHome_join(&str, ".config/gtk-3.0/settings.ini");

	mIniRead_loadFile(&ini, str.buf);

	mStrFree(&str);

	if(!ini) return;

	//

	mIniRead_setGroup(ini, "Settings");

	p->style.icontheme = mIniRead_getText_dup(ini, "gtk-icon-theme-name", NULL);

	mIniRead_end(ini);
}
*/

/* 設定ファイルから読み込み (起動時) */

static mlkbool _load_style_startup(mAppBase *p,const char *filename)
{
	mIniRead *ini;
	int i;
	uint32_t col;

	//ファイルが存在しない

	if(!mIsExistFile(filename)) return FALSE;

	//

	mIniRead_loadFile(&ini, filename);
	if(!ini) return FALSE;

	mIniRead_setGroup(ini, "style");

	//フォント

	p->style.fontstr = mIniRead_getText_dup(ini, "font", NULL);

	//アイコンテーマ

	p->style.icontheme = mIniRead_getText_dup(ini, "icon_theme", NULL);

	//色

	mIniRead_setGroup(ini, "color");

	for(i = 1; i < MGUICOL_NUM; i++)
	{
		col = mIniRead_getHex(ini, g_color_name[i - 1], (uint32_t)-1);

		if(col != (uint32_t)-1)
			mGuiCol_setRGBColor(i, col);
	}

	mIniRead_end(ini);

	return TRUE;
}

/** スタイルデータ解放 */

void __mGuiStyleFree(mAppStyleData *p)
{
	mFree(p->fontstr);
	mFree(p->icontheme);

	p->fontstr = NULL;
	p->icontheme = NULL;
}

/** 起動時、設定ファイル読み込み */

void __mGuiStyleLoad(mAppBase *p)
{
	mStr str = MSTR_INIT;

	//設定ファイル読み込み
	
	mGuiGetPath_config(&str, "az-mlk-style.conf");

	if(!_load_style_startup(p, str.buf))
	{
		//config

		mStrPathSetHome_join(&str, ".config/az-mlk-style.conf");

		_load_style_startup(p, str.buf);
	}

	mStrFree(&str);

	//アイコンテーマの指定がない場合

#if !defined(MLK_NO_ICONTHEME)
	if(!p->style.icontheme)
		_find_default_icontheme(p);
#endif
}


//==========================
// スタイル編集用
//==========================


/**@ スタイルデータを解放 */

void mGuiStyle_free(mGuiStyleInfo *p)
{
	mFontInfoFree(&p->fontinfo);
	mStrFree(&p->strIcontheme);
}

/**@ スタイルを読み込み
 *
 * @d:それぞれの項目で、指定がない場合は値を変更しない */

void mGuiStyle_load(const char *filename,mGuiStyleInfo *info)
{
	mIniRead *ini;
	const char *text;
	int i;
	uint32_t col;

	mIniRead_loadFile(&ini, filename);
	if(!ini) return;

	mIniRead_setGroup(ini, "style");

	//フォント

	text = mIniRead_getText(ini, "font", NULL);
	if(text)
		mFontInfoSetFromText(&info->fontinfo, text);

	//アイコンテーマ

	text = mIniRead_getText(ini, "icon_theme", NULL);
	if(text)
		mStrSetText(&info->strIcontheme, text);

	//色

	mIniRead_setGroup(ini, "color");

	for(i = 1; i < MGUICOL_NUM; i++)
	{
		col = mIniRead_getHex(ini, g_color_name[i - 1], (uint32_t)-1);

		if(col != (uint32_t)-1)
			info->col[i] = col;
	}

	mIniRead_end(ini);
}

/**@ 色を読み込んで、GUI色に適用 */

void mGuiStyle_loadColor(const char *filename)
{
	mIniRead *ini;
	int i;
	uint32_t col;

	mIniRead_loadFile(&ini, filename);
	if(!ini) return;

	mIniRead_setGroup(ini, "color");

	for(i = 1; i < MGUICOL_NUM; i++)
	{
		col = mIniRead_getHex(ini, g_color_name[i - 1], (uint32_t)-1);

		if(col != (uint32_t)-1)
			mGuiCol_setRGBColor(i, col);
	}

	mIniRead_end(ini);
}

/**@ スタイルを保存 */

void mGuiStyle_save(const char *filename,mGuiStyleInfo *info)
{
	mStr str = MSTR_INIT;
	FILE *fp;
	int i;

	fp = mIniWrite_openFile(filename);
	if(!fp) return;

	mIniWrite_putGroup(fp, "style");

	//フォント (空の場合は出力しない)

	if(info->fontinfo.mask)
	{
		mFontInfoToText(&str, &info->fontinfo);

		mIniWrite_putStr(fp, "font", &str);
	}

	//アイコンテーマ

	if(mStrIsnotEmpty(&info->strIcontheme))
		mIniWrite_putStr(fp, "icon_theme", &info->strIcontheme);

	//色

	mIniWrite_putGroup(fp, "color");

	for(i = 1; i < MGUICOL_NUM; i++)
		mIniWrite_putHex(fp, g_color_name[i - 1], info->col[i]);

	fclose(fp);

	mStrFree(&str);
}

/**@ 現在のスタイル情報を取得 */

void mGuiStyle_getCurrentInfo(mGuiStyleInfo *info)
{
	//フォント

	mFontInfoSetFromText(&info->fontinfo, MLKAPP->style.fontstr);

	//アイコンテーマ

	mStrSetText(&info->strIcontheme, MLKAPP->style.icontheme);

	//色

	mGuiStyle_getCurrentColor(info);
}

/**@ 現在の GUI 色を取得 */

void mGuiStyle_getCurrentColor(mGuiStyleInfo *info)
{
	int i;

	for(i = 1; i < MGUICOL_NUM; i++)
		info->col[i] = mGuiCol_getRGB(i);
}

/**@ 色を適用 */

void mGuiStyle_setColor(mGuiStyleInfo *info)
{
	int i;

	for(i = 1; i < MGUICOL_NUM; i++)
		mGuiCol_setRGBColor(i, info->col[i]);

	mGuiCol_RGBtoPix_all();
}
