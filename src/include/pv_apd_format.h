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

/************************************
 * APD 読み書き 内部用 (v3,v4 共通)
 ************************************/

#define _MAKE_ID(a,b,c,d)  ((a << 24) | (b << 16) | (c << 8) | d)

//合成モードID
static uint32_t g_blendmode_id[] = {
	_MAKE_ID('n','o','r','m'), _MAKE_ID('m','u','l',' '), _MAKE_ID('a','d','d',' '), _MAKE_ID('s','u','b',' '),
	_MAKE_ID('s','c','r','n'), _MAKE_ID('o','v','r','l'), _MAKE_ID('h','d','l','g'), _MAKE_ID('s','t','l','g'),
	_MAKE_ID('d','o','d','g'), _MAKE_ID('b','u','r','n'), _MAKE_ID('l','b','u','n'), _MAKE_ID('v','i','v','l'),
	_MAKE_ID('l','i','n','l'), _MAKE_ID('p','i','n','l'), _MAKE_ID('l','i','g','t'), _MAKE_ID('d','a','r','k'),
	_MAKE_ID('d','i','f','f'), _MAKE_ID('l','m','a','d'), _MAKE_ID('l','m','d','g'), 0
};

