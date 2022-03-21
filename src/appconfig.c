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
 * AppConfig 関連の関数
 *****************************************/

#include <string.h>

#include "mlk_gui.h"
#include "mlk_str.h"
#include "mlk_list.h"
#include "mlk_file.h"
#include "mlk_dir.h"
#include "mlk_util.h"

#include "def_macro.h"
#include "def_config.h"

#include "appconfig.h"


//------------------

//水彩プリセット:初期値
static const uint32_t g_water_preset[] = {
	500|(800<<10), 500|(600<<10), 400|(500<<10), 300|(300<<10), 100|(700<<10)
};

//------------------


/** 解放 */

void AppConfig_free(void)
{
	AppConfig *p = APPCONF;
	
	if(!p) return;

	mStrFree(&p->strFont_panel);
	mStrFree(&p->strFileDlgDir);
	mStrFree(&p->strTempDir);
	mStrFree(&p->strTempDirProc);
	mStrFree(&p->strImageViewerDir);
	mStrFree(&p->strLayerFileDir);
	mStrFree(&p->strSelectFileDir);
	mStrFree(&p->strFontFileDir);
	mStrFree(&p->strUserTextureDir);
	mStrFree(&p->strUserBrushDir);
	mStrFree(&p->strCursorFile);

	mStrArrayFree(p->strRecentFile, CONFIG_RECENTFILE_NUM);
	mStrArrayFree(p->strRecentOpenDir, CONFIG_RECENTDIR_NUM);
	mStrArrayFree(p->strRecentSaveDir, CONFIG_RECENTDIR_NUM);

	mListDeleteAll(&p->list_layertempl);
	mListDeleteAll(&p->list_textword);
	mListDeleteAll(&p->list_filterparam);

	mFree(p->toolbar_btts);

	mFree(p);

	APPCONF = NULL;
}

/** AppConfig 作成 */

int AppConfig_new(void)
{
	APPCONF = (AppConfig *)mMalloc0(sizeof(AppConfig));

	return (APPCONF == NULL);
}

/* 作業用ディレクトリ作成
 *
 * strTmpDirProc に実際のディレクトリパスセット。 */

static void _create_tempdir(AppConfig *p)
{
	mStr *pstr;
	char *name;

	pstr = &APPCONF->strTempDirProc;

	//'<tmpdir>/azpainter-<procid>'

	mStrCopy(pstr,  &APPCONF->strTempDir);
	mStrPathJoin(pstr, "azpainter-");

	name = mGetProcessName();
	mStrAppendText(pstr, name);
	mFree(name);

	//すでに存在していた場合、削除

	if(mIsExistDir(pstr->buf))
		AppConfig_deleteTempDir();

	//作成 (失敗時は、パスを空に)

	if(mCreateDir(pstr->buf, -1) != MLKERR_OK)
		mStrFree(pstr);
}

/** 設定ファイル読み込み後の設定 */

void AppConfig_set_afterConfig(void)
{
	AppConfig *p = APPCONF;

	//---- 値の調整

	//イメージキャンバス:左右反転は、起動時常にOFF

	p->imgviewer.flags &= ~CONFIG_IMAGEVIEWER_F_MIRROR;

	//素材ディレクトリが空の場合、初期値セット

	if(mStrIsEmpty(&p->strUserTextureDir))
		mGuiGetPath_config(&p->strUserTextureDir, APP_DIRNAME_TEXTURE);

	if(mStrIsEmpty(&p->strUserBrushDir))
		mGuiGetPath_config(&p->strUserBrushDir, APP_DIRNAME_BRUSH);

	//作業用ディレクトリが空の場合、初期値セット

	if(mStrIsEmpty(&APPCONF->strTempDir))
		mStrPathSetTempDir(&APPCONF->strTempDir);

	//---- 作業用ディレクトリ作成

	_create_tempdir(p);
}

/** 水彩プリセットをデフォルトにセット */

void AppConfig_setWaterPreset_default(void)
{
	memcpy(APPCONF->panel.water_preset, g_water_preset, CONFIG_WATER_PRESET_NUM * 4);
}

/** 作業用ディレクトリ削除 */

void AppConfig_deleteTempDir(void)
{
	mDir *dir;
	mStr str = MSTR_INIT;

	//パスが空なら、作成に失敗している

	if(mStrIsEmpty(&APPCONF->strTempDirProc))
		return;

	//残っているファイルを削除

	dir = mDirOpen(APPCONF->strTempDirProc.buf);
	if(!dir) return;

	while(mDirNext(dir))
	{
		if(!mDirIsDirectory(dir))
		{
			mDirGetFilename_str(dir, &str, TRUE);
			mDeleteFile(str.buf);
		}
	}

	mDirClose(dir);

	mStrFree(&str);

	//ディレクトリを削除

	mDeleteDir(APPCONF->strTempDirProc.buf);
}

/** 作業用ディレクトリのパス取得
 *
 * path: 追加するパス
 * return: FALSE でパスなし (作業用ディレクトリなし) */

mlkbool AppConfig_getTempPath(mStr *str,const char *path)
{
	if(mStrIsEmpty(&APPCONF->strTempDirProc))
		return FALSE;
	else
	{
		mStrCopy(str, &APPCONF->strTempDirProc);
		mStrPathJoin(str, path);

		return TRUE;
	}
}

