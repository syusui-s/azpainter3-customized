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

#ifndef MLK_SIMD_H
#define MLK_SIMD_H

#define MLK_ENABLE_SSE    0
#define MLK_ENABLE_SSE2   0
#define MLK_ENABLE_SSE3   0
#define MLK_ENABLE_SSSE3  0
#define MLK_ENABLE_SSE4_1 0
#define MLK_ENABLE_SSE4_2 0
#define MLK_ENABLE_AVX    0
#define MLK_ENABLE_AVX2   0
#define MLK_ENABLE_FMA    0


#if !defined(MLK_NO_SIMD)

#ifdef HAVE_SSE
# include "xmmintrin.h"
# undef  MLK_ENABLE_SSE
# define MLK_ENABLE_SSE 1
#endif

#ifdef HAVE_SSE2
# include "emmintrin.h"
# undef  MLK_ENABLE_SSE2
# define MLK_ENABLE_SSE2 1
#endif

#ifdef HAVE_SSE3
# include "pmmintrin.h"
# undef  MLK_ENABLE_SSE3
# define MLK_ENABLE_SSE3 1
#endif

#ifdef HAVE_SSSE3
# include "tmmintrin.h"
# undef  MLK_ENABLE_SSSE3
# define MLK_ENABLE_SSSE3 1
#endif

#ifdef HAVE_SSE4_1
# include "smmintrin.h"
# undef  MLK_ENABLE_SSE4_1
# define MLK_ENABLE_SSE4_1 1
#endif

#ifdef HAVE_SSE4_2
# include "nmmintrin.h"
# undef  MLK_ENABLE_SSE4_2
# define MLK_ENABLE_SSE4_2 1
#endif

#ifdef HAVE_AVX
# undef  MLK_ENABLE_AVX
# define MLK_ENABLE_AVX 1
#endif

#ifdef HAVE_AVX2
# undef  MLK_ENABLE_AVX2
# define MLK_ENABLE_AVX2 1
#endif

#ifdef HAVE_FMA
# undef  MLK_ENABLE_FMA
# define MLK_ENABLE_FMA 1
#endif

#if defined(HAVE_AVX) || defined(HAVE_AVX2) || defined(HAVE_FMA)
# include "immintrin.h"
#endif

#endif //NO_SIMD

#endif
