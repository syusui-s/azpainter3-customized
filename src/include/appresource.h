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

/***********************************
 * アプリ用リソース管理
 ***********************************/

typedef struct
{
	mImageList *imglist_icon_other,	//その他用アイコン
		*imglist_toolbar,			//ツールバー
		*imglist_paneltool_tool,	//(パネル)ツール
		*imglist_paneltool_sub,		//(パネル)ツール:サブ
		*imglist_panelopt_type,		//(パネル)オプション:タイプ
		*imglist_panelcolor_type,	//(パネル)カラー:タイプの選択 [18x13]
		*imglist_panelcolpal_type,	//(パネル)カラーパレット:タイプ [20x14]
		*imglist_panellayer_check,	//(パネル)レイヤー:パラメータのチェック
		*imglist_panellayer_cmd;	//(パネル)レイヤー:コマンド
}AppResource;

extern AppResource *g_app_resource;
#define APPRES  g_app_resource


#define APPRES_NUMBER_5x9_WIDTH (5 * 15)
#define APPRES_MENUBTT_SIZE  7

//その他アイコン
enum
{
	APPRES_OTHERICON_HOME,
	APPRES_OTHERICON_OPEN,
	APPRES_OTHERICON_PREV,
	APPRES_OTHERICON_NEXT,
	APPRES_OTHERICON_ZOOM,
	APPRES_OTHERICON_FIT,
	APPRES_OTHERICON_FLIP_HORZ
};

//リスト編集用アイコン (変更時、TRID も修正すること)
enum
{
	APPRES_LISTEDIT_ADD,
	APPRES_LISTEDIT_DEL,
	APPRES_LISTEDIT_UP,
	APPRES_LISTEDIT_DOWN,
	APPRES_LISTEDIT_RENAME,
	APPRES_LISTEDIT_COPY,
	APPRES_LISTEDIT_EDIT,
	APPRES_LISTEDIT_OPEN,
	APPRES_LISTEDIT_SAVE,
	APPRES_LISTEDIT_MOVE
};

//ファイルアイコン
enum
{
	APPRES_FILEICON_PARENT,
	APPRES_FILEICON_DIR,
	APPRES_FILEICON_FILE
};

//----------

void AppResource_init(void);
void AppResource_free(void);

const char *AppResource_getOpenFileFilter_normal(void);
mFuncEmpty *AppResource_getLoadImageChecks_normal(void);
const uint8_t *AppResource_getToolBarDefaultBtts(void);

const uint8_t *AppResource_get1bitImg_number5x9(void);
const uint8_t *AppResource_get1bitImg_menubtt(void);

mImageList *AppResource_createImageList_listedit(void);
mImageList *AppResource_createImageList_fileicon(void);
mImageList *AppResource_createImageList_ruleicon(void);

