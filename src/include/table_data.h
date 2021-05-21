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

/****************************
 * テーブルデータ
 ****************************/

typedef struct
{
	double sintbl[512];	//sin テーブル (360度 = 512)
	uint8_t *ptonetbl;	//レイヤトーン化用テーブル
	int tone_bits;
}TableData;

extern TableData *g_app_tabledata;


#define TABLEDATA_GET_SIN(n)  (g_app_tabledata->sintbl[n & 511])
#define TABLEDATA_GET_COS(n)  (g_app_tabledata->sintbl[(n + 128) & 511])

#define TABLEDATA_TONE_WIDTH  256
#define TABLEDATA_TONE_BITS   8
#define TABLEDATA_TONE_GETVAL8(x,y)  *(g_app_tabledata->ptonetbl + ((y) & 255) * TABLEDATA_TONE_WIDTH + ((x) & 255))
#define TABLEDATA_TONE_GETVAL16(x,y) *((uint16_t *)g_app_tabledata->ptonetbl + ((y) & 255) * TABLEDATA_TONE_WIDTH + ((x) & 255))


void TableData_init(void);
void TableData_free(void);
void TableData_setToneTable(int bits);

void ToneTable_setData(uint8_t *tblbuf,int width,int bits);

