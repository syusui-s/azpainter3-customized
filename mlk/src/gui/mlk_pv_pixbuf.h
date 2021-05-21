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

#ifndef MLK_PV_PIXBUF_H
#define MLK_PV_PIXBUF_H

#define MPIXBUFBASE(p)  ((mPixbufBase *)(p))

/** mPixbuf 内部データ */

typedef struct
{
	mFuncPixbufSetBuf setbuf;	//バッファに色をセットする関数
	mRect rcclip,		//現在のクリッピング範囲
		rcclip_root;	//システムで指定するクリッピング範囲 (rcclip は常にこの範囲内となる)
	int offsetX,		//描画時のオフセット位置
		offsetY;
	uint8_t fclip,		//クリッピングフラグ
		fcreate;		//作成時のフラグ
}mPixbufPrivate;

/** mPixbuf 基本データ */

typedef struct
{
	mPixbuf b;
	mPixbufPrivate pv;
}mPixbufBase;


enum
{
	MPIXBUF_CLIPF_HAVE = 1<<0,		//クリッピング範囲あり (イメージ範囲外も含む)
	MPIXBUF_CLIPF_HAVE_ROOT = 1<<1,	//root 範囲あり (イメージ範囲内)
	MPIXBUF_CLIPF_OUT = 1<<2		//rcclip の範囲がイメージ外 (描画可能な範囲がない)
};


mlkbool __mPixbufSetClipRoot(mPixbuf *p,mRect *rc);

#endif
