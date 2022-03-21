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

#ifndef MLK_PACKBITS_H
#define MLK_PACKBITS_H

typedef struct _mPackBits mPackBits;

struct _mPackBits
{
	uint8_t *buf,
		*workbuf;
	uint32_t bufsize,
		worksize,
		encsize;

	void *param;

	mlkerr (*readwrite)(mPackBits *p,uint8_t *buf,int size);
};


#ifdef __cplusplus
extern "C" {
#endif

mlkerr mPackBits_decode(mPackBits *p);
mlkerr mPackBits_encode(mPackBits *p);

#ifdef __cplusplus
}
#endif

#endif
