/*$
 Copyright (C) 2013-2022 Azel.

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

/****************************
 * 共通のウィジェット関数
 ****************************/

mDialog *widget_createDialog(mWindow *parent,int size,const char *title,mFuncWidgetHandle_event event);

void widget_createLabel(mWidget *parent,const char *text);
void widget_createLabel_trid(mWidget *parent,int trid);
void widget_createLabel_width(mWidget *parent);
void widget_createLabel_height(mWidget *parent);

mLineEdit *widget_createEdit_num(mWidget *parent,int wlen,int min,int max,int dig,int val);
mLineEdit *widget_createEdit_num_change(mWidget *parent,int id,int wlen,int min,int max,int dig,int val);

mWidget *widget_createImgButton_1bitText(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack4,const uint8_t *imgbuf,int size,int minsize);
void widget_createHelpButton(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack4);
mWidget *widget_createMenuButton(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack4);
void widget_createEditButton(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack4);
mWidget *widget_createSaveButton(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack4);
mWidget *widget_createCopyButton(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack4,int minsize);
mWidget *widget_createPasteButton(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack4,int minsize);

mLineEdit *widget_createLabelEditNum(mWidget *parent,const char *label,int wlen,int min,int max,int val);
mComboBox *widget_createLabelCombo(mWidget *parent,const char *label,int id);
mColorButton *widget_createLabelColorButton(mWidget *parent,const char *label,int id,uint32_t col);

mCheckButton *widget_createColorBits(mWidget *parent,int is_8bit);
mIconBar *widget_createListEdit_iconbar_btt(mWidget *parent,const uint8_t *pbttno,mlkbool cancel);

mlkbool widget_message_confirm(mWindow *parent,const char *mes);
