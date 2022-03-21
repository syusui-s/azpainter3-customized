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

#ifndef MLK_H
#define MLK_H

#include "mlk_config.h"
#include <stdint.h>

typedef int       mlkerr;
typedef uint8_t   mlkbool;
typedef uintptr_t mlksize;
typedef int64_t   mlkfoff;
typedef uint32_t  mlkuchar;
typedef uint16_t  mlkuchar16;
typedef uint32_t  mRgbCol;
typedef uint32_t  mPixCol;

typedef struct _mBufIO mBufIO;
typedef struct _mIniRead   mIniRead;
typedef struct _mFileStat  mFileStat;
typedef struct _mImageBuf  mImageBuf;
typedef struct _mImageBuf2 mImageBuf2;
typedef struct _mImageList mImageList;
typedef struct _mPixbuf    mPixbuf;
typedef void * mIconTheme;
typedef struct _mThread mThread;
typedef void * mThreadMutex;
typedef void * mThreadCond;

typedef struct _mFontSystem mFontSystem;
typedef struct _mFont mFont;
typedef struct _mFontInfo mFontInfo;
typedef struct _mFontDrawInfo mFontDrawInfo;

typedef void (*mFuncEmpty)(void);


/*---- macro ----*/

#undef NULL
#undef TRUE
#undef FALSE

#define NULL    (0)
#define NULLPTR ((void *)0)
#define TRUE    (1)
#define FALSE   (0)

#define MLK_DIRSEP  '/'

#define MLK_MATH_PI    (3.14159265358979323846)
#define MLK_MATH_PI_2  (1.57079632679489661923)
#define MLK_MATH_1_PI  (0.318309886183790671538)
#define MLK_MATH_2_PI  (0.636619772367581343076)
#define MLK_MATH_DBL_EPSILON  (2.2204460492503131e-016)

#define MLK_MAKE32_2(hi,low)   (((uint32_t)(hi) << 16) | (low))
#define MLK_MAKE32_4(a,b,c,d)  (((uint32_t)(a) << 24) | ((b) << 16) | ((c) << 8) | (d))
#define MLK_RGB(r,g,b)         (((uint32_t)(r) << 16) | ((g) << 8) | (b))
#define MLK_ARGB(r,g,b,a)      (((uint32_t)(a) << 24) | ((r) << 16) | ((g) << 8) | (b))
#define MLK_RGB_R(c)           (((c) >> 16) & 0xff)
#define MLK_RGB_G(c)           (((c) >> 8) & 0xff)
#define MLK_RGB_B(c)           ((c) & 0xff)
#define MLK_ARGB_A(c)          ((c) >> 24)
#define MLK_FREE_NULL(p)       mFree(p); p = NULL
#define MLK_PTR_TO_UINT16(p)   (*((uint16_t *)(p)))
#define MLK_PTR_TO_UINT32(p)   (*((uint32_t *)(p)))
#define MLK_FLAG_ON(v,f)       v |= f
#define MLK_FLAG_OFF(v,f)      v &= ~(f)

#define MLK_TO_STR(a)  #a
#define MLK_TO_STR_MACRO(a)  MLK_TO_STR(a)

#if defined(__STDC_VERSION__) && __STDC_VERSION__ < 201102L
#define _Alignas(n)  __attribute__((aligned(n)))
#endif

/*---- enum ----*/

enum MLKERR
{
	MLKERR_OK = 0,
	MLKERR_UNKNOWN,
	MLKERR_ALLOC,
	MLKERR_OPEN,
	MLKERR_IO,
	MLKERR_OVER_DATA,
	MLKERR_NEED_MORE,
	MLKERR_INVALID_VALUE,
	MLKERR_UNSUPPORTED,
	MLKERR_MAX_SIZE,
	MLKERR_FORMAT_HEADER,
	MLKERR_DAMAGED,
	MLKERR_DECODE,
	MLKERR_ENCODE,
	MLKERR_LONGJMP,
	MLKERR_UNFOUND,
	MLKERR_EXIST,
	MLKERR_EMPTY
};

/*---- struct ----*/

typedef struct _mPoint
{
	int32_t x,y;
}mPoint;

typedef struct _mDoublePoint
{
	double x,y;
}mDoublePoint;

typedef struct _mSize
{
	int32_t w,h;
}mSize;

typedef struct _mRect
{
	int32_t x1,y1,x2,y2;
}mRect;

typedef struct _mBox
{
	int32_t x,y,w,h;
}mBox;

typedef struct _mBufSize
{
	void *buf;
	mlksize size;
}mBufSize;

typedef struct _mBuf
{
	uint8_t *buf;
	mlksize allocsize,
		expand_size,
		cursize;
}mBuf;

/* mStr */

typedef struct _mStr mStr;

struct _mStr
{
	char *buf;
	int len,allocsize;
};

#define MSTR_INIT {0,0,0}

/* mList */

typedef struct _mList     mList;
typedef struct _mListItem mListItem;
typedef struct _mListCacheItem mListCacheItem;

struct _mListItem
{
	mListItem *prev,*next;
};

struct _mListCacheItem
{
	mListItem *prev,*next;
	uint32_t refcnt;
};

struct _mList
{
	mListItem *top,*bottom;
	void (*item_destroy)(mList *,mListItem *);
	uint32_t num;
};

#define MLIST_INIT   {0,0,0,0}
#define MLISTITEM(p)      ((mListItem *)(p))
#define MLISTCACHEITEM(p) ((mListCacheItem *)(p))

/* mTree */

typedef struct _mTree mTree;
typedef struct _mTreeItem mTreeItem;

struct _mTreeItem
{
	mTreeItem *prev,*next,
		*first,*last,
		*parent;
};

struct _mTree
{
	mTreeItem *top,*bottom;
	void (*item_destroy)(mTree *,mTreeItem *);
};

#define MTREE_INIT    {0,0,0}
#define MTREEITEM(p)  ((mTreeItem *)(p))



/*---- function ----*/

#ifdef __cplusplus
extern "C" {
#endif

void mDebug(const char *format,...);
void mError(const char *format,...);
void mWarning(const char *format,...);

void mFree_r(void *ptr);
void *mMalloc_r(mlksize size);
void *mMalloc0_r(mlksize size);
void *mRealloc_r(void *ptr,mlksize size);
void *mMallocAlign_r(mlksize size,int alignment);
char *mStrdup_r(const char *str);
char *mStrndup_r(const char *str,int len);

void mMemset0(void *buf,mlksize size);
void *mMemdup(const void *src,mlksize size);

int mStrlen(const char *str);
int mStrdup_getlen(const char *str,char **dst);
char *mStrdup_free(char **dst,const char *src);
char *mStrdup_upper(const char *str);
int mStrncpy(char *dst,const char *src,int size);

mRgbCol mBlendRGB_a256(mRgbCol src,mRgbCol dst,int a);

/* memdebug */

#ifdef MLK_MEMDEBUG

void mFree_debug(void *ptr);
void *mMalloc_debug(mlksize size,const char *filename,int line);
void *mMalloc0_debug(mlksize size,const char *filename,int line);
void *mRealloc_debug(void *ptr,mlksize size,const char *filename,int line);
void *mMallocAlign_debug(mlksize size,int alignment,const char *filename,int line);
char *mStrdup_debug(const char *str,const char *filename,int line);
char *mStrndup_debug(const char *str,int len,const char *filename,int line);

#define mFree(ptr)           mFree_debug(ptr)
#define mMalloc(size)        mMalloc_debug(size, __FILE__, __LINE__)
#define mMalloc0(size)       mMalloc0_debug(size, __FILE__, __LINE__)
#define mRealloc(ptr,size)   mRealloc_debug(ptr, size, __FILE__, __LINE__)
#define mMallocAlign(size,align) mMallocAlign_debug(size, align, __FILE__, __LINE__)
#define mStrdup(str)         mStrdup_debug(str, __FILE__, __LINE__)
#define mStrndup(str,len)    mStrndup_debug(str, len, __FILE__, __LINE__)

#else

#define mFree(ptr)           mFree_r(ptr)
#define mMalloc(size)        mMalloc_r(size)
#define mMalloc0(size)       mMalloc0_r(size)
#define mRealloc(ptr,size)   mRealloc_r(ptr, size)
#define mMallocAlign(size,align) mMallocAlign_r(size, align)
#define mStrdup(str)         mStrdup_r(str)
#define mStrndup(str,len)    mStrndup_r(str, len)

#endif /* MLK_MEMDEBUG < */

#ifdef __cplusplus
}
#endif

#endif
