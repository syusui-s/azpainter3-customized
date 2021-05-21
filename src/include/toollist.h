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
 * ツールリストデータ
 *****************************************/

typedef struct _ToolListGroupItem ToolListGroupItem;
typedef struct _ToolListItem ToolListItem;
typedef struct _ToolListItem_brush ToolListItem_brush;
typedef struct _BrushEditData BrushEditData;

#define BRUSH_PRESSURE_PT_NUM   8

/* グループアイテム */

struct _ToolListGroupItem
{
	mListItem i;

	mList list;		//アイテムリスト
	char *name;		//名前
	uint8_t colnum,	//横に並べる数
		fopened;	//開いている
};

/* アイテム (base) */

struct _ToolListItem
{
	mListItem i;
	ToolListGroupItem *group; //親グループ
	char *name;
	uint8_t type,
		icon;
};

//アイテムタイプ
enum
{
	TOOLLIST_ITEMTYPE_BRUSH,
	TOOLLIST_ITEMTYPE_TOOL
};

/* アイテム (tool) */

typedef struct
{
	mListItem i;
	ToolListGroupItem *group;
	char *name;
	uint8_t type,
		icon;

	uint8_t toolno,		//ツール番号
		subno;			//ツールサブ番号 (255 で、ツールリストのサブタイプを使う)
	uint32_t toolopt;	//ツールの設定
}ToolListItem_tool;

/* ブラシ用パラメータ値 */

typedef struct
{
	uint16_t size,		//直径サイズ [1=0.1]
		size_min,		//サイズ最小 (バー用)
		size_max,		//サイズ最大
		pressure_size,		//筆圧0のサイズ(%) [1=0.1]
		pressure_opacity,	//筆圧0の濃度(%) [1=0.1]
		interval,		//点の間隔 [1=0.01]
		rand_size,		//ランダムサイズ [1=0.1]
		rand_pos,		//ランダム位置 [1=0.01]
		angle_base,		//回転ベース角度 (0-359)
		water_param[2],	//水彩パラメータ
		flags;			//フラグ
	uint8_t brush_type,	//ブラシタイプ
		unit_size,		//サイズの単位
		opacity,		//濃度 (0-100)
		pixelmode,		//塗りタイプ
		hosei_type,		//手ブレ補正タイプ
		hosei_val,		//手ブレ補正強さ (0-)
		shape_hard,		//形状硬さ (0-100)
		shape_sand,		//砂化強さ (0-100)
		rand_angle;		//回転ランダム幅 (0-180)
	uint32_t pt_press_curve[BRUSH_PRESSURE_PT_NUM];
		//筆圧カーブの点 (上位16bit=X,下位16bit=Y) ([1] 以降が 0 で終わり。なければ最大数)
}BrushParam;

/* アイテム (brush) */

struct _ToolListItem_brush
{
	mListItem i;
	ToolListGroupItem *group;
	char *name;
	uint8_t type,
		icon;

	char *shape_path,	//ブラシ形状ファイルパス (空で円形)
		*texture_path;	//テクスチャファイルパス (空でなし[強制]、"?" でオプション指定を使う)

	BrushParam v;
};

/* 編集用ブラシデータ */

struct _BrushEditData
{
	BrushParam v;
	mStr str_shape,
		str_texture;
};

//ブラシタイプ
enum
{
	BRUSH_TYPE_NORMAL,
	BRUSH_TYPE_ERASE,
	BRUSH_TYPE_WATER,
	BRUSH_TYPE_BLUR
};

//サイズ単位
enum
{
	BRUSH_UNIT_SIZE_PX,
	BRUSH_UNIT_SIZE_MM
};

//フラグ
enum
{
	BRUSH_FLAGS_AUTO_SAVE = 1<<0,	//値の変化時、常に保存
	BRUSH_FLAGS_ROTATE_DIR = 1<<1,	//進行方向に回転
	BRUSH_FLAGS_ANTIALIAS = 1<<2,	//アンチエイリアス
	BRUSH_FLAGS_CURVE = 1<<3,		//曲線補間
	BRUSH_FLAGS_WATER_TP_WHITE = 1<<4,	//水彩時、透明色は白とする
	BRUSH_FLAGS_PRESSURE_COMMON = 1<<5	//共通の筆圧カーブ
};

//値の範囲など
enum
{
	BRUSH_SIZE_MIN = 1,
	BRUSH_SIZE_MAX = 10000,
	BRUSH_INTERVAL_MIN = 5,
	BRUSH_INTERVAL_MAX = 1000,
	BRUSH_RAND_POS_MAX = 5000,
	BRUSH_HOSEI_VAL_MAX = 20
};

//

#define TOOLLIST_FOR_GROUP(pi)   for(pi = (ToolListGroupItem *)g_app_draw->tlist->list_group.top; pi; pi = (ToolListGroupItem *)pi->i.next)
#define TOOLLIST_FOR_ITEM(gi,pi) for(pi = (ToolListItem *)gi->list.top; pi; pi = (ToolListItem *)pi->i.next)


//----------------

void ToolList_init(mList *list);
void ToolList_getToolValue(int toolno,int *dst_subno,int *dst_icon,uint32_t *dst_opt);

/* group */

ToolListGroupItem *ToolList_insertGroup(mList *list,ToolListGroupItem *ins,const char *name,int colnum);

int ToolListGroupItem_getViewVertNum(ToolListGroupItem *p);
ToolListItem *ToolListGroupItem_getItemAtIndex(ToolListGroupItem *p,int index);

/* item */

ToolListItem *ToolList_insertItem_brush(ToolListGroupItem *group,ToolListItem *ins,const char *name);
ToolListItem *ToolList_insertItem_tool(ToolListGroupItem *group,ToolListItem *ins,const char *name,int toolno);
ToolListItem *ToolList_pasteItem(ToolListGroupItem *group,ToolListItem *ins,ToolListItem *src);
void ToolList_moveItem(ToolListItem *src,ToolListGroupItem *dst_group,ToolListItem *dst_item);

void ToolListItem_free(ToolListItem *p);
ToolListItem *ToolListItem_copy(ToolListItem *src,BrushEditData *edit);

/* brushitem */

void ToolListBrushItem_setDefault(ToolListItem_brush *p);
void ToolListBrushItem_setEdit(BrushEditData *p,ToolListItem_brush *src);

/* BrushEditData */

void BrushEditData_free(BrushEditData *p);
int BrushEditData_getPixelModeNum(BrushEditData *p);
int BrushEditData_adjustSize(BrushEditData *p,int size);

/* file */

void ToolList_loadFile(const char *filename,mlkbool fadd);
void ToolList_loadConfigFile(void);
void ToolList_saveFile(const char *filename,ToolListGroupItem *group);
void ToolList_saveConfigFile(void);
