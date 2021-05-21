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

/***************************************
 * キャンバスキー
 ***************************************/

enum
{
	CANVASKEY_NONE = 0,
	CANVASKEY_CMD_TOOL = 1,			//ツール変更
	CANVASKEY_CMD_DRAWTYPE = 30,	//描画タイプ変更
	CANVASKEY_CMD_OTHER = 50,		//他コマンド
	CANVASKEY_OP_TOOL = 100,		//+キー:ツール動作
	CANVASKEY_OP_DRAWTYPE = 130,	//+キー:描画タイプ
	CANVASKEY_OP_SELECT = 140,		//+キー:選択範囲ツール
	CANVASKEY_OP_REGIST = 150,		//+キー:登録ツール
	CANVASKEY_OP_OTHER = 160,		//+キー:他

	CANVASKEY_CMD_OTHER_NUM = 12,	//他コマンドの数
	CANVASKEY_OP_OTHER_NUM = 3		//+キー:他の数
};

//他コマンド
enum
{
	CANVASKEY_CMD_OTHER_UNDO,
	CANVASKEY_CMD_OTHER_REDO,
	CANVASKEY_CMD_OTHER_ZOOM_UP,
	CANVASKEY_CMD_OTHER_ZOOM_DOWN,
	CANVASKEY_CMD_OTHER_ROTATE_RESET,
	CANVASKEY_CMD_OTHER_TOGGLE_DRAWCOL,
	CANVASKEY_CMD_OTHER_LAYER_SEL_UP,
	CANVASKEY_CMD_OTHER_LAYER_SEL_DOWN,
	CANVASKEY_CMD_OTHER_LAYER_TOGGLE_VISIBLE,
	CANVASKEY_CMD_OTHER_TOOLLIST_NEXT,
	CANVASKEY_CMD_OTHER_TOOLLIST_PREV,
	CANVASKEY_CMD_OTHER_TOOLLIST_TOGGLE_SEL
};

//+キーほか
enum
{
	CANVASKEY_OP_OTHER_CANVAS_ZOOM,
	CANVASKEY_OP_OTHER_BRUSHSIZE,
	CANVASKEY_OP_OTHER_GET_LAYER
};


//+キー操作のコマンドか
#define CANVASKEY_IS_OPERATE(id)  ((id) >= CANVASKEY_OP_TOOL && (id) < CANVASKEY_OP_OTHER + CANVASKEY_OP_OTHER_NUM)


