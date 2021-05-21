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
 * コンテナレイアウト関数
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"

#include "mlk_pv_widget.h"

#if 0
#define _PUT_DEBUG(...);  mDebug(__VA_ARGS__);
#else
#define _PUT_DEBUG(...);
#endif


//==================================
// 共通
//==================================


/** コンテナ内の子ウィジェットを配置可能な領域のサイズを取得
 *
 * コンテナの内部余白は除く */

static void _get_container_inside_size(mContainer *p,int *pw,int *ph)
{
	int w,h;

	w = p->wg.w - p->ct.padding.left - p->ct.padding.right;
	h = p->wg.h - p->ct.padding.top - p->ct.padding.bottom;
	
	if(w < 1) w = 1;
	if(h < 1) h = 1;
	
	*pw = w, *ph = h;
}

/** 推奨サイズセット
 *
 * 各最大サイズにコンテナの余白幅を追加して hintW/hintH/initW/initH にセット */

static void _set_hint_size(mWidget *wg,mSize *hint,mSize *init)
{
	mContainer *p = (mContainer *)wg;
	int pw,ph;

	pw = p->ct.padding.left + p->ct.padding.right;
	ph = p->ct.padding.top + p->ct.padding.bottom;

	wg->hintW = hint->w + pw;
	wg->hintH = hint->h + ph;
	wg->initW = init->w + pw;
	wg->initH = init->h + ph;
}


//==================================
// 水平コンテナ
//==================================


/** 推奨サイズ計算 */

void __mLayoutCalcHorz(mWidget *root)
{
	mWidget *p;
	mSize hint,init,maxhint,maxinit;
	int sep;

	_PUT_DEBUG("#container:HORZ:calc\n");
	
	sep = MLK_CONTAINER(root)->ct.sep;

	maxhint.w = maxhint.h = 0;
	maxinit.w = maxinit.h = 0;

	//自身の表示状態は関係なく、子全体のサイズを取得。
	//calc_hint は下位から順に実行されるので、子ウィジェットはすべてセット済み

	for(p = root->first; p; p = p->next)
	{
		if(p->fstate & MWIDGET_STATE_VISIBLE)
		{
			__mWidgetGetLayoutCalcSize(p, &hint, &init);

			maxhint.w += hint.w;
			maxinit.w += init.w;

			if(p->next)
			{
				maxhint.w += sep;
				maxinit.w += sep;
			}

			if(maxhint.h < hint.h) maxhint.h = hint.h;
			if(maxinit.h < init.h) maxinit.h = init.h;
		}
	}

	_set_hint_size(root, &maxhint, &maxinit);
}

/** 水平レイアウト実行 */

void __mLayoutHorz(mWidget *wg)
{
	mContainer *container = MLK_CONTAINER(wg);
	mWidget *p,*expand_last_wg;
	int expand_cnt,expand_w_each,expand_w_last;
	int cw,ch,x,y,w,wg_w,wg_h,lw,lh,mw,mh,tmp,left,top;
	uint32_t lflags;

	_PUT_DEBUG("#container:HORZ:layout\n");

	//配置可能な領域のサイズ
	
	_get_container_inside_size(container, &cw, &ch);
	
	//拡張ウィジェットの数、拡張用の残りサイズ、最後の拡張ウィジェット取得
	
	w = cw;
	expand_cnt = 0;
	expand_last_wg = NULL;
	
	for(p = wg->first; p; p = p->next)
	{
		if(!(p->fstate & MWIDGET_STATE_VISIBLE)) continue;
		
		if(p->flayout & (MLF_EXPAND_X | MLF_EXPAND_W))
		{
			expand_cnt++;
			expand_last_wg = p;
		}
		else
			w -= __mWidgetGetLayoutW_space(p);
		
		if(p->next) w -= container->ct.sep;
	}
	
	//拡張ウィジェットの一つの幅と最後のアイテムの幅

	if(!expand_cnt) expand_cnt = 1;
	if(w <= 0) w = 0;

	expand_w_each = w / expand_cnt;
	expand_w_last = w - expand_w_each * (expand_cnt - 1);
	
	//---------- レイアウト
	
	left = container->ct.padding.left;
	top  = container->ct.padding.top;
	
	for(p = wg->first; p; p = p->next)
	{
		if(p->calc_layout_size)
		{
			lw = cw;
			lh = ch;
			(p->calc_layout_size)(p, &lw, &lh);
		}
		else
		{
			lw = __mWidgetGetLayoutW(p);
			lh = __mWidgetGetLayoutH(p);
		}
		
		lflags = p->flayout;
		
		mw = p->margin.left + p->margin.right;
		mh = p->margin.top + p->margin.bottom;
		
		x = y = 0;
		wg_w = lw;
		wg_h = lh;
				
		//拡張、高さ
		
		if(lflags & MLF_EXPAND_H)
		{
			wg_h = ch - mh;
			if(wg_h < lh) wg_h = lh;
		}
		
		//Y 位置

		if(lflags & MLF_MIDDLE)
			y = (ch - wg_h - mh) >> 1;
		else if(lflags & MLF_BOTTOM)
			y = ch - wg_h - p->margin.bottom;
		
		if(y < 0) y = 0;
		
		y += top + p->margin.top;
		
		//ウィジェット幅
				
		if(!(lflags & (MLF_EXPAND_X | MLF_EXPAND_W)))
			w = lw + mw;
		else
		{
			w = (p == expand_last_wg)? expand_w_last: expand_w_each;

			tmp = w - mw;
			if(tmp < lw) tmp = lw;
			
			if(lflags & MLF_EXPAND_W) wg_w = tmp;
			
			w = tmp + mw;
		}
		
		//X 位置
		
		if(lflags & MLF_CENTER)
			x = (w - wg_w - mw) >> 1;
		else if(lflags & MLF_RIGHT)
			x = w - wg_w - mw;
		
		if(x < 0) x = 0;
		
		x += left + p->margin.left;
		
		//ウィジェット位置・サイズ
		// :splitter のサイズ変更対象で、初期状態が非表示の場合、
		// :レイアウトを行っておかないと、移動後に正しい位置にならないため、
		// :位置とサイズは非表示状態でも常にセットする。
		
		mWidgetMoveResize(p, x, y, wg_w, wg_h);
		
		//次の位置
		
		if(p->fstate & MWIDGET_STATE_VISIBLE)
		{
			left += w;
			if(p->next) left += container->ct.sep;
		}
	}
}


//==================================
// 垂直コンテナ
//==================================


/** 推奨サイズ計算 */

void __mLayoutCalcVert(mWidget *root)
{
	mWidget *p;
	mSize hint,init,maxhint,maxinit;
	int sep;

	_PUT_DEBUG("#container:VERT:calc\n");

	sep = MLK_CONTAINER(root)->ct.sep;
	
	maxhint.w = maxhint.h = 0;
	maxinit.w = maxinit.h = 0;

	for(p = root->first; p; p = p->next)
	{
		if(p->fstate & MWIDGET_STATE_VISIBLE)
		{
			__mWidgetGetLayoutCalcSize(p, &hint, &init);

			maxhint.h += hint.h;
			maxinit.h += init.h;

			if(p->next)
			{
				maxhint.h += sep;
				maxinit.h += sep;
			}

			if(maxhint.w < hint.w) maxhint.w = hint.w;
			if(maxinit.w < init.w) maxinit.w = init.w;
		}
	}
	
	_set_hint_size(root, &maxhint, &maxinit);
}

/** 垂直レイアウト実行 */

void __mLayoutVert(mWidget *wg)
{
	mContainer *container = MLK_CONTAINER(wg);
	mWidget *p,*expand_last_wg;
	int expand_cnt,expand_w_each,expand_w_last;
	int cw,ch,x,y,h,wg_w,wg_h,lw,lh,mw,mh,tmp,left,top;
	uint32_t lflags;
	
	_PUT_DEBUG("#container:VERT:layout\n");

	_get_container_inside_size(container, &cw, &ch);

	//拡張アイテム情報取得
	
	h = ch;
	expand_cnt = 0;
	expand_w_each = expand_w_last = 0;
	expand_last_wg = NULL;
	
	for(p = container->wg.first; p; p = p->next)
	{
		if(!(p->fstate & MWIDGET_STATE_VISIBLE)) continue;
		
		if(p->flayout & (MLF_EXPAND_Y | MLF_EXPAND_H))
		{
			expand_cnt++;
			expand_last_wg = p;
		}
		else
			h -= __mWidgetGetLayoutH_space(p);
		
		if(p->next) h -= container->ct.sep;
	}
	
	//拡張アイテムの各幅と最後のアイテムの幅
	
	if(!expand_cnt) expand_cnt = 1;
	if(h <= 0) h = 0;

	expand_w_each = h / expand_cnt;
	expand_w_last = h - expand_w_each * (expand_cnt - 1);

	//---------- レイアウト
	
	left = container->ct.padding.left;
	top  = container->ct.padding.top;
	
	for(p = container->wg.first; p; p = p->next)
	{
		if(p->calc_layout_size)
		{
			lw = cw;
			lh = ch;
			(p->calc_layout_size)(p, &lw, &lh);
		}
		else
		{
			lw = __mWidgetGetLayoutW(p);
			lh = __mWidgetGetLayoutH(p);
		}

		lflags = p->flayout;
		
		mw = p->margin.left + p->margin.right;
		mh = p->margin.top + p->margin.bottom;
		
		x = y = 0;
		wg_w = lw;
		wg_h = lh;
				
		//幅・拡張
		
		if(lflags & MLF_EXPAND_W)
		{
			wg_w = cw - mw;
			if(wg_w < lw) wg_w = lw;
		}
		
		//X 位置
		
		if(lflags & MLF_CENTER)
			x = (cw - wg_w - mw) >> 1;
		else if(lflags & MLF_RIGHT)
			x = cw - wg_w - p->margin.right;
		
		if(x < 0) x = 0;
		
		x += left + p->margin.left;
		
		//ウィジェット高さ
				
		if(!(lflags & (MLF_EXPAND_Y | MLF_EXPAND_H)))
			h = lh + mh;
		else
		{
			h = (p == expand_last_wg)? expand_w_last: expand_w_each;

			tmp = h - mh;
			if(tmp < lh) tmp = lh;
			
			if(lflags & MLF_EXPAND_H) wg_h = tmp;
			
			h = tmp + mh;
		}
		
		//Y 位置
		
		if(lflags & MLF_MIDDLE)
			y = (h - wg_h - mh) >> 1;
		else if(lflags & MLF_BOTTOM)
			y = h - wg_h - mh;
		
		if(y < 0) y = 0;
		
		y += top + p->margin.top;
		
		//ウィジェット位置・サイズ
		
		mWidgetMoveResize(p, x, y, wg_w, wg_h);

		//次の位置
		
		if(p->fstate & MWIDGET_STATE_VISIBLE)
		{
			top += h;
			if(p->next) top += container->ct.sep;
		}
	}
}


//==================================
// GRID : グリッド
//==================================


typedef struct
{
	int expand_last_no,		//拡張列の最後の番号
		expand_w_each,		//拡張の各幅
		expand_w_last;		//拡張の最後の幅
	uint32_t cell_size[1];	//各列の幅か高さ (上位2bit はフラグ)
}__grid_info;

#define GRID_CELL_GETSIZE(n)  ((n) & ((1<<30) - 1))
#define GRID_CELL_F_EXPAND    ((uint32_t)1<<31)		//拡張
#define GRID_CELL_F_ADDSEP    (1<<30)				//余白追加


/** 同じ列の次の行へ */

static mWidget *_get_next_row(mContainer *ct,mWidget *p)
{
	int cols = ct->ct.grid_cols;

	for(; p && cols; cols--, p = p->next);
	
	return p;
}

/** 推奨サイズ計算 */

void __mLayoutCalcGrid(mWidget *root)
{
	mContainer *container;
	mWidget *p,*p2;
	mSize hint,init,maxhint,maxinit;
	int cols,sep,maxh,maxi,cnt;

	_PUT_DEBUG("#container:GRID:calc\n");

	container = MLK_CONTAINER(root);
	cols = container->ct.grid_cols;

	maxhint.w = maxhint.h = 0;
	maxinit.w = maxinit.h = 0;
		
	//-------- 幅 (縦列の各アイテムの最大幅)
	
	sep = container->ct.sep;

	for(p = root->first, cnt = cols; p && cnt; cnt--, p = p->next)
	{
		//縦列の最大幅

		maxh = maxi = 0;
		
		for(p2 = p; p2; p2 = _get_next_row(container, p2))
		{
			if(p2->fstate & MWIDGET_STATE_VISIBLE)
			{
				__mWidgetGetLayoutCalcSize(p2, &hint, &init);

				if(maxh < hint.w) maxh = hint.w;
				if(maxi < init.w) maxi = init.w;
			}
		}

		//最後の列でなければ余白追加

		if(cnt > 1)
		{
			if(maxh) maxh += sep;
			if(maxi) maxi += sep;
		}

		maxhint.w += maxh;
		maxinit.w += maxi;
	}
	
	//------ 高さ
	
	sep = container->ct.sep_grid_v;

	for(p = root->first; p; )
	{
		//横1列の最大高さ
		
		maxh = maxi = 0;
	
		for(cnt = cols; p && cnt; cnt--, p = p->next)
		{
			if(p->fstate & MWIDGET_STATE_VISIBLE)
			{
				__mWidgetGetLayoutCalcSize(p, &hint, &init);

				if(maxh < hint.h) maxh = hint.h;
				if(maxi < init.h) maxi = init.h;
			}
		}

		//最後の行でなければ余白追加

		if(p)
		{
			if(maxh) maxh += sep;
			if(maxi) maxi += sep;
		}

		maxhint.h += maxh;
		maxinit.h += maxi;
	}

	//

	_set_hint_size(root, &maxhint, &maxinit);
}


/** 縦一列の列ごとの情報を作成
 *
 * width: 領域の幅
 * ret_rows: 縦列の数が入る */

static __grid_info *_get_grid_col_info(mContainer *ctn,int width,int *ret_rows)
{
	__grid_info *info;
	mWidget *px,*py;
	int i,flags,max,tmp,cols,rows = 0,expand_cnt = 0,expand_last_no = 0;
	uint32_t cell;
	
	cols = ctn->ct.grid_cols;

	//終端は各列の幅

	info = (__grid_info *)mMalloc(sizeof(__grid_info) + (cols - 1) * sizeof(uint32_t));
	if(!info) return NULL;
	
	//縦一列ごとに処理
	
	for(px = ctn->wg.first, i = 0; px && i < cols; px = px->next, i++)
	{
		//縦一列の状態と最大幅
		//[flags] bit0:一つでも表示、bit1:一つでも拡張あり
	
		flags = max = 0;
	
		for(py = px; py; py = _get_next_row(ctn, py))
		{
			if(py->fstate & MWIDGET_STATE_VISIBLE)
			{
				flags |= 1;
				
				if(py->flayout & (MLF_EXPAND_X | MLF_EXPAND_W))
					flags |= 2;
				
				tmp = __mWidgetGetLayoutW_space(py);
				if(tmp > max) max = tmp;
			}

			//最初の列の処理時のみ、縦列数を計算
			if(i == 0) rows++;
		}
		
		//列の幅とフラグをセット
		
		cell = max;
		if(flags & 2) cell |= GRID_CELL_F_EXPAND;
		if((flags & 1) && i < cols - 1) cell |= GRID_CELL_F_ADDSEP;
		
		info->cell_size[i] = cell;
		
		//列を表示する場合、拡張用の情報
		
		if(flags)
		{
			if(flags & 2)
			{
				expand_cnt++;
				expand_last_no = i;
			}
			else
				width -= max;
			
			if(cell & GRID_CELL_F_ADDSEP) width -= ctn->ct.sep;
		}
	}
	
	//拡張列の情報
	
	if(!expand_cnt) expand_cnt = 1;
	if(width < 0) width = 0;

	info->expand_last_no = expand_last_no;
	info->expand_w_each  = width / expand_cnt;
	info->expand_w_last  = width - info->expand_w_each * (expand_cnt - 1);

	//
	
	*ret_rows = rows;
	
	return info;
}

/** 横一列の情報作成
 *
 * rows: 縦列の数
 * height: 領域の高さ */

static __grid_info *_get_grid_row_info(mContainer *ctn,int rows,int height)
{
	__grid_info *info;
	mWidget *p;
	int i,j,flags,max,tmp,cols,expand_cnt = 0,expand_last_no = 0;
	uint32_t cell;
	
	cols = ctn->ct.grid_cols;

	info = (__grid_info *)mMalloc(sizeof(__grid_info) + (rows - 1) * sizeof(uint32_t));
	if(!info) return NULL;
	
	//横一列ごとに処理
	
	for(p = ctn->wg.first, i = 0; p; i++)
	{
		//横一列の状態と最大高さ
		
		flags = max = 0;
	
		for(j = 0; p && j < cols; j++, p = p->next)
		{
			if(p->fstate & MWIDGET_STATE_VISIBLE)
			{
				flags |= 1;
				
				if(p->flayout & (MLF_EXPAND_Y | MLF_EXPAND_H))
					flags |= 2;
				
				tmp = __mWidgetGetLayoutH_space(p);
				if(tmp > max) max = tmp;
			}
		}
		
		//列の高さとフラグセット
		
		cell = max;
		if(flags & 2) cell |= GRID_CELL_F_EXPAND;
		if((flags & 1) && p) cell |= GRID_CELL_F_ADDSEP;
		
		info->cell_size[i] = cell;
		
		//列を表示する場合、拡張用の情報
		
		if(flags)
		{
			if(flags & 2)
			{
				expand_cnt++;
				expand_last_no = i;
			}
			else
				height -= max;
	
			if(cell & GRID_CELL_F_ADDSEP) height -= ctn->ct.sep_grid_v;
		}
	}
	
	//拡張列の情報
	
	if(!expand_cnt) expand_cnt = 1;
	if(height < 0) height = 0;

	info->expand_last_no = expand_last_no;
	info->expand_w_each  = height / expand_cnt;
	info->expand_w_last  = height - info->expand_w_each * (expand_cnt - 1);
	
	return info;
}

/** グリッドレイアウト実行 */

void __mLayoutGrid(mContainer *wg)
{
	mContainer *container = MLK_CONTAINER(wg);
	__grid_info *info_col,*info_row;
	mWidget *p;
	int cw,ch,rows,top,left,left_start,row_no,col_no,maxh;
	int lw,lh,lflags,mw,mh,x,y,wg_w,wg_h,w,h,tmp;
	uint32_t cell;

	_PUT_DEBUG("#container:GRID:layout\n");

	_get_container_inside_size(container, &cw, &ch);
	
	//縦/横各列の情報取得
	
	info_col = _get_grid_col_info(container, cw, &rows);
	if(!info_col) return;
	
	info_row = _get_grid_row_info(container, rows, ch);
	if(!info_row)
	{
		mFree(info_col);
		return;
	}
	
	//
	
	left_start = container->ct.padding.left;
	top = container->ct.padding.top;
	
	for(p = container->wg.first, row_no = 0; p; row_no++)
	{
		left = left_start;
		maxh = 0;
		
		for(col_no = 0; p && col_no < container->ct.grid_cols; col_no++, p = p->next)
		{
			if(p->calc_layout_size)
			{
				lw = GRID_CELL_GETSIZE(info_col->cell_size[col_no]);
				lh = GRID_CELL_GETSIZE(info_row->cell_size[row_no]);

				(p->calc_layout_size)(p, &lw, &lh);
			}
			else
			{
				lw = __mWidgetGetLayoutW(p);
				lh = __mWidgetGetLayoutH(p);
			}

			lflags = p->flayout;
			
			mw = p->margin.left + p->margin.right;
			mh = p->margin.top + p->margin.bottom;
			
			x = y = 0;
			wg_w = lw;
			wg_h = lh;
			
			//幅
			// :w = セル幅, wg_w = ウィジェット幅
			
			cell = info_col->cell_size[col_no];

			w = GRID_CELL_GETSIZE(cell);
			
			if(lflags & MLF_GRID_COL_W)
				//列の幅に合わせる
				wg_w = w - mw;
			else if(cell & GRID_CELL_F_EXPAND)
			{
				//<縦列の中に一つでも拡張がある場合>
				//セル幅は、拡張の幅の方が大きければそちらを使う
				
				tmp = (col_no == info_col->expand_last_no)? info_col->expand_w_last: info_col->expand_w_each;
				if(w < tmp) w = tmp;
				
				if(lflags & MLF_EXPAND_W) wg_w = w - mw;
			}
			
			//高さ
			
			cell = info_row->cell_size[row_no];
			
			h = GRID_CELL_GETSIZE(cell);

			if(lflags & MLF_GRID_ROW_H)
				//横列の高さに合わせる
				wg_h = h - mh;
			else if(cell & GRID_CELL_F_EXPAND)
			{
				//横列の中に一つでも拡張がある場合
				
				tmp = (row_no == info_row->expand_last_no)? info_row->expand_w_last: info_row->expand_w_each;
				if(h < tmp) h = tmp;
				
				if(lflags & MLF_EXPAND_H) wg_h = h - mh;
			}
			
			//X 位置
			
			if(lflags & MLF_CENTER)
				x = (w - wg_w - mw) >> 1;
			else if(lflags & MLF_RIGHT)
				x = w - wg_w - mw;
			
			if(x < 0) x = 0;
			
			x += left + p->margin.left;
			
			//Y 位置
			
			if(lflags & MLF_MIDDLE)
				y = (h - wg_h - mh) >> 1;
			else if(lflags & MLF_BOTTOM)
				y = h - wg_h - mh;
			
			if(y < 0) y = 0;
			
			y += top + p->margin.top;
			
			//位置・サイズ変更
			
			mWidgetMoveResize(p, x, y, wg_w, wg_h);
			
			//次へ
			
			if(p->fstate & MWIDGET_STATE_VISIBLE)
			{
				left += w;

				if(info_col->cell_size[col_no] & GRID_CELL_F_ADDSEP)
					left += container->ct.sep;
			
				if(h > maxh) maxh = h;
			}
		}
		
		top += maxh;

		if(info_row->cell_size[row_no] & GRID_CELL_F_ADDSEP)
			top += container->ct.sep_grid_v;
	}
	
	//
	
	mFree(info_col);
	mFree(info_row);
}
