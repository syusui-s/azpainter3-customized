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
 * mContainer
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_button.h"

#include "mlk_pv_widget.h"


//---------------------

void __mLayoutCalcHorz(mWidget *);
void __mLayoutCalcVert(mWidget *);
void __mLayoutCalcGrid(mWidget *);

void __mLayoutHorz(mWidget *);
void __mLayoutVert(mWidget *);
void __mLayoutGrid(mWidget *);

//---------------------


/**@ コンテナ作成
 *
 * @g:mContainer
 *
 * @d:デフォルトで垂直コンテナとなる。 */

mWidget *mContainerNew(mWidget *parent,int size)
{
	mContainer *p;
	
	if(size < sizeof(mContainer))
		size = sizeof(mContainer);
	
	p = (mContainer *)mWidgetNew(parent, size);
	if(!p) return NULL;
	
	mContainerSetType(p, MCONTAINER_TYPE_VERT);
	
	p->wg.ftype |= MWIDGET_TYPE_CONTAINER;
	p->wg.draw = mWidgetDrawHandle_bkgnd;
	p->wg.calc_hint = NULL;
	
	//コンテナの場合、mWidget::calc_hint == NULL であれば、
	//mContainer::calc_hint が使われる
	
	return (mWidget *)p;
}

/**@ コンテナ作成 */

mWidget *mContainerCreate(mWidget *parent,int type,int sep,uint32_t flayout,uint32_t margin_pack)
{
	mContainer *p;

	p = (mContainer *)mContainerNew(parent, 0);
	if(!p) return NULL;

	mContainerSetType(p, type);

	p->wg.flayout = flayout;
	p->ct.sep = sep;

	mWidgetSetMargin_pack4(MLK_WIDGET(p), margin_pack);

	return (mWidget *)p;
}

/**@ 垂直コンテナ作成 */

mWidget *mContainerCreateVert(mWidget *parent,int sep,uint32_t flayout,uint32_t margin_pack)
{
	return mContainerCreate(parent, MCONTAINER_TYPE_VERT, sep, flayout, margin_pack);
}

/**@ 水平コンテナ作成 */

mWidget *mContainerCreateHorz(mWidget *parent,int sep,uint32_t flayout,uint32_t margin_pack)
{
	return mContainerCreate(parent, MCONTAINER_TYPE_HORZ, sep, flayout, margin_pack);
}

/**@ グリッドコンテナ作成 */

mWidget *mContainerCreateGrid(mWidget *parent,int cols,int seph,int sepv,uint32_t flayout,uint32_t margin_pack)
{
	mContainer *p;

	p = (mContainer *)mContainerNew(parent, 0);
	if(!p) return NULL;

	mContainerSetType_grid(p, cols, seph, sepv);

	p->wg.flayout = flayout;
	p->ct.sep = seph;
	p->ct.sep_grid_v = sepv;

	mWidgetSetMargin_pack4(MLK_WIDGET(p), margin_pack);

	return (mWidget *)p;
}

/**@ コンテナのタイプをセット
 *
 * @d:レイアウトの関数などがセットされる。\
 * グリッドタイプの場合、横の数は 2 で初期化される。 */

void mContainerSetType(mContainer *p,int type)
{
	p->ct.type = type;
	p->ct.grid_cols = 2;
	
	switch(type)
	{
		//水平
		case MCONTAINER_TYPE_HORZ:
			p->ct.calc_hint = __mLayoutCalcHorz;
			p->wg.layout = __mLayoutHorz;
			break;
		//グリッド
		case MCONTAINER_TYPE_GRID:
			p->ct.calc_hint = __mLayoutCalcGrid;
			p->wg.layout = __mLayoutGrid;
			break;
		//垂直
		default:
			p->ct.calc_hint = __mLayoutCalcVert;
			p->wg.layout = __mLayoutVert;
			break;
	}
}

/**@ 垂直タイプにセット */

void mContainerSetType_vert(mContainer *p,int sep)
{
	mContainerSetType(p, MCONTAINER_TYPE_VERT);
	p->ct.sep = sep;
}

/**@ 水平タイプにセット */

void mContainerSetType_horz(mContainer *p,int sep)
{
	mContainerSetType(p, MCONTAINER_TYPE_HORZ);
	p->ct.sep = sep;
}

/**@ グリッドタイプにセット＆各情報セット
 *
 * @p:cols 横の数
 * @p:seph 横の区切り余白
 * @p:sepv 縦の区切り余白 */

void mContainerSetType_grid(mContainer *p,int cols,int seph,int sepv)
{
	mContainerSetType(p, MCONTAINER_TYPE_GRID);

	p->ct.grid_cols = (cols < 2)? 2: cols;
	p->ct.sep = seph;
	p->ct.sep_grid_v = sepv;
}

/**@ 余白をセット (上下左右同じ値に) */

void mContainerSetPadding_same(mContainer *p,int val)
{
	__mWidgetRectSetSame(&p->ct.padding, val);
}

/**@ 余白をセット (4つの値をパック)
 *
 * @p:val 上位バイトから順に、左-上-右-下 */

void mContainerSetPadding_pack4(mContainer *p,uint32_t val)
{
	__mWidgetRectSetPack4(&p->ct.padding, val);
}

/**@ 区切り余白と、余白(パック値)をセット */

void mContainerSetSepPadding(mContainer *p,int sep,uint32_t pack)
{
	p->ct.sep = sep;

	__mWidgetRectSetPack4(&p->ct.padding, pack);
}

/**@ OK/キャンセルボタンが格納されたコンテナを作成
 *
 * @d:水平コンテナ内に作成され、レイアウトは MLF_RIGHT。\
 * OK ボタンがデフォルトボタンとなる。
 *
 * @p:padding_pack コンテナの余白幅を4つパックで指定
 * @r:作成された水平コンテナ */

mWidget *mContainerCreateButtons_okcancel(mWidget *parent,uint32_t padding_pack)
{
	mWidget *ct;
	
	ct = mContainerCreateHorz(parent, 4, MLF_RIGHT, 0);

	mContainerSetPadding_pack4(MLK_CONTAINER(ct), padding_pack);

	mContainerAddButtons_okcancel(ct);

	return ct;
}

/**@ コンテナに直接 OK・キャンセルボタンを追加
 *
 * @d:OK/キャンセルボタンのコンテナに他のボタンなどを付けたい時に使う。 */

void mContainerAddButtons_okcancel(mWidget *parent)
{
	mWidget *p;

	//OK ボタン
	
	p = (mWidget *)mButtonCreate(parent, MLK_WID_OK, 0, 0, 0, MLK_TR_SYS(MLK_TRSYS_OK));
	
	p->fstate |= MWIDGET_STATE_ENTER_SEND;
	
	//キャンセルボタン
	
	mButtonCreate(parent, MLK_WID_CANCEL, 0, 0, 0, MLK_TR_SYS(MLK_TRSYS_CANCEL));
}


