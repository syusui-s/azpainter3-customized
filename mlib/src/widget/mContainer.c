/*$
 Copyright (C) 2013-2020 Azel.

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
 * mContainer [コンテナ]
 *****************************************/


#include "mDef.h"

#include "mContainerDef.h"
#include "mContainer.h"

#include "mWidget.h"
#include "mButton.h"
#include "mTrans.h"


//---------------------

void __mLayoutCalcHorz(mWidget *);
void __mLayoutCalcVert(mWidget *);
void __mLayoutCalcGrid(mWidget *);

void __mLayoutHorz(mWidget *);
void __mLayoutVert(mWidget *);
void __mLayoutGrid(mWidget *);

//---------------------

/**
@var mContainerData::gridCols
グリッドタイプの場合の横カラム数

@var mContainerData::sepW
水平・垂直タイプの場合の区切り余白 (px)

@var mContainerData::gridSepCol
グリッドタイプ時の区切り余白 (横方向)

@var mContainerData::padding
コンテナとコンテナ内容部分の間の余白

@var mContainerData::calcHint
コンテナ用の推奨サイズ計算。 \n
mWidget::calcHint が NULL の場合はこれが使われる。
*/


/**
@defgroup container mContainer
@brief コンテナ

- mWidget::draw は、デフォルトで mWidgetHandleFunc_draw_drawBkgnd()。
- 派生ウィジェットなどで、コンテナの推奨サイズを計算するには、mContainerData::calcHint の関数を使う。

<h3>継承</h3>
mWidget \> mContainer

@ingroup group_widget
@{

@file mContainerDef.h
@file mContainer.h
@struct mContainerData
@struct _mContainer
@enum MCONTAINER_TYPE
*/


/** コンテナ作成 */

mWidget *mContainerCreate(mWidget *parent,int type,int gridcols,int sepw,uint32_t fLayout)
{
	mContainer *p;

	p = mContainerNew(0, parent);
	if(!p) return NULL;

	mContainerSetType(p, type, gridcols);

	p->wg.fLayout = fLayout;
	p->ct.sepW = sepw;

	return M_WIDGET(p);
}

/** コンテナ作成 (グリッドタイプ) */

mWidget *mContainerCreateGrid(mWidget *parent,int gridcols,int sepcol,int seprow,uint32_t fLayout)
{
	mWidget *p;

	p = mContainerCreate(parent, MCONTAINER_TYPE_GRID, gridcols, 0, fLayout);
	if(p)
	{
		M_CONTAINER(p)->ct.gridSepCol = sepcol;
		M_CONTAINER(p)->ct.gridSepRow = seprow;
	}

	return p;
}

/** コンテナ作成 */

mContainer *mContainerNew(int size,mWidget *parent)
{
	mContainer *p;
	
	if(size < sizeof(mContainer)) size = sizeof(mContainer);
	
	p = (mContainer *)mWidgetNew(size, parent);
	if(!p) return NULL;
	
	mContainerSetType(p, MCONTAINER_TYPE_VERT, 0);
	
	p->wg.fType |= MWIDGET_TYPE_CONTAINER;
	p->wg.draw = mWidgetHandleFunc_draw_drawBkgnd;
	p->wg.calcHint = NULL;
	
	/* コンテナの場合、mWidget::calcHint == NULL なら
	 * mContainer::calcHint が使われる */
	
	return p;
}

/** コンテナのタイプセット */

void mContainerSetType(mContainer *p,int type,int grid_cols)
{
	if(grid_cols < 2) grid_cols = 2;

	p->ct.type = type;
	p->ct.gridCols = grid_cols;
	
	switch(type)
	{
		case MCONTAINER_TYPE_HORZ:
			p->ct.calcHint = __mLayoutCalcHorz;
			p->wg.layout = __mLayoutHorz;
			break;
		case MCONTAINER_TYPE_GRID:
			p->ct.calcHint = __mLayoutCalcGrid;
			p->wg.layout = __mLayoutGrid;
			break;
		default:
			p->ct.calcHint = __mLayoutCalcVert;
			p->wg.layout = __mLayoutVert;
			break;
	}
}

/** グリッドタイプにセットし、各情報セット */

void mContainerSetTypeGrid(mContainer *p,int gridcols,int sepcol,int seprow)
{
	mContainerSetType(p, MCONTAINER_TYPE_GRID, gridcols);

	p->ct.gridSepCol = sepcol;
	p->ct.gridSepRow = seprow;
}

/** 余白をセット (上下左右同じ値) */

void mContainerSetPadding_one(mContainer *p,int val)
{
	p->ct.padding.left = val;
	p->ct.padding.top = val;
	p->ct.padding.right = val;
	p->ct.padding.bottom = val;
}

/** 余白をセット
 *
 * @param val 上位バイトから順に left,top,right,bottom */

void mContainerSetPadding_b4(mContainer *p,uint32_t val)
{
	p->ct.padding.left   = (uint8_t)(val >> 24);
	p->ct.padding.top    = (uint8_t)(val >> 16);
	p->ct.padding.right  = (uint8_t)(val >> 8);
	p->ct.padding.bottom = (uint8_t)val;
}

/** OK/キャンセルボタンを作成
 *
 * @return 作成された水平コンテナ */

mWidget *mContainerCreateOkCancelButton(mWidget *parent)
{
	mWidget *ct;
	mButton *btt;
	
	//水平コンテナ
	
	ct = mContainerCreate(parent, MCONTAINER_TYPE_HORZ, 0, 4, MLF_RIGHT);
	
	//OK ボタン
	
	btt = mButtonCreate(ct, M_WID_OK, 0, 0, 0, M_TR_T2(M_TRGROUP_SYS, M_TRSYS_OK));
	
	btt->wg.fState |= MWIDGET_STATE_ENTER_DEFAULT;
	
	//キャンセルボタン
	
	mButtonCreate(ct, M_WID_CANCEL, 0, 0, 0, M_TR_T2(M_TRGROUP_SYS, M_TRSYS_CANCEL));

	return ct;
}

/** @} */
