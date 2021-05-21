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

/***************************
 * ヘルプ
 ***************************/

//グループ
enum
{
	HELP_TRGROUP_SINGLE = 0
};

//単一ヘルプ
enum
{
	HELP_TRID_COLORPALETTEVIEW,	//カラーパレット
	HELP_TRID_LAYER_COLOR,		//レイヤ色選択
	HELP_TRID_LAYER_LIST,		//レイヤ一覧操作
	HELP_TRID_RULE,				//定規
	HELP_TRID_TOOL_CUTPASTE,	//(ツール)切り貼り
	HELP_TRID_TEXT_DIALOG,		//テキストダイアログ
	HELP_TRID_REGFONT_CHAREDIT,	//テキスト、登録フォント文字編集
	HELP_TRID_TOOL_TEXT,		//テキストツール
	HELP_TRID_BRUSH_PRESSURE,	//ブラシ筆圧編集
	HELP_TRID_SEL_MATERIAL,		//画像素材選択
	HELP_TRID_HEADTAIL,			//入り抜き
	HELP_TRID_CANVASKEY,		//キャンバスキー設定
	HELP_TRID_ENVOPT_UNDOBUFSIZE	//環境設定:アンドゥバッファサイズ
};


void AppHelp_message(mWindow *parent,int groupid,int trid);
