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
 * mPager
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_pager.h"


/* ページのデータ解放 */

static void _destroy_page(mPager *p)
{
	if(p->pg.info.destroy)
		(p->pg.info.destroy)(p, p->pg.info.pagedat);

	mFree(p->pg.info.pagedat);
}


/**@ mPager データ解放 */

void mPagerDestroy(mWidget *wg)
{
	_destroy_page(MLK_PAGER(wg));
}

/**@ 作成 */

mPager *mPagerNew(mWidget *parent,int size)
{
	mPager *p;

	if(size < sizeof(mPager))
		size = sizeof(mPager);

	p = (mPager *)mContainerNew(parent, size);
	if(!p) return NULL;

	p->wg.destroy = mPagerDestroy;
	p->wg.event = mPagerHandle_event;
	p->wg.notify_to_rep = MWIDGET_NOTIFYTOREP_SELF;

	return p;
}

/**@ 作成 */

mPager *mPagerCreate(mWidget *parent,uint32_t flayout,uint32_t margin_pack)
{
	mPager *p;

	p = mPagerNew(parent, 0);
	if(!p) return NULL;

	p->wg.flayout = flayout;

	mWidgetSetMargin_pack4(MLK_WIDGET(p), margin_pack);

	return p;
}

/**@ event ハンドラ関数 */

int mPagerHandle_event(mWidget *wg,mEvent *ev)
{
	mPager *p = MLK_PAGER(wg);

	if(p->pg.info.event)
		return (p->pg.info.event)(p, ev, p->pg.info.pagedat);
	else
		return FALSE;
}

/**@ ページの値データのポイントを取得 */

void *mPagerGetDataPtr(mPager *p)
{
	return p->pg.data;
}

/**@ ページの値データのポインタをセット
 *
 * @d:ウィジェット作成時はこのバッファから値を取得し、
 *  破棄時はこのバッファに値をセットする。 */

void mPagerSetDataPtr(mPager *p,void *data)
{
	p->pg.data = data;
}

/**@ 現在のページのウィジェットから、データを取得
 *
 * @r:データが取得できたか */

mlkbool mPagerGetPageData(mPager *p)
{
	if(!p->pg.info.getdata)
		return FALSE;
	else
		return (p->pg.info.getdata)(p, p->pg.info.pagedat, p->pg.data);
}

/**@ 現在のページのウィジェットにデータをセット
 *
 * @r:データがセットできたか */

mlkbool mPagerSetPageData(mPager *p)
{
	if(!p->pg.info.setdata)
		return FALSE;
	else
		return (p->pg.info.setdata)(p, p->pg.info.pagedat, p->pg.data);
}

/**@ 現在のページを破棄
 *
 * @d:現在のページのデータを取得し、子ウィジェットをすべて破棄。 */

void mPagerDestroyPage(mPager *p)
{
	//現在のデータ取得

	mPagerGetPageData(p);

	//情報クリア

	_destroy_page(p);

	mMemset0(&p->pg.info, sizeof(mPagerInfo));

	//ウィジェット破棄

	while(p->wg.first)
		mWidgetDestroy(p->wg.first);
}

/**@ 中身のページをセット
 *
 * @d:現在のページを破棄し、新しいページのウィジェットを作成後、ウィジェットにデータをセットする。\
 *  ※再レイアウトは行われないので、セット後に、影響のある最上位の親に対して実行すること。\
 *  子ウィジェットを作成した時点で、上位のウィジェットの推奨サイズは再計算される。\
 * \
 * ページのデータが変化しない場合、setdata を NULL にして、ページの作成と同時に、ウィジェットに値をセットしても構わない。
 *
 * @p:create ページの作成関数。\
 *  mPager を親として子ウィジェットを作成し、mPagerInfo に各値をセットする。
 *
 * @r:作成に成功したか */

mlkbool mPagerSetPage(mPager *p,mFuncPagerCreate create)
{
	//現在のページ破棄
	
	mPagerDestroyPage(p);

	//作成

	if(!create || !(create)(p, &p->pg.info))
		return FALSE;

	//データセット

	mPagerSetPageData(p);

	return TRUE;
}

