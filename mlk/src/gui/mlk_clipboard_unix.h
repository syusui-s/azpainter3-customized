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

#ifndef MLK_CLIPBOARD_UNIX_H
#define MLK_CLIPBOARD_UNIX_H

typedef struct
{
	void *dat_buf;	//自身が持っているクリップボードデータ (NULL でなし)
	int dat_type;
	uint32_t dat_size;

	mList list_mime;	//持っているデータが送信可能な MIME タイプのリスト
	
	mFuncEmpty sendfunc;	//データを送る時の関数 (NULL でタイプごとのデフォルト)

	void *send_data;		//[送信中] バックエンドごとのデータ
	uint8_t send_is_self;	//[送信中] 自身に対する送信か (TRUE の場合、send_data は mBuf *)

	//各バックエンド用関数
	void (*setmimetype_default)(mList *,int);
	void (*addmimetype)(mList *,const char *);
	mlkbool (*setselection)(void);
	mlkbool (*sendto)(const void *,int);
	int (*gettext)(mStr *);
	int (*getdata)(const char *,mFuncEmpty,void *);
	char **(*getmimetypelist)(void);
}mClipboardUnix;


void mClipboardUnix_destroy(mClipboardUnix *p);
void mClipboardUnix_freeData(mClipboardUnix *p);

mlkbool mClipboardUnix_setData(mClipboardUnix *p,
	int type,const void *buf,uint32_t size,const char *mimetypes,mFuncEmpty handle);

mlkbool mClipboardUnix_send(mClipboardUnix *p,const void *buf,int size);

int mClipboardUnix_getText(mClipboardUnix *p,mStr *str);
int mClipboardUnix_getData(mClipboardUnix *p,const char *mimetype,mFuncEmpty handle,void *param);

char **mClipboardUnix_getMimeTypeList(mClipboardUnix *p);
char *mClipboardUnix_findMimeType(mClipboardUnix *p,const char *mime_types);

#endif
