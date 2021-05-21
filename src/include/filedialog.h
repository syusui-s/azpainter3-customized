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

/***********************************
 * ファイルダイアログ
 ***********************************/

typedef struct _LoadImageOption LoadImageOption;

mlkbool FileDialog_openImage(mWindow *parent,const char *filter,int def_filter,
	const char *initdir,mStr *strdst);

mlkbool FileDialog_openImage_confdir(mWindow *parent,const char *filter,int def_filter,mStr *strdst);

mlkbool FileDialog_openImage_forCanvas(mWindow *parent,const char *initdir,mStr *strdst,LoadImageOption *opt);
