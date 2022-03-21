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
 * ツールリストデータ
 *****************************************/

#include <stdio.h>
#include <string.h>

#include "mlk_gui.h"
#include "mlk_list.h"
#include "mlk_str.h"
#include "mlk_stdio.h"
#include "mlk_util.h"

#include "def_draw.h"
#include "def_draw_toollist.h"
#include "def_toolicon.h"
#include "def_pixelmode.h"

#include "toollist.h"
#include "curve_spline.h"

#include "draw_toollist.h"


//----------------

#define BRUSHITEM_MAX_NUM  500
#define _CONFIG_FILENAME   "toollist.dat"

//----------------



/* (group) アイテム破棄ハンドラ */

static void _group_item_destroy(mList *list,mListItem *item)
{
	ToolListGroupItem *pi = (ToolListGroupItem *)item;

	mFree(pi->name);

	mListDeleteAll(&pi->list);
}

/* アイテム破棄ハンドラ */

static void _item_destroy(mList *list,mListItem *item)
{
	ToolListItem *pi = (ToolListItem *)item;
	ToolListItem_brush *brush;

	mFree(pi->name);

	//ブラシ

	if(pi->type == TOOLLIST_ITEMTYPE_BRUSH)
	{
		brush = (ToolListItem_brush *)pi;

		mFree(brush->shape_path);
		mFree(brush->texture_path);
	}
}

/* mStr からアイテム用文字列ポインタ取得 */

static char *_str_to_itemtext(mStr *str)
{
	if(mStrIsEmpty(str))
		return NULL;
	else
		return mStrdup(str->buf);
}

/* ツール用のアイコン番号を取得 */

static int _get_icon_tool(int toolno,int sub)
{
	int icon = 0;

	switch(toolno)
	{
		case TOOL_DOTPEN:
			icon = TOOLICON_DOTPEN;
			break;
		case TOOL_DOTPEN_ERASE:
			icon = TOOLICON_DOTPEN_ERASE;
			break;
		case TOOL_FINGER:
			icon = TOOLICON_FINGER;
			break;
		case TOOL_FILL_POLYGON:
			icon = TOOLICON_FILLPOLY_BOX + sub;
			break;
		case TOOL_FILL_POLYGON_ERASE:
			icon = TOOLICON_FILLPOLYERASE_BOX + sub;
			break;
		case TOOL_FILL:
			icon = TOOLICON_FILL;
			break;
		case TOOL_FILL_ERASE:
			icon = TOOLICON_FILL_ERASE;
			break;
		case TOOL_GRADATION:
			icon = TOOLICON_GRAD_LINE + sub;
			break;
		case TOOL_SELECT_FILL:
			icon = TOOLICON_SELECT_FILL;
			break;
		case TOOL_MOVE:
			icon = TOOLICON_MOVE_CUR + sub;
			break;
		case TOOL_SELECT:
			icon = TOOLICON_SELECT_BOX + sub;
			break;
		case TOOL_CANVAS_MOVE:
			icon = TOOLICON_CANVAS_MOVE;
			break;
		case TOOL_CANVAS_ROTATE:
			icon = TOOLICON_CANVAS_ROTATE;
			break;
		case TOOL_SPOIT:
			icon = TOOLICON_SPOIT_CANVAS + sub;
			break;
	}

	return icon;
}


//===============================


/** グループリスト初期化 */

void ToolList_init(mList *list)
{
	list->item_destroy = _group_item_destroy;
}

/** ツールアイテム用の値を取得
 *
 * subno: 255 でツールの現在値を使用 */

void ToolList_getToolValue(int toolno,int *dst_subno,int *dst_icon,uint32_t *dst_opt)
{
	AppDrawTool *ptool = &APPDRAW->tool;
	int sub,icon;
	uint32_t opt;

	sub = ptool->subno[toolno];
	opt = 0;
	icon = _get_icon_tool(toolno, sub);
	
	switch(toolno)
	{
		case TOOL_DOTPEN:
			opt = ptool->opt_dotpen;
			sub = 255;
			break;
		case TOOL_DOTPEN_ERASE:
			opt = ptool->opt_dotpen_erase;
			sub = 255;
			break;
		case TOOL_FINGER:
			opt = ptool->opt_finger;
			sub = 255;
			break;
		case TOOL_FILL_POLYGON:
			opt = ptool->opt_fillpoly;
			break;
		case TOOL_FILL_POLYGON_ERASE:
			opt = ptool->opt_fillpoly_erase;
			break;
		case TOOL_FILL:
			opt = ptool->opt_fill;
			break;
		/*
		case TOOL_FILL_ERASE:
			break;
		*/
		case TOOL_GRADATION:
			opt = ptool->opt_grad;
			break;
		case TOOL_SELECT_FILL:
			opt = ptool->opt_selfill;
			break;
		/*
		case TOOL_MOVE:
			break;
		case TOOL_SELECT:
			break;
		case TOOL_CANVAS_MOVE:
			break;
		case TOOL_CANVAS_ROTATE:
			break;
		case TOOL_SPOIT:
			break;
		*/
	}

	if(dst_subno) *dst_subno = sub;
	if(dst_icon) *dst_icon = icon;
	if(dst_opt) *dst_opt = opt;
}


//===============================
// グループ
//===============================


/** 新規グループの挿入
 *
 * ins: NULL で追加 */

ToolListGroupItem *ToolList_insertGroup(mList *list,ToolListGroupItem *ins,const char *name,int colnum)
{
	ToolListGroupItem *pi;

	pi = (ToolListGroupItem *)mListInsertNew(list, MLISTITEM(ins), sizeof(ToolListGroupItem));
	if(!pi) return NULL;

	pi->list.item_destroy = _item_destroy;

	pi->name = mStrdup(name);
	pi->colnum = colnum;
	pi->fopened = TRUE;

	return pi;
}


/** 表示時の縦アイテム数取得 (開いている時) */

int ToolListGroupItem_getViewVertNum(ToolListGroupItem *p)
{
	//[!] 挿入用に、終端には常に１個分の空アイテムが入るようにする

	return (p->list.num + p->colnum) / p->colnum;
}

/** グループ内のインデックス番号からアイテム取得 */

ToolListItem *ToolListGroupItem_getItemAtIndex(ToolListGroupItem *p,int index)
{
	return (ToolListItem *)mListGetItemAtIndex(&p->list, index);
}


//===============================
// アイテム
//===============================


/** ブラシを挿入 */

ToolListItem *ToolList_insertItem_brush(ToolListGroupItem *group,ToolListItem *ins,const char *name)
{
	ToolListItem_brush *pi;

	if(group->list.num >= BRUSHITEM_MAX_NUM) return NULL;

	pi = (ToolListItem_brush *)mListInsertNew(&group->list, MLISTITEM(ins), sizeof(ToolListItem_brush));
	if(!pi) return NULL;

	pi->group = group;
	pi->name = mStrdup(name);
	pi->type = TOOLLIST_ITEMTYPE_BRUSH;
	pi->icon = TOOLICON_BRUSH_NORMAL;

	ToolListBrushItem_setDefault(pi);

	return (ToolListItem *)pi;
}

/** ツールを挿入 */

ToolListItem *ToolList_insertItem_tool(ToolListGroupItem *group,ToolListItem *ins,const char *name,int toolno)
{
	ToolListItem_tool *pi;
	uint32_t opt;
	int sub,icon;

	if(group->list.num >= BRUSHITEM_MAX_NUM) return NULL;

	pi = (ToolListItem_tool *)mListInsertNew(&group->list, MLISTITEM(ins), sizeof(ToolListItem_tool));
	if(!pi) return NULL;

	pi->group = group;
	pi->name = mStrdup(name);
	pi->type = TOOLLIST_ITEMTYPE_TOOL;
	pi->toolno = toolno;

	//設定データ
	// sub: 255 でツールリストのサブタイプを使う

	ToolList_getToolValue(toolno, &sub, &icon, &opt);
	
	pi->icon = icon;
	pi->toolopt = opt;
	pi->subno = sub;

	return (ToolListItem *)pi;
}

/** コピーアイテムの貼り付け(挿入) */

ToolListItem *ToolList_pasteItem(ToolListGroupItem *group,ToolListItem *ins,ToolListItem *src)
{
	ToolListItem *pi;

	if(group->list.num >= BRUSHITEM_MAX_NUM) return NULL;

	//確保 & コピー

	pi = ToolListItem_copy(src, NULL);
	if(!pi) return NULL;

	//グループ内にリンク

	pi->group = group;

	mListInsertItem(&group->list, MLISTITEM(ins), MLISTITEM(pi));

	return pi;
}

/** アイテム位置移動 (D&D) */

void ToolList_moveItem(ToolListItem *src,ToolListGroupItem *dst_group,ToolListItem *dst_item)
{
	if(src == dst_item || !dst_group)
		return;

	//同じグループ内の移動時、src < dst の場合は、dst の次の位置へ移動

	if(src->group == dst_group && dst_item)
	{
		if(mListItemGetDir(MLISTITEM(src), MLISTITEM(dst_item)) < 0)
			dst_item = (ToolListItem *)dst_item->i.next;
	}

	//src のグループから src を外す

	mListRemoveItem(&src->group->list, MLISTITEM(src));

	//dst のグループにリンクし直す

	mListInsertItem(&dst_group->list, MLISTITEM(dst_item), MLISTITEM(src));

	src->group = dst_group;
}

/** 確保したアイテムを解放 */

void ToolListItem_free(ToolListItem *p)
{
	if(p)
	{
		_item_destroy(NULL, MLISTITEM(p));

		mFree(p);
	}
}

/** アイテムをコピー
 *
 * edit: src がブラシの場合、NULL 以外で編集用データからコピー
 * return: 確保されたアイテムが返る */

ToolListItem *ToolListItem_copy(ToolListItem *src,BrushEditData *edit)
{
	ToolListItem *pi;
	ToolListItem_brush *pb,*psb;
	int size;

	size = (src->type == TOOLLIST_ITEMTYPE_TOOL)?
		sizeof(ToolListItem_tool): sizeof(ToolListItem_brush);

	//確保

	pi = (ToolListItem *)mMalloc(size);
	if(!pi) return NULL;

	//コピー

	memcpy(pi, src, size);

	pi->name = mStrdup(src->name);

	//ブラシ

	if(pi->type == TOOLLIST_ITEMTYPE_BRUSH)
	{
		pb = (ToolListItem_brush *)pi;
		psb = (ToolListItem_brush *)src;

		if(edit)
		{
			pb->v = edit->v;

			pb->shape_path = _str_to_itemtext(&edit->str_shape);
			pb->texture_path = _str_to_itemtext(&edit->str_texture);
		}
		else
		{
			pb->shape_path = mStrdup(psb->shape_path);
			pb->texture_path = mStrdup(psb->texture_path);
		}
	}

	return pi;
}


//===============================
// ブラシアイテム
//===============================


/** ブラシアイテム、デフォルト値セット */

void ToolListBrushItem_setDefault(ToolListItem_brush *p)
{
	mStrdup_free(&p->texture_path, "?");

	p->v.size = 80;
	p->v.size_min = BRUSH_SIZE_MIN;
	p->v.size_max = 4000;
	p->v.interval = 15;
	p->v.pressure_opacity = 1000;
	p->v.water_param[0] = 300;
	p->v.water_param[1] = 300;

	p->v.brush_type = BRUSH_TYPE_NORMAL;
	p->v.opacity = 100;
	p->v.hosei_type = 0;
	p->v.shape_hard = 100;
	p->v.flags = BRUSH_FLAGS_ANTIALIAS;

	p->v.pt_press_curve[1] = CURVE_SPLINE_VAL_MAXPOS;
}

/** ブラシアイテムの値を編集用データにセット */

void ToolListBrushItem_setEdit(BrushEditData *p,ToolListItem_brush *src)
{
	//値のコピー

	p->v = src->v;

	//文字列のセット

	mStrSetText(&p->str_shape, src->shape_path);
	mStrSetText(&p->str_texture, src->texture_path);
}


//===============================
// BrushEditData (編集用データ)
//===============================


/** BrushEditData 解放 */

void BrushEditData_free(BrushEditData *p)
{
	if(p)
	{
		mStrFree(&p->str_shape);
		mStrFree(&p->str_texture);

		mFree(p);
	}
}

/** 現在のブラシタイプの塗りタイプの数を取得 */

int BrushEditData_getPixelModeNum(BrushEditData *p)
{
	switch(p->v.brush_type)
	{
		case BRUSH_TYPE_ERASE:
			return PIXELMODE_ERASE_NUM;
		default:
			return PIXELMODE_BRUSH_NUM;
	}
}

/** ブラシサイズを調整して取得 */

int BrushEditData_adjustSize(BrushEditData *p,int size)
{
	if(size < p->v.size_min)
		size = p->v.size_min;
	else if(size > p->v.size_max)
		size = p->v.size_max;

	return size;
}


//===============================
// ファイル読み込み
//===============================


/* 文字列を読み込んでサイズを引く */

static int _read_str(FILE *fp,char **dst,uint32_t *psize)
{
	int n;

	n = mFILEreadStr_lenBE16(fp, dst);
	if(n == -1) return 1;

	*psize -= 2 + n;

	return 0;
}

/* グループデータ読み込み */

static mlkbool _read_group(FILE *fp,uint32_t size,ToolListGroupItem **ppdst)
{
	ToolListGroupItem *pi;

	//追加

	pi = ToolList_insertGroup(&APPDRAW->tlist->list_group, NULL, NULL, 1);
	if(!pi) return FALSE;
	
	//読み込み

	if(mFILEreadByte(fp, &pi->colnum)
		|| mFILEreadByte(fp, &pi->fopened)
		|| _read_str(fp, &pi->name, &size))
		return FALSE;

	size -= 2;

	//skip

	if(size)
		fseek(fp, size, SEEK_CUR);

	//

	*ppdst = pi;

	return TRUE;
}

/* アイテム共通データ読み込み
 *
 * psize: 共通データサイズ分が減る */

static mlkbool _read_item_common(FILE *fp,ToolListItem *pi,uint32_t *psize,ToolListItem **ppsel)
{
	uint8_t flags;
	int n;

	//flags, name

	if(mFILEreadByte(fp, &flags)
		|| _read_str(fp, &pi->name, psize))
		return FALSE;

	*psize -= 1;

	//選択アイテム

	if(flags & 1)
		*ppsel = pi;

	//登録アイテム

	n = (flags >> 1) & 15;

	if(n)
		APPDRAW->tlist->regitem[n - 1] = pi;

	return TRUE;
}

/* ツールアイテム読み込み */

static mlkbool _read_item_tool(FILE *fp,ToolListGroupItem *group,uint32_t size,ToolListItem **ppsel)
{
	ToolListItem_tool *pi;
	uint8_t dat[6];

	//追加

	pi = (ToolListItem_tool *)mListAppendNew(&group->list, sizeof(ToolListItem_tool));
	if(!pi) return FALSE;

	pi->group = group;
	pi->type = TOOLLIST_ITEMTYPE_TOOL;

	//共通

	if(!_read_item_common(fp, (ToolListItem *)pi, &size, ppsel))
		return FALSE;

	//データ

	if(mFILEreadOK(fp, dat, 6)) return FALSE;

	mGetBuf_format(dat, ">bbi",
		&pi->toolno, &pi->subno, &pi->toolopt);

	//アイコン

	pi->icon = _get_icon_tool(pi->toolno, pi->subno);

	//skip

	if(size > 6)
		fseek(fp, size - 6, SEEK_CUR);

	return TRUE;
}

/* ブラシアイテム読み込み */

static mlkbool _read_item_brush(FILE *fp,ToolListGroupItem *group,uint32_t size,ToolListItem **ppsel)
{
	ToolListItem_brush *pi;
	uint8_t dat[65],*ps;
	int i;

	//追加

	pi = (ToolListItem_brush *)mListAppendNew(&group->list, sizeof(ToolListItem_brush));
	if(!pi) return FALSE;

	pi->group = group;
	pi->type = TOOLLIST_ITEMTYPE_BRUSH;

	//共通

	if(!_read_item_common(fp, (ToolListItem *)pi, &size, ppsel))
		return FALSE;

	//ブラシ形状パス, テクスチャパス

	if(_read_str(fp, &pi->shape_path, &size)
		|| _read_str(fp, &pi->texture_path, &size))
		return FALSE;

	//---- データ

	if(mFILEreadOK(fp, dat, 65)) return FALSE;

	size -= 65;

	ps = dat;

	//1byte

	pi->v.brush_type = ps[0];
	pi->v.unit_size = ps[1];
	pi->v.opacity = ps[2];
	pi->v.pixelmode = ps[3];
	pi->v.hosei_type = ps[4];
	pi->v.hosei_val = ps[5];
	pi->v.shape_hard = ps[6];
	pi->v.shape_sand = ps[7];
	pi->v.rand_angle = ps[8];

	ps += 9;

	//2byte

	ps += mGetBuf_format(ps, ">hhhhhhhhhhhh",
		&pi->v.size, &pi->v.size_min, &pi->v.size_max, &pi->v.pressure_size, &pi->v.pressure_opacity,
		&pi->v.interval, &pi->v.rand_size, &pi->v.rand_pos, &pi->v.angle_base,
		&pi->v.water_param[0], &pi->v.water_param[1], &pi->v.flags);

	//4byte

	for(i = 0; i < BRUSH_PRESSURE_PT_NUM; i++, ps += 4)
		pi->v.pt_press_curve[i] = mGetBufBE32(ps);

	//---- アイコン

	pi->icon = TOOLICON_BRUSH_NORMAL + pi->v.brush_type;

	//--- skip

	if(size)
		fseek(fp, size, SEEK_CUR);

	return TRUE;
}

/** ファイルから読み込み
 *
 * 追加時、登録アイテムがあれば上書き。
 *
 * fadd: TRUE=追加、FALSE=初期読み込み */

void ToolList_loadFile(const char *filename,mlkbool fadd)
{
	FILE *fp;
	ToolListGroupItem *group = NULL;
	ToolListItem *pisel = NULL;
	uint8_t ver,type;
	uint32_t size;
	int ret;

	fp = mFILEopen(filename, "rb");
	if(!fp) return;

	//ヘッダ

	if(mFILEreadStr_compare(fp, "AZPTTLIST")
		|| mFILEreadByte(fp, &ver)
		|| ver != 0)
		goto END;

	//データ

	while(1)
	{
		//type, size
		
		if(mFILEreadByte(fp, &type)
			|| type == 255
			|| mFILEreadBE32(fp, &size))
			break;

		//

		if(type == 254)
		{
			//グループ
			ret = _read_group(fp, size, &group);
		}
		else
		{
			//アイテム

			if(!group) break;
			
			if(type == TOOLLIST_ITEMTYPE_BRUSH)
				ret = _read_item_brush(fp, group, size, &pisel);
			else if(type == TOOLLIST_ITEMTYPE_TOOL)
				ret = _read_item_tool(fp, group, size, &pisel);
			else
				ret = FALSE;
		}

		if(!ret) break;
	}

	//選択アイテム

	if(!fadd && pisel)
		drawToolList_setSelItem(APPDRAW, pisel);

END:
	fclose(fp);
}

/** 設定ファイル読み込み */

void ToolList_loadConfigFile(void)
{
	mStr str = MSTR_INIT;

	mGuiGetPath_config(&str, _CONFIG_FILENAME);

	ToolList_loadFile(str.buf, FALSE);

	mStrFree(&str);
}


//===============================
// ファイル保存
//===============================


/* アイテム共通書き込み
 *
 * size: 共通データのサイズを除くサイズ */

static void _write_item_common(FILE *fp,ToolListItem *pi,int size)
{
	int len;
	uint8_t flags;

	len = mStrlen(pi->name);

	//データサイズ

	mFILEwriteBE32(fp, 1 + 2 + len + size);

	//flags

	flags = (drawToolList_getRegistItemNo(pi) + 1) << 1;

	if(pi == APPDRAW->tlist->selitem)
		flags |= 1;

	mFILEwriteByte(fp, flags);

	//名前

	mFILEwriteStr_lenBE16(fp, pi->name, len);
}

/* ツールアイテム書き込み */

static void _write_item_tool(FILE *fp,ToolListItem_tool *pi)
{
	uint8_t dat[6];

	_write_item_common(fp, (ToolListItem *)pi, 6);

	mSetBuf_format(dat, ">bbi",
		pi->toolno, pi->subno, pi->toolopt);

	mFILEwriteOK(fp, dat, 6);
}

/* ブラシアイテム書き込み */

static void _write_item_brush(FILE *fp,ToolListItem_brush *pi)
{
	int slen,tlen,i;
	uint8_t dat[65],*pd;

	pd = dat;

	slen = mStrlen(pi->shape_path);
	tlen = mStrlen(pi->texture_path);

	_write_item_common(fp, (ToolListItem *)pi, 65 + 2 + slen + 2 + tlen);

	//ブラシ形状パス

	mFILEwriteStr_lenBE16(fp, pi->shape_path, slen);

	//テクスチャパス

	mFILEwriteStr_lenBE16(fp, pi->texture_path, tlen);

	//----- データ

	//1byte

	pd[0] = pi->v.brush_type;
	pd[1] = pi->v.unit_size;
	pd[2] = pi->v.opacity;
	pd[3] = pi->v.pixelmode;
	pd[4] = pi->v.hosei_type;
	pd[5] = pi->v.hosei_val;
	pd[6] = pi->v.shape_hard;
	pd[7] = pi->v.shape_sand;
	pd[8] = pi->v.rand_angle;

	pd += 9;

	//2byte

	pd += mSetBuf_format(pd, ">hhhhhhhhhhhh",
		pi->v.size, pi->v.size_min, pi->v.size_max, pi->v.pressure_size, pi->v.pressure_opacity,
		pi->v.interval, pi->v.rand_size, pi->v.rand_pos, pi->v.angle_base,
		pi->v.water_param[0], pi->v.water_param[1], pi->v.flags);

	//4byte

	for(i = 0; i < BRUSH_PRESSURE_PT_NUM; i++, pd += 4)
		mSetBufBE32((uint8_t *)pd, pi->v.pt_press_curve[i]);

	mFILEwriteOK(fp, dat, 65);
}

/* グループデータ書き込み */

static void _write_group(FILE *fp,ToolListGroupItem *gi)
{
	ToolListItem *pi;
	int len;

	//---- グループデータ

	//長さはある程度制限されている
	len = mStrlen(gi->name);

	//type
	mFILEwriteByte(fp, 254);
	//size
	mFILEwriteBE32(fp, 1 + 1 + 2 + len);

	//横に並べる数
	mFILEwriteByte(fp, gi->colnum);
	//flags
	mFILEwriteByte(fp, gi->fopened);
	//名前
	mFILEwriteStr_lenBE16(fp, gi->name, len);

	//----- アイテム

	TOOLLIST_FOR_ITEM(gi, pi)
	{
		//type
		mFILEwriteByte(fp, pi->type);

		//

		if(pi->type == TOOLLIST_ITEMTYPE_BRUSH)
			_write_item_brush(fp, (ToolListItem_brush *)pi);
		else
			_write_item_tool(fp, (ToolListItem_tool *)pi);
	}
}

/** ファイルに保存
 *
 * group: 保存するグループ単体。NULL で全体を保存。*/

void ToolList_saveFile(const char *filename,ToolListGroupItem *group)
{
	FILE *fp;

	fp = mFILEopen(filename, "wb");
	if(!fp) return;

	//ヘッダ

	fputs("AZPTTLIST", fp);
	
	mFILEwriteByte(fp, 0);

	//データ

	if(group)
		_write_group(fp, group);
	else
	{
		TOOLLIST_FOR_GROUP(group)
		{
			_write_group(fp, group);
		}
	}

	//終了

	mFILEwriteByte(fp, 255);

	fclose(fp);
}

/** 設定ファイルに保存 */

void ToolList_saveConfigFile(void)
{
	mStr str = MSTR_INIT;

	mGuiGetPath_config(&str, _CONFIG_FILENAME);

	ToolList_saveFile(str.buf, NULL);

	mStrFree(&str);
}

