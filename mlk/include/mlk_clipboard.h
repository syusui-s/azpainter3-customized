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

#ifndef MLK_CLIPBOARD_H
#define MLK_CLIPBOARD_H

typedef struct _mClipboardSendData mClipboardSendData;

typedef int (*mFuncClipboardSend)(mClipboardSendData *p);
typedef int (*mFuncClipboardRecv)(void *buf,uint32_t size,void *param);

struct _mClipboardSendData
{
	const char *mimetype;
	void *buf;
	int type;
	uint32_t size;
};

enum MCLIPBOARD_TYPE
{
	MCLIPBOARD_TYPE_DATA,
	MCLIPBOARD_TYPE_TEXT,
	MCLIPBOARD_TYPE_USER = 1000
};

enum MCLIPBOARD_RET
{
	MCLIPBOARD_RET_OK,
	MCLIPBOARD_RET_NONE,
	MCLIPBOARD_RET_SEND_RAW,
	MCLIPBOARD_RET_ERR,
	MCLIPBOARD_RET_ERR_TYPE
};


#ifdef __cplusplus
extern "C" {
#endif

mlkbool mClipboardRelease(void);
mlkbool mClipboardSetData(int type,const void *buf,uint32_t size,const char *mimetypes,mFuncClipboardSend handle);
mlkbool mClipboardSetText(const char *text,int len);
mlkbool mClipboardSend(const void *buf,int size);

int mClipboardGetText(mStr *str);
int mClipboardGetData(const char *mimetype,mFuncClipboardRecv handle,void *param);

char **mClipboardGetMimeTypeList(void);
char *mClipboardFindMimeType(const char *mime_types);

#ifdef __cplusplus
}
#endif

#endif
