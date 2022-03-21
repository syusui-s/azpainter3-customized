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

/*****************************************
 * CHANGECOL
 *****************************************/

#include <math.h>

#include "mlk.h"
#include "mlk_color.h"

#include "changecol.h"
#include "draw_main.h"


/** CHANGECOL から、各値を取得 */

int ChangeColor_parse(int *cbuf,uint32_t col)
{
	cbuf[0] = CHANGECOL_GET_C1(col);
	cbuf[1] = CHANGECOL_GET_C2(col);
	cbuf[2] = CHANGECOL_GET_C3(col);

	return CHANGECOL_GET_TYPE(col);
}

/** CHANGECOL -> HSV (double) */

void ChangeColor_to_HSV(mHSVd *hsv,uint32_t col)
{
	int type,c[3];
	double d1,d2,dtmp,dtmp2;

	type = ChangeColor_parse(c, col);

	d1 = c[1] / 255.0;
	d2 = c[2] / 255.0;

	if(type == CHANGECOL_TYPE_HSV)
	{
		//HSV
		
		hsv->h = c[0] / 60.0;
		hsv->s = d1;
		hsv->v = d2;
	}
	else if(type == CHANGECOL_TYPE_HSL)
	{
		//HSL -> HSV (d1=s, d2=l)

		dtmp = d1 * (1 - fabs(2 * d2 - 1));
		dtmp2 = 2 * d2 + dtmp;
		
		hsv->h = c[0] / 60.0;
		hsv->s = (dtmp2 == 0)? 0: (2 * dtmp) / dtmp2;
		hsv->v = d2 + dtmp * 0.5;
	}
	else
	{
		//RGB -> HSV

		mRGB8pac_to_HSV(hsv, drawColor_getDrawColor());
	}
}

/** CHANGECOL -> HSV (int) */

void ChangeColor_to_HSV_int(int *dst,uint32_t col)
{
	int type,c[3];
	mHSVd hsv;

	type = ChangeColor_parse(c, col);

	if(type == CHANGECOL_TYPE_HSV)
	{
		dst[0] = c[0];
		dst[1] = c[1];
		dst[2] = c[2];
	}
	else
	{
		ChangeColor_to_HSV(&hsv, col);

		dst[0] = (int)(hsv.h * 60 + 0.5);
		dst[1] = (int)(hsv.s * 255 + 0.5);
		dst[2] = (int)(hsv.v * 255 + 0.5);
	}
}

/** CHANGECOL -> HSL (int) */

void ChangeColor_to_HSL_int(int *dst,uint32_t col)
{
	int type,c[3];
	double d1,d2,dtmp,dtmp2;
	mHSLd hsl;

	type = ChangeColor_parse(c, col);

	if(type != CHANGECOL_TYPE_HSL)
	{
		if(type == CHANGECOL_TYPE_HSV)
		{
			//HSV -> HSL

			d1 = c[1] / 255.0; //s
			d2 = c[2] / 255.0; //v
			dtmp = d2 * (2 - d1);
			dtmp2 = 1 - fabs(dtmp - 1); //v=0 で 0

			hsl.s = (dtmp2 == 0)? 0: d1 * d2 / dtmp2;
			hsl.l = dtmp * 0.5;
		}
		else
		{
			//RGB -> HSL
			
			mRGB8pac_to_HSL(&hsl, drawColor_getDrawColor());

			c[0] = (int)(hsl.h * 60 + 0.5);
		}

		//S,L
		c[1] = (int)(hsl.s * 255 + 0.5);
		c[2] = (int)(hsl.l * 255 + 0.5);
	}

	dst[0] = c[0];
	dst[1] = c[1];
	dst[2] = c[2];
}

