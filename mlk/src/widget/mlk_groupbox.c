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
 * mGroupBox [グループボックス]
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_groupbox.h"
#include "mlk_pixbuf.h"
#include "mlk_guicol.h"

#include "mlk_pv_widget.h"


//-----------------------------

#define _LABEL_X   6		//テキストの左位置
#define _LABEL_PADDING_X 2  //枠とテキスト間の水平余白

//-----------------------------



/**@ データ解放 */

void mGroupBoxDestroy(mWidget *wg)
{
	mWidgetLabelText_free(&MLK_GROUPBOX(wg)->gb.txt);
}

/**@ 作成 */

mGroupBox *mGroupBoxNew(mWidget *parent,int size,uint32_t fstyle)
{
	mGroupBox *p;
	
	if(size < sizeof(mGroupBox))
		size = sizeof(mGroupBox);
	
	p = (mGroupBox *)mContainerNew(parent, size);
	if(!p) return NULL;
	
	p->wg.destroy = mGroupBoxDestroy;
	p->wg.calc_hint = mGroupBoxHandle_calcHint;
	p->wg.draw = mGroupBoxHandle_draw;

	__mWidgetRectSetPack4(&p->gb.padding, MLK_MAKE32_4(8,7,8,7));

	p->gb.fstyle = fstyle;
	
	return p;
}

/**@ 作成 */

mGroupBox *mGroupBoxCreate(mWidget *parent,uint32_t flayout,uint32_t margin_pack,
	uint32_t fstyle,const char *text)
{
	mGroupBox *p;

	p = mGroupBoxNew(parent, 0, fstyle);
	if(!p) return NULL;

	__mWidgetCreateInit(MLK_WIDGET(p), 0, flayout, margin_pack);

	mWidgetLabelText_set(MLK_WIDGET(p), &p->gb.txt, text,
		fstyle & MGROUPBOX_S_COPYTEXT);

	return p;
}

/**@ 内側の余白幅をセット
 *
 * @d:※一度レイアウトした後に変更する場合、再レイアウトが必要。 */

void mGroupBoxSetPadding_pack4(mGroupBox *p,uint32_t pack)
{
	__mWidgetRectSetPack4(&p->gb.padding, pack);

	mWidgetSetRecalcHint(MLK_WIDGET(p));
}

/**@ ラベルセット
 *
 * @d:テキストを複製するかは、スタイルフラグによる */

void mGroupBoxSetText(mGroupBox *p,const char *text)
{
	mWidgetLabelText_set(MLK_WIDGET(p), &p->gb.txt, text,
		p->gb.fstyle & MGROUPBOX_S_COPYTEXT);
}

/**@ ラベルをセット (複製)
 *
 * @d:スタイルフラグは変更しない。 */

void mGroupBoxSetText_copy(mGroupBox *p,const char *text)
{
	mWidgetLabelText_set(MLK_WIDGET(p), &p->gb.txt, text, TRUE);
}

/**@ calc_hint ハンドラ関数 */

void mGroupBoxHandle_calcHint(mWidget *wg)
{
	mGroupBox *p = MLK_GROUPBOX(wg);
	mSize size;

	//テキストサイズ (ラベル無しなら、高さを 0 にする)

	mWidgetLabelText_onCalcHint(wg, &p->gb.txt, &size);

	if(size.w == 0)
	{
		p->gb.txt.szfull.h = 0;
		size.h = 0;
	}

	//枠の幅分を追加
	
	if(size.w)
		size.w += 6 + _LABEL_PADDING_X * 2;

	//コンテナの padding

	p->ct.padding = p->gb.padding;
	p->ct.padding.top += size.h;

	//コンテナの推奨サイズ
	
	(p->ct.calc_hint)(wg);

	//ラベル幅の方が大きければ、ラベル幅に合わせる

	if(wg->hintW < size.w) wg->hintW = size.w;
	if(wg->initW < size.w) wg->initW = size.w;
}

/**@ draw ハンドラ関数 */

void mGroupBoxHandle_draw(mWidget *wg,mPixbuf *pixbuf)
{
	mGroupBox *p = MLK_GROUPBOX(wg);
	mBox box;
	int tx,ftop,tw;

	//背景
	
	mWidgetDrawBkgnd(wg, NULL);
	
	//枠 (ftop = 枠の top 位置)
	
	ftop = p->gb.txt.szfull.h >> 1;
	
	mPixbufBox(pixbuf, 0, ftop, wg->w, wg->h - ftop, MGUICOL_PIX(FRAME_BOX));
	
	//ラベル
	
	if(p->gb.txt.text)
	{
		//テキストの幅 (余白含む)
		
		tw = p->gb.txt.szfull.w + _LABEL_PADDING_X * 2;

		//テキスト左位置
		// 基本的に _LABEL_X 位置で左端固定。
		// _LABEL_X から描画すると幅を超える場合は、中央揃え。

		tx = (_LABEL_X + tw >= wg->w)? (wg->w - tw) >> 1: _LABEL_X;
		
		//テキスト部分に重なる枠を消す
		
		box.x = tx;
		box.y = ftop;
		box.w = tw;
		box.h = 1;
		
		mWidgetDrawBkgnd_force(wg, &box);
		
		//テキスト
		
		mWidgetLabelText_draw(&p->gb.txt, pixbuf, mWidgetGetFont(wg),
			tx + _LABEL_PADDING_X, 0, p->gb.txt.szfull.w, MGUICOL_RGB(TEXT), 0);
	}
}

