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
 * AppDraw:ツールリスト
 *****************************************/

#include <string.h>

#include "mlk.h"
#include "mlk_list.h"
#include "mlk_str.h"

#include "def_draw.h"
#include "def_draw_toollist.h"
#include "def_brushdraw.h"
#include "def_pixelmode.h"
#include "def_toolicon.h"

#include "toollist.h"
#include "materiallist.h"

#include "draw_toollist.h"



/* 選択アイテム変更時の更新 */

static void _update_selitem(AppDraw *p)
{
	ToolListItem *item = p->tlist->selitem;

	if(item && item->type == TOOLLIST_ITEMTYPE_BRUSH)
	{
		//編集用データにセット

		ToolListBrushItem_setEdit(p->tlist->brush, (ToolListItem_brush *)item);

		//描画用パラメータセット

		drawToolList_setBrushDrawParam(-1);
	}
}


//============================
//
//============================


/** 現在の選択がブラシアイテムではないか */

mlkbool drawToolList_isnotBrushItem(void)
{
	return (!APPDRAW->tlist->selitem
		|| APPDRAW->tlist->selitem->type != TOOLLIST_ITEMTYPE_BRUSH);
}

/** 登録アイテムの場合、番号を返す
 *
 * return: -1 で登録アイテムではない */

int drawToolList_getRegistItemNo(ToolListItem *item)
{
	ToolListItem **reg = APPDRAW->tlist->regitem;
	int i;

	for(i = 0; i < DRAW_TOOLLIST_REG_NUM; i++)
	{
		if(reg[i] == item)
			return i;
	}

	return -1;
}

/** グループ内に一つでも登録アイテムがあるか (描画時用) */

mlkbool drawToolList_isRegistItem_inGroup(ToolListGroupItem *group)
{
	ToolListItem **reg = APPDRAW->tlist->regitem;
	int i;

	for(i = 0; i < DRAW_TOOLLIST_REG_NUM; i++)
	{
		if(reg[i] && reg[i]->group == group)
			return TRUE;
	}

	return FALSE;
}

/** 現在のブラシデータを取得 (編集用)
 *
 * return: NULL で選択なし、またはツールアイテム */

BrushEditData *drawToolList_getBrush(void)
{
	ToolListItem *sel = APPDRAW->tlist->selitem;

	if(!sel || sel->type != TOOLLIST_ITEMTYPE_BRUSH)
		return NULL;
	else
		return APPDRAW->tlist->brush;
}

/** 選択アイテム変更
 *
 * item: NULL で選択なし
 * return: 選択が変更されたか */

mlkbool drawToolList_setSelItem(AppDraw *p,ToolListItem *item)
{
	if(item == p->tlist->selitem)
		return FALSE;
	else
	{
		//選択変更

		p->tlist->last_selitem = p->tlist->selitem;
	
		p->tlist->selitem = item;

		_update_selitem(p);

		return TRUE;
	}
}

/** 次/前のアイテムを選択
 *
 * return: 選択が変更されたか */

mlkbool drawToolList_selectMove(AppDraw *p,mlkbool next)
{
	ToolListItem *item = p->tlist->selitem;
	ToolListGroupItem *gi;

	gi = item->group;

	if(!item)
		return FALSE;
	else if(next)
	{
		//次
		
		item = (ToolListItem *)item->i.next;

		if(!item)
		{
			//終端なら、次のグループの先頭
			
			for(gi = (ToolListGroupItem *)gi->i.next; gi; gi = (ToolListGroupItem *)gi->i.next)
			{
				if(gi->list.top)
				{
					item = (ToolListItem *)gi->list.top;
					break;
				}
			}
		}
	}
	else
	{
		item = (ToolListItem *)item->i.prev;

		if(!item)
		{
			//先頭なら、前のグループの終端
			
			for(gi = (ToolListGroupItem *)gi->i.prev; gi; gi = (ToolListGroupItem *)gi->i.prev)
			{
				if(gi->list.bottom)
				{
					item = (ToolListItem *)gi->list.bottom;
					break;
				}
			}
		}
	}

	if(!item) return FALSE;

	//変更

	return drawToolList_setSelItem(p, item);
}

/** 直前の選択アイテムと切り替え */

mlkbool drawToolList_toggleSelect_last(AppDraw *p)
{
	if(!p->tlist->last_selitem)
		return FALSE;
	else
		return drawToolList_setSelItem(p, p->tlist->last_selitem);
}

/** アイテム削除
 *
 * 選択アイテムだった場合、削除後は、グループ内の前後のアイテムが選択される。
 *
 * return: TRUE で選択アイテムが変更された */

mlkbool drawToolList_deleteItem(ToolListItem *item)
{
	AppDrawToolList *p;
	ToolListItem *selnext = NULL;
	int i;

	p = APPDRAW->tlist;

	//削除後の選択アイテム

	if(item == p->selitem)
	{
		selnext = (ToolListItem *)item->i.next;
		if(!selnext) selnext = (ToolListItem *)item->i.prev;
	}

	//登録アイテムなら解除

	i = drawToolList_getRegistItemNo(item);

	if(i != -1)
		p->regitem[i] = NULL;

	//前回の選択アイテム

	if(item == p->last_selitem)
		p->last_selitem = NULL;
	
	//削除

	mListDelete(&item->group->list, MLISTITEM(item));

	//選択変更

	if(item != p->selitem)
		return FALSE;
	else
	{
		//選択アイムの削除時
		// :last_selitem は変更なし
		
		p->selitem = selnext;

		_update_selitem(APPDRAW);
		
		return TRUE;
	}
}

/** グループアイテムの削除
 *
 * グループ内に選択アイテムが含まれる場合、選択アイテムは"なし"になる。 */

void drawToolList_deleteGroup(ToolListGroupItem *item)
{
	AppDrawToolList *p = APPDRAW->tlist;
	int i;

	//選択アイテムが含まれる場合、選択をなしに

	if(p->selitem && p->selitem->group == item)
		drawToolList_setSelItem(APPDRAW, NULL);

	//前回の選択アイテムが含まれる場合、なしに

	if(p->last_selitem && p->last_selitem->group == item)
		p->last_selitem = NULL;

	//登録アイテムが含まれる場合、解除

	for(i = 0; i < DRAW_TOOLLIST_REG_NUM; i++)
	{
		if(p->regitem[i] && p->regitem[i]->group == item)
			p->regitem[i] = NULL;
	}

	//削除

	mListDelete(&p->list_group, MLISTITEM(item));
}

/** 貼り付け用にアイテムコピー */

void drawToolList_copyItem(AppDraw *p,ToolListItem *item)
{
	BrushEditData *edit;

	ToolListItem_free(p->tlist->copyitem);

	//コピーするアイテムが選択ブラシの場合、編集用データからコピー

	if(item == p->tlist->selitem
		&& item->type == TOOLLIST_ITEMTYPE_BRUSH)
		edit = p->tlist->brush;
	else
		edit = NULL;

	//確保してコピー

	p->tlist->copyitem = ToolListItem_copy(item, edit);
}

/** 登録アイテムにセット
 *
 * no: -1 でなし */

void drawToolList_setRegistItem(ToolListItem *item,int no)
{
	int sno;

	sno = drawToolList_getRegistItemNo(item);

	//解除

	if(sno >= 0)
		APPDRAW->tlist->regitem[sno] = NULL;

	//セット

	if(no >= 0)
		APPDRAW->tlist->regitem[no] = item;
}

/** 登録アイテムをすべて解除 */

void drawToolList_releaseRegistAll(void)
{
	mMemset0(APPDRAW->tlist->regitem, sizeof(void *) * DRAW_TOOLLIST_REG_NUM);
}


//============================
// 描画用パラメータ
//============================


/* 半径サイズを取得 */

static double _get_drawparam_radius(BrushParam *pv)
{
	double d;

	d = pv->size * 0.05; //size = 直径

	if(pv->unit_size == BRUSH_UNIT_SIZE_PX)
		return d;
	else
	{
		//mm
		
		d = d / 25.4 * APPDRAW->imgdpi;

		if(d > 600) d = 600;

		return d;
	}
}

/* 形状の硬さパラメータ値取得 */

static double _get_drawparam_shape_hard(int val)
{
	if(val <= 50)
		return (-0.4 - 0.7) * (val * 0.02) + 0.7;
	else
		return (-5 + 0.4) * ((val - 50) * 0.02) - 0.4;
}

/* 砂化のパラメータ値取得 */

static int _get_drawparam_shape_sand(int val)
{
	if(val == 0)
		return 0;
	else if(val <= 50)
		return val * 2;
	else if(val <= 80)
		return 100 + (val - 50) * 10;
	else if(val <= 90)
		return 300 + (val - 80) * 400;
	else
		return 5000 + (val - 90) * 1000;
}

/* ぼかしの半径値取得 */

static int _get_drawparam_blur_range(int val)
{
	if(val < 25)
		return 1;
	else if(val < 50)
		return 2;
	else if(val < 75)
		return 3;
	else
		return 4;
}

/* 筆圧カーブのセット
 *
 * [!] フラグは直接変更する。 */

static void _set_drawparam_pressure(BrushDrawParam *dp,BrushParam *pv,mlkbool is_reg)
{
	uint32_t *pt,*ptreg;
	int i,fmake;

	if(pv->flags & BRUSH_FLAGS_PRESSURE_COMMON)
		pt = APPDRAW->tlist->pt_press_comm;
	else
		pt = pv->pt_press_curve;

	//

	if((pt[1] >> 16) == CURVE_SPLINE_POS_VAL)
	{
		//線形補間

		dp->pressure_min = (double)(pt[0] & 0xffff) / CURVE_SPLINE_POS_VAL;
		dp->pressure_range = (double)(pt[1] & 0xffff) / CURVE_SPLINE_POS_VAL - dp->pressure_min;

		dp->flags &= ~BRUSHDP_F_PRESSURE_CURVE;
	}
	else
	{
		//----- スプライン補間

		fmake = TRUE;

		//登録ブラシの場合、前回と同じ値ならそのまま

		if(is_reg)
		{
			ptreg = APPDRAW->tlist->pt_press_reg;
			
			for(i = 0; i < CURVE_SPLINE_MAXNUM; i++)
			{
				if(pt[i] != ptreg[i]) break;
			}

			fmake = (i != CURVE_SPLINE_MAXNUM);

			//生成する場合、新しい値をセット

			if(fmake)
				memcpy(ptreg, pt, 4 * CURVE_SPLINE_MAXNUM);
		}

		//テーブル生成

		if(fmake)
		{
			CurveSpline_setCurveTable(&APPDRAW->tlist->curve_spline,
				dp->press_curve_tbl, pt, 0);
		}

		dp->flags |= BRUSHDP_F_PRESSURE_CURVE;
	}
}

/** ブラシデータから描画用パラメータセット
 *
 * regno: -1 で現在のブラシ。0〜で登録ブラシ */

void drawToolList_setBrushDrawParam(int regno)
{
	BrushDrawParam *dst;
	BrushParam *pv;
	ToolListItem_brush *pireg;
	BrushEditData *pedit;
	uint32_t flags = 0;

	if(regno >= 0)
	{
		//登録ブラシ
		
		dst = APPDRAW->tlist->dp_reg;
		pireg = (ToolListItem_brush *)APPDRAW->tlist->regitem[regno];
		pedit = NULL;
		pv = &pireg->v;
	}
	else
	{
		//現在のブラシ
		
		dst = APPDRAW->tlist->dp_cur;
		pireg = NULL;
		pedit = APPDRAW->tlist->brush;
		pv = &APPDRAW->tlist->brush->v;
	}

	//--------

	dst->radius = _get_drawparam_radius(pv);
	
	dst->opacity = pv->opacity * 0.01;
	dst->interval = pv->interval * 0.01;
	dst->min_size = pv->pressure_size * 0.001;
	dst->min_opacity = pv->pressure_opacity * 0.001;

	//ぼかし半径

	if(pv->brush_type == BRUSH_TYPE_BLUR)
		dst->blur_range = _get_drawparam_blur_range(pv->opacity);

	//アンチエイリアス

	if(!(pv->flags & BRUSH_FLAGS_ANTIALIAS))
		flags |= BRUSHDP_F_NO_ANTIALIAS;

	//曲線補間
	
	if(pv->flags & BRUSH_FLAGS_CURVE)
		flags |= BRUSHDP_F_CURVE;

	//矩形上書き

	if(pv->pixelmode == PIXELMODE_OVERWRITE_SQUARE)
		flags |= BRUSHDP_F_OVERWRITE_TP;

	//ランダムサイズ

	if(pv->rand_size)
	{
		dst->rand_size_min = 1.0 - pv->rand_size * 0.001;
		flags |= BRUSHDP_F_RANDOM_SIZE;
	}

	//ランダム位置

	if(pv->rand_pos)
	{
		dst->rand_pos_len = pv->rand_pos * 0.02;
		flags |= BRUSHDP_F_RANDOM_POS;
	}

	//水彩

	if(pv->brush_type == BRUSH_TYPE_WATER)
		flags |= BRUSHDP_F_WATER;

	dst->water_param[0] = pv->water_param[0] * 0.001;
	dst->water_param[1] = pv->water_param[1] * 0.001;

	if(pv->flags & BRUSH_FLAGS_WATER_TP_WHITE)
		flags |= BRUSHDP_F_WATER_TP_WHITE;

	//ブラシ形状

	dst->shape_hard = _get_drawparam_shape_hard(pv->shape_hard);
	dst->shape_sand = _get_drawparam_shape_sand(pv->shape_sand);

	if(pv->shape_hard == 100)
		flags |= BRUSHDP_F_SHAPE_HARD_MAX;

	dst->angle = pv->angle_base * 512 / 360;  //0-511
	dst->angle_rand = pv->rand_angle * 256 / 180;

	if(pv->flags & BRUSH_FLAGS_ROTATE_DIR)
		flags |= BRUSHDP_F_TRAVELLING_DIR;

	//ブラシ画像

	dst->img_shape = MaterialList_getImage(APPDRAW->list_material,
		MATERIALLIST_TYPE_BRUSH_SHAPE,
		(pireg)? pireg->shape_path: pedit->str_shape.buf, FALSE);

	//テクスチャ画像

	dst->img_texture = MaterialList_getImage(APPDRAW->list_material,
		MATERIALLIST_TYPE_BRUSH_TEXTURE,
		(pireg)? pireg->texture_path: pedit->str_texture.buf, FALSE);

	//フラグ

	dst->flags = flags;

	//筆圧カーブ
	// [!] フラグは直接変更するため、フラグセット後に行うこと。

	_set_drawparam_pressure(dst, pv, (regno >= 0));
}

/** ブラシ描画時の情報を取得
 *
 * regno: 負の値で現在のブラシ、0〜で登録ブラシ
 * return: テクスチャのイメージ */

ImageMaterial *drawToolList_getBrushDrawInfo(int regno,BrushDrawParam **ppdraw,BrushParam **ppparam)
{
	AppDrawToolList *p = APPDRAW->tlist;
	BrushDrawParam *dp;
	ToolListItem_brush *pibrush;
	char *pctex;
	ImageMaterial *imgtex;

	//登録ツール
	// :登録ツール=選択ブラシの場合は、選択ブラシのデータを使う

	if(regno >= 0)
	{
		pibrush = (ToolListItem_brush *)p->regitem[regno];

		if(pibrush == (ToolListItem_brush *)p->selitem)
			regno = -1;
	}

	//

	if(regno < 0)
	{
		//選択ブラシ

		dp = p->dp_cur;
		*ppparam = &p->brush->v;

		pctex = p->brush->str_texture.buf;

		//半径サイズ
		// :ブラシサイズの単位が mm の場合、DPI によって半径値が異なるため、
		// :描画時に計算する。

		dp->radius = _get_drawparam_radius(&p->brush->v);
	}
	else
	{
		//登録ブラシ (選択ブラシでない場合)

		dp = p->dp_reg;
		*ppparam = &pibrush->v;

		pctex = pibrush->texture_path;

		drawToolList_setBrushDrawParam(regno);
	}

	*ppdraw = dp;

	//テクスチャ

	if(pctex == NULL || *pctex == 0)
		//強制なし
		imgtex = NULL;
	else if(*pctex == '?')
		//オプションのテクスチャ
		imgtex = APPDRAW->imgmat_opttex;
	else
		imgtex = dp->img_texture;

	return imgtex;
}


//============================
// ブラシ項目値変更
//============================
/*
  - 常に保存時 (手動保存 OFF 時) は、選択アイテムの値も変更。
  - dp_cur の描画用パラメータも更新する。
*/


#define _SELBRUSH  ((ToolListItem_brush *)APPDRAW->tlist->selitem)
#define _REGBRUSH  ((ToolListItem_brush *)APPDRAW->tlist->regitem[APPDRAW->w.brush_regno])
#define _EDITBRUSH (APPDRAW->tlist->brush)
#define _DRAWPARAM (APPDRAW->tlist->dp_cur)


/* ブラシ値の変更が無効か */

static mlkbool _is_no_setparam(void)
{
	return (!APPDRAW->tlist->selitem
		|| APPDRAW->tlist->selitem->type != TOOLLIST_ITEMTYPE_BRUSH);
}

/* 選択アイテムが常に保存タイプか */

static mlkbool _is_autosave(void)
{
	return (APPDRAW->tlist->selitem
		&& (_SELBRUSH->v.flags & BRUSH_FLAGS_AUTO_SAVE));
}

/* dp_cur->flags のフラグを ON/OFF
 *
 * return: on の値をそのまま返す */

static mlkbool _dpcur_flag_onoff(uint32_t flags,int on)
{
	if(on)
		APPDRAW->tlist->dp_cur->flags |= flags;
	else
		APPDRAW->tlist->dp_cur->flags &= ~flags;

	return on;
}

/* mStr の文字列を選択ブラシの文字列にセット */

static void _set_str_to_text(char **ppdst,mStr *str)
{
	if(!mStrCompareEq(str, *ppdst))
	{
		mFree(*ppdst);

		if(mStrIsEmpty(str))
			*ppdst = NULL;
		else
			*ppdst = mStrdup(str->buf);
	}
}

/** 現在の値を保存
 *
 * return: TRUE で、タイプが変更されたため、アイコン更新 */

mlkbool drawBrushParam_save(void)
{
	BrushEditData *ps = _EDITBRUSH;
	ToolListItem_brush *pi;
	mlkbool ret = FALSE;

	if(_is_no_setparam()) return FALSE;

	if (APPDRAW->w.brush_regno >= 0) {
		pi = _REGBRUSH;
	} else {
		pi = _SELBRUSH;
	}

	//タイプが変更された場合、アイコン変更

	if(pi->v.brush_type != ps->v.brush_type)
	{
		pi->icon = TOOLICON_BRUSH_NORMAL + ps->v.brush_type;
		ret = TRUE;
	}

	//適用

	pi->v = ps->v;

	_set_str_to_text(&pi->shape_path, &ps->str_shape);
	_set_str_to_text(&pi->texture_path, &ps->str_texture);

	return ret;
}

/** ツールリストパネル上での現在値を保存 */

void drawBrushParam_saveSimple(void)
{
	BrushEditData *ps = _EDITBRUSH;
	ToolListItem_brush *pi = _SELBRUSH;

	if(_is_no_setparam()) return;

	pi->v.size = ps->v.size;
	pi->v.size_min = ps->v.size_min;
	pi->v.size_max = ps->v.size_max;
	pi->v.opacity = ps->v.opacity;
}

/** 常に保存を切り替え
 *
 * return: フラグ (bit0:保存ボタンが有効か, bit1:アイコン変更)  */

int drawBrushParam_toggleAutoSave(void)
{
	int fon,ret = 0;

	if(_is_no_setparam()) return FALSE;

	_SELBRUSH->v.flags ^= BRUSH_FLAGS_AUTO_SAVE;
	_EDITBRUSH->v.flags ^= BRUSH_FLAGS_AUTO_SAVE;

	fon = _EDITBRUSH->v.flags & BRUSH_FLAGS_AUTO_SAVE;

	//ON になった場合、現在値保存

	if(fon)
	{
		if(drawBrushParam_save())
			ret |= 2;
	}

	//OFF の場合、保存ボタン有効

	if(!fon) ret |= 1;

	return ret;
}

/** タイプ変更
 *
 * return: TRUE でアイコン変更によりリストを更新 */

mlkbool drawBrushParam_setType(int val)
{
	ToolListItem_brush *pi;
	mlkbool ret = FALSE;

	if(_is_no_setparam()) return FALSE;

	//塗りタイプはクリア

	_EDITBRUSH->v.brush_type = val;
	_EDITBRUSH->v.pixelmode = 0;
	
	if(_is_autosave())
	{
		pi = _SELBRUSH;
		
		pi->v.brush_type = val;
		pi->v.pixelmode = 0;

		//アイコン確定

		pi->icon = TOOLICON_BRUSH_NORMAL + val;
		ret = TRUE;
	}

	//DP:ぼかし半径

	if(val == BRUSH_TYPE_BLUR)
		_DRAWPARAM->blur_range = _get_drawparam_blur_range(_EDITBRUSH->v.opacity);

	//DP:水彩フラグ

	_dpcur_flag_onoff(BRUSHDP_F_WATER, (val == BRUSH_TYPE_WATER));

	return ret;
}

/** サイズ */

void drawBrushParam_setSize(int val)
{
	if(_is_no_setparam()) return;

	val = BrushEditData_adjustSize(_EDITBRUSH, val);

	_EDITBRUSH->v.size = val;

	if(_is_autosave())
		_SELBRUSH->v.size = val;

	//[!] 半径値は描画時に計算する
}

/** サイズの単位 */

void drawBrushParam_setSizeUnit(int val)
{
	if(_is_no_setparam()) return;

	_EDITBRUSH->v.unit_size = val;

	if(_is_autosave())
		_SELBRUSH->v.unit_size = val;
}

/** サイズの設定 */

void drawBrushParam_setSizeOpt(int min,int max)
{
	BrushEditData *pb = _EDITBRUSH;
	int size;

	if(_is_no_setparam()) return;

	pb->v.size_min = min;
	pb->v.size_max = max;

	if(_is_autosave())
	{
		_SELBRUSH->v.size_min = min;
		_SELBRUSH->v.size_max = max;
	}

	//現在のサイズが範囲外の場合、範囲内に調整

	size = pb->v.size;

	if(size < min)
		size = min;
	else if(size > max)
		size = max;

	if(size != pb->v.size)
	{
		pb->v.size = size;
	
		if(_is_autosave())
			_SELBRUSH->v.size = size;
	}
}

/** 濃度 */

void drawBrushParam_setOpacity(int val)
{
	if(_is_no_setparam()) return;

	_EDITBRUSH->v.opacity = val;

	if(_is_autosave())
		_SELBRUSH->v.opacity = val;

	//DP:濃度

	_DRAWPARAM->opacity = val * 0.01;

	//DP:ぼかし半径

	if(_EDITBRUSH->v.brush_type == BRUSH_TYPE_BLUR)
		_DRAWPARAM->blur_range = _get_drawparam_blur_range(val);
}

/** 手ブレ補正タイプ */

void drawBrushParam_setHoseiType(int val)
{
	if(_is_no_setparam()) return;

	_EDITBRUSH->v.hosei_type = val;

	if(_is_autosave())
		_SELBRUSH->v.hosei_type = val;
}

/** 手ブレ補正強さ */

void drawBrushParam_setHoseiVal(int val)
{
	if(_is_no_setparam()) return;

	_EDITBRUSH->v.hosei_val = val;

	if(_is_autosave())
		_SELBRUSH->v.hosei_val = val;
}

/** 塗りタイプ */

void drawBrushParam_setPixelMode(int val)
{
	if(_is_no_setparam()) return;

	_EDITBRUSH->v.pixelmode = val;

	if(_is_autosave())
		_SELBRUSH->v.pixelmode = val;

	//DP:ブラシ形状の透明部分も描画するか

	_dpcur_flag_onoff(BRUSHDP_F_OVERWRITE_TP, (val == PIXELMODE_OVERWRITE_SQUARE));
}

/** 間隔 */

void drawBrushParam_setInterval(int val)
{
	if(_is_no_setparam()) return;

	_EDITBRUSH->v.interval = val;

	if(_is_autosave())
		_SELBRUSH->v.interval = val;

	//DP:間隔

	_DRAWPARAM->interval = val * 0.01;
}

/** テクスチャパス */

void drawBrushParam_setTexturePath(const char *path)
{
	if(_is_no_setparam()) return;

	mStrSetText(&_EDITBRUSH->str_texture, path);

	if(_is_autosave())
		_set_str_to_text(&_SELBRUSH->texture_path, &_EDITBRUSH->str_texture);

	//DP:イメージ

	_DRAWPARAM->img_texture = MaterialList_getImage(APPDRAW->list_material,
		MATERIALLIST_TYPE_BRUSH_TEXTURE, path, FALSE);
}

/** アンチエイリアス  */

void drawBrushParam_toggleAntialias(void)
{
	if(_is_no_setparam()) return;

	_EDITBRUSH->v.flags ^= BRUSH_FLAGS_ANTIALIAS;

	if(_is_autosave())
		_SELBRUSH->v.flags ^= BRUSH_FLAGS_ANTIALIAS;

	//DP:フラグ

	_DRAWPARAM->flags ^= BRUSHDP_F_NO_ANTIALIAS;
}

/** 曲線補間  */

void drawBrushParam_toggleCurve(void)
{
	if(_is_no_setparam()) return;

	_EDITBRUSH->v.flags ^= BRUSH_FLAGS_CURVE;

	if(_is_autosave())
		_SELBRUSH->v.flags ^= BRUSH_FLAGS_CURVE;

	//DP:フラグ

	_DRAWPARAM->flags ^= BRUSHDP_F_CURVE;
}

/** ランダムサイズ */

void drawBrushParam_setRandomSize(int val)
{
	if(_is_no_setparam()) return;

	_EDITBRUSH->v.rand_size = val;

	if(_is_autosave())
		_SELBRUSH->v.rand_size = val;

	//DP:ランダムサイズ最小

	if(_dpcur_flag_onoff(BRUSHDP_F_RANDOM_SIZE, (val != 0)))
		_DRAWPARAM->rand_size_min = 1.0 - val * 0.001;
}

/** ランダム位置 */

void drawBrushParam_setRandomPos(int val)
{
	if(_is_no_setparam()) return;

	_EDITBRUSH->v.rand_pos = val;

	if(_is_autosave())
		_SELBRUSH->v.rand_pos = val;

	//DP:ランダム位置

	if(_dpcur_flag_onoff(BRUSHDP_F_RANDOM_POS, (val != 0)))
		_DRAWPARAM->rand_pos_len = val * 0.02;
}

/** 水彩パラメータ */

void drawBrushParam_setWaterParam(int no,int val)
{
	if(_is_no_setparam()) return;

	_EDITBRUSH->v.water_param[no] = val;

	if(_is_autosave())
		_SELBRUSH->v.water_param[no] = val;

	//DP:水彩パラメータ

	_DRAWPARAM->water_param[no] = val * 0.001;
}

/** 水彩:透明色は白とする */

void drawBrushParam_toggleWaterTpWhite(void)
{
	if(_is_no_setparam()) return;

	_EDITBRUSH->v.flags ^= BRUSH_FLAGS_WATER_TP_WHITE;

	if(_is_autosave())
		_SELBRUSH->v.flags ^= BRUSH_FLAGS_WATER_TP_WHITE;

	//DP:フラグ

	_DRAWPARAM->flags ^= BRUSHDP_F_WATER_TP_WHITE;
}

/** ブラシ画像パス */

void drawBrushParam_setShapePath(const char *path)
{
	if(_is_no_setparam()) return;

	mStrSetText(&_EDITBRUSH->str_shape, path);

	if(_is_autosave())
		_set_str_to_text(&_SELBRUSH->shape_path, &_EDITBRUSH->str_shape);

	//DP:イメージ

	_DRAWPARAM->img_shape = MaterialList_getImage(APPDRAW->list_material,
		MATERIALLIST_TYPE_BRUSH_SHAPE,
		path, FALSE);
}

/** 形状の硬さ */

void drawBrushParam_setShapeHard(int val)
{
	if(_is_no_setparam()) return;

	_EDITBRUSH->v.shape_hard = val;

	if(_is_autosave())
		_SELBRUSH->v.shape_hard = val;

	//DP:硬さパラメータ、フラグ

	_DRAWPARAM->shape_hard = _get_drawparam_shape_hard(val);

	_dpcur_flag_onoff(BRUSHDP_F_SHAPE_HARD_MAX, (val == 100));
}

/** 形状の砂化 */

void drawBrushParam_setShapeSand(int val)
{
	if(_is_no_setparam()) return;

	_EDITBRUSH->v.shape_sand = val;

	if(_is_autosave())
		_SELBRUSH->v.shape_sand = val;

	//DP:砂化パラメータ

	_DRAWPARAM->shape_sand = _get_drawparam_shape_sand(val);
}

/** 回転のベース角度 */

void drawBrushParam_setAngleBase(int val)
{
	if(_is_no_setparam()) return;

	_EDITBRUSH->v.angle_base = val;

	if(_is_autosave())
		_SELBRUSH->v.angle_base = val;

	//DP:角度

	_DRAWPARAM->angle = val * 512 / 360;
}

/** 回転ランダム角度 */

void drawBrushParam_setAngleRandom(int val)
{
	if(_is_no_setparam()) return;

	_EDITBRUSH->v.rand_angle = val;

	if(_is_autosave())
		_SELBRUSH->v.rand_angle = val;

	//DP:角度

	_DRAWPARAM->angle_rand = val * 256 / 180;
}

/** 進行方向に回転 */

void drawBrushParam_toggleShapeRotateDir(void)
{
	if(_is_no_setparam()) return;

	_EDITBRUSH->v.flags ^= BRUSH_FLAGS_ROTATE_DIR;

	if(_is_autosave())
		_SELBRUSH->v.flags ^= BRUSH_FLAGS_ROTATE_DIR;

	//DP:フラグ

	_DRAWPARAM->flags ^= BRUSHDP_F_TRAVELLING_DIR;
}

/** 筆圧0のサイズ */

void drawBrushParam_setPressureSize(int val)
{
	if(_is_no_setparam()) return;

	_EDITBRUSH->v.pressure_size = val;

	if(_is_autosave())
		_SELBRUSH->v.pressure_size = val;

	//DP:最小サイズ

	_DRAWPARAM->min_size = val * 0.001;
}

/** 筆圧0の濃度 */

void drawBrushParam_setPressureOpacity(int val)
{
	if(_is_no_setparam()) return;

	_EDITBRUSH->v.pressure_opacity = val;

	if(_is_autosave())
		_SELBRUSH->v.pressure_opacity = val;

	//DP:最小濃度

	_DRAWPARAM->min_opacity = val * 0.001;
}

/** 共通の筆圧カーブ切り替え */

void drawBrushParam_togglePressureCommon(void)
{
	if(_is_no_setparam()) return;

	_EDITBRUSH->v.flags ^= BRUSH_FLAGS_PRESSURE_COMMON;

	if(_is_autosave())
		_SELBRUSH->v.flags ^= BRUSH_FLAGS_PRESSURE_COMMON;

	//DP:筆圧

	_set_drawparam_pressure(_DRAWPARAM, &_EDITBRUSH->v, FALSE);
}

/** 筆圧カーブの位置変更時
 *
 * 位置データはすでにセットされている。 */

void drawBrushParam_changePressureCurve(void)
{
	if(_is_autosave())
	{
		memcpy(_SELBRUSH->v.pt_press_curve, _EDITBRUSH->v.pt_press_curve,
			CURVE_SPLINE_MAXNUM * 4);
	}

	//DP:筆圧

	_set_drawparam_pressure(_DRAWPARAM, &_EDITBRUSH->v, FALSE);
}

