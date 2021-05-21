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

#ifndef MLK_RAND_H
#define MLK_RAND_H

typedef struct _mRandXor mRandXor;
typedef struct _mRandSFMT mRandSFMT;

struct _mRandXor
{
	uint32_t x,y,z,w;
};


#ifdef __cplusplus
extern "C" {
#endif

void mRandXor_init(mRandXor *p,uint32_t seed);
uint32_t mRandXor_getUint32(mRandXor *p);
int mRandXor_getIntRange(mRandXor *p,int min,int max);
double mRandXor_getDouble(mRandXor *p);

mRandSFMT *mRandSFMT_new(void);
void mRandSFMT_free(mRandSFMT *p);
void mRandSFMT_init(mRandSFMT *p,uint32_t seed);
uint32_t mRandSFMT_getUint32(mRandSFMT *p);
uint64_t mRandSFMT_getUint64(mRandSFMT *p);
int mRandSFMT_getIntRange(mRandSFMT *p,int min,int max);
double mRandSFMT_getDouble(mRandSFMT *p);

#ifdef __cplusplus
}
#endif

#endif
