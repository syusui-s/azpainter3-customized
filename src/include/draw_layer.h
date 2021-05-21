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

/**********************************
 * AppDraw レイヤ処理
 **********************************/

typedef struct _LayerNewOptInfo LayerNewOptInfo;


void drawLayer_newLayer(AppDraw *p,LayerNewOptInfo *info,LayerItem *pi_above);
void drawLayer_newLayer_direct(AppDraw *p,int type);
mlkerr drawLayer_newLayer_file(AppDraw *p,const char *filename,mRect *rcupdate);
mlkbool drawLayer_newLayer_image(AppDraw *p,TileImage *img);

void drawLayer_copy(AppDraw *p);
void drawLayer_delete(AppDraw *p,mlkbool update);
void drawLayer_erase(AppDraw *p);
void drawLayer_setLayerType(AppDraw *p,LayerItem *item,int type,mlkbool lum_to_alpha);
void drawLayer_editImage_full(AppDraw *p,int type);

void drawLayer_combine(AppDraw *p,mlkbool drop);
void drawLayer_combineMulti(AppDraw *p,int target,mlkbool newlayer,int type);
void drawLayer_blendAll(AppDraw *p);

mlkbool drawLayer_setCurrent(AppDraw *p,LayerItem *item);
void drawLayer_setCurrent_visibleOnList(AppDraw *p,LayerItem *item);
void drawLayer_setTexture(AppDraw *p,LayerItem *item,const char *path);
void drawLayer_setLayerColor(AppDraw *p,LayerItem *item,uint32_t col);

void drawLayer_toggle_folderOpened(AppDraw *p,LayerItem *item);
void drawLayer_toggle_visible(AppDraw *p,LayerItem *item);
void drawLayer_toggle_lock(AppDraw *p,LayerItem *item);
void drawLayer_setLayerMask(AppDraw *p,LayerItem *item,int type);

void drawLayer_visibleAll(AppDraw *p,int type);
void drawLayer_visibleAll_checked_rev(AppDraw *p);
void drawLayer_visibleAll_img_rev(AppDraw *p);

void drawLayer_folder_open_all(AppDraw *p);
void drawLayer_folder_close_all(AppDraw *p);

void drawLayer_setFlags_all_off(AppDraw *p,uint32_t flags);

void drawLayer_toggle_tone_to_gray(AppDraw *p);

void drawLayer_moveUpDown(AppDraw *p,mlkbool up);
void drawLayer_moveDND(AppDraw *p,LayerItem *dst,int type);
void drawLayer_moveChecked_to_folder(AppDraw *p,LayerItem *dst);

void drawLayer_deleteForUndo(AppDraw *p,LayerItem *item);
void drawLayer_afterMoveList_forUndo(AppDraw *p);

void drawLayer_selectUpDown(AppDraw *p,mlkbool up);
void drawLayer_selectGrabLayer(AppDraw *p,int x,int y);

