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

/************************************
 * 素材画像の選択ウィジェット
 ************************************/

typedef struct _SelMaterial SelMaterial;

enum
{
	SELMATERIAL_TYPE_TEXTURE,		//オプション/レイヤのテクスチャ
	SELMATERIAL_TYPE_BRUSH_TEXTURE,	//ブラシのテクスチャ
	SELMATERIAL_TYPE_BRUSH_SHAPE	//ブラシ形状
};

SelMaterial *SelMaterial_new(mWidget *parent,int id,int type);
void SelMaterial_setPath(SelMaterial *p,const char *path);
void SelMaterial_getPath(SelMaterial *p,mStr *str);

mWidget *SelMaterialTex_new(mWidget *parent,int id);
void SelMaterialTex_setFlag(mWidget *p,int flag);

//strdst にパスが入っている場合、その位置を表示
mlkbool SelMaterialDlg_run(mWindow *parent,mStr *strdst,mlkbool is_texture);
