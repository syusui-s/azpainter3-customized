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
 * AppDraw:ファイル読み書き
 **********************************/

mlkerr drawFile_save_imageFile(AppDraw *p,const char *filename,uint32_t format,int dstbits,int falpha,mPopupProgress *prog);

mlkerr drawFile_load_apd_v4(AppDraw *p,const char *filename,mPopupProgress *prog);
mlkerr drawFile_save_apd_v4(AppDraw *p,const char *filename,mPopupProgress *prog);

mlkerr drawFile_load_apd_v1v2(const char *filename,mPopupProgress *prog);
mlkerr drawFile_load_apd_v3(AppDraw *p,const char *filename,mPopupProgress *prog);

mlkerr drawFile_load_adw(const char *filename,mPopupProgress *prog);

mlkbool drawFile_load_psd(AppDraw *p,const char *filename,mPopupProgress *prog);
mlkerr drawFile_save_psd(AppDraw *p,const char *filename,mPopupProgress *prog);
