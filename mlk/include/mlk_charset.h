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

#ifndef MLK_CHARSET_H
#define MLK_CHARSET_H

typedef void * mIconv;

typedef struct _mIconvCallback
{
	int (*in)(void *buf,int size,void *param);
	int (*out)(void *buf,int size,void *param);
}mIconvCallback;


#ifdef __cplusplus
extern "C" {
#endif

void mInitLocale(void);
const char *mGetLocaleCharset(void);
mlkbool mLocaleCharsetIsUTF8(void);

mlkbool mIconvOpen(mIconv *p,const char *from,const char *to);
void mIconvClose(mIconv p);
char *mIconvConvert(mIconv p,const void *src,int srclen,int *dstlen,int nullsize);
void mIconvConvert_outfunc(mIconv p,const void *src,int srclen,int (*func)(void *buf,int size,void *param),void *param);
mlkbool mIconvConvert_callback(mIconv p,int inbufsize,int outbufsize,mIconvCallback *cb,void *param);

void *mConvertCharset(const void *src,int srclen,const char *from,const char *to,int *dstlen,int nullsize);

char *mUTF8toLocale(const char *str,int len,int *dstlen);
char *mLocaletoUTF8(const char *str,int len,int *dstlen);
mlkuchar *mLocaletoUTF32(const char *src,int len,int *dstlen);

char *mWidetoUTF8(const void *src,int len,int *dstlen);
mlkuchar *mWidetoUTF32(const void *src,int len,int *dstlen);
void *mLocaletoWide(const char *src,int len,int *dstlen);

void mPutUTF8(void *fp,const char *str,int len);
void mPutUTF8_stdout(const char *str);
void mPutUTF8_format_stdout(const char *format,const char *str);
void mPutUTF8_stderr(const char *str);

#ifdef __cplusplus
}
#endif

#endif
