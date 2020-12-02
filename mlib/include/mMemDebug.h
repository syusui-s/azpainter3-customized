/*$
 Copyright (C) 2013-2020 Azel.

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

#ifdef MLIB_MEMDEBUG

#ifndef MLIB_MEMDEBUG_H
#define MLIB_MEMDEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

void *mMemDebug_malloc(size_t,const char *,int);
void *mMemDebug_realloc(void *,size_t,const char *,int);
void mMemDebug_free(void *);
char *mMemDebug_strdup(const char *,const char *,int);
char *mMemDebug_strndup(const char *,size_t,const char *,int);

#undef strdup
#undef strndup

#define malloc(sz)        mMemDebug_malloc(sz, __FILE__, __LINE__)
#define realloc(ptr,sz)   mMemDebug_realloc(ptr, sz, __FILE__, __LINE__)
#define free(ptr)         mMemDebug_free(ptr)
#define strdup(str)       mMemDebug_strdup(str, __FILE__, __LINE__)
#define strndup(str,len)  mMemDebug_strndup(str, len, __FILE__, __LINE__)

#ifdef __cplusplus
}
#endif

#endif

#endif
