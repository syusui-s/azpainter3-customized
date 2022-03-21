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

#ifndef MLK_GIFDEC_H
#define MLK_GIFDEC_H

typedef struct _mGIFDec mGIFDec;

enum MGIFDEC_FORMAT
{
	MGIFDEC_FORMAT_RAW = 0,
	MGIFDEC_FORMAT_RGB,
	MGIFDEC_FORMAT_RGBA
};


#ifdef __cplusplus
extern "C" {
#endif

mGIFDec *mGIFDec_new(void);
void mGIFDec_close(mGIFDec *p);

mlkbool mGIFDec_openFile(mGIFDec *p,const char *filename);
mlkbool mGIFDec_openBuf(mGIFDec *p,const void *buf,mlksize bufsize);
mlkbool mGIFDec_openFILEptr(mGIFDec *p,void *fp);

mlkerr mGIFDec_readHeader(mGIFDec *p);

void mGIFDec_getImageSize(mGIFDec *p,mSize *size);
uint8_t *mGIFDec_getPalette(mGIFDec *p,int *pnum);
int mGIFDec_getTransIndex(mGIFDec *p);
int32_t mGIFDec_getTransRGB(mGIFDec *p);

mlkerr mGIFDec_moveNextImage(mGIFDec *p);
mlkerr mGIFDec_getImage(mGIFDec *p);
mlkbool mGIFDec_getNextLine(mGIFDec *p,void *buf,int format,mlkbool trans_to_a0);

#ifdef __cplusplus
}
#endif

#endif
