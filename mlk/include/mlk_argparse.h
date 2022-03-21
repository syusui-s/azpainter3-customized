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

#ifndef MLK_ARGPARSE_H
#define MLK_ARGPARSE_H

typedef struct _mArgParse mArgParse;
typedef struct _mArgParseOpt mArgParseOpt;


struct _mArgParse
{
	int argc;
	char **argv;
	mArgParseOpt *opts,
		*curopt;
	uint32_t flags;
};

struct _mArgParseOpt
{
	const char *longopt;
	int16_t opt;
	uint16_t flags;
	void (*func)(mArgParse *p,char *arg);
};


enum MARGPARSE_FLAGS
{
	MARGPARSE_FLAGS_UNKNOWN_IS_ARG = 1<<0
};

enum MARGPARSEOPT_FLAGS
{
	MARGPARSEOPT_F_HAVE_ARG = 1<<0,
	MARGPARSEOPT_F_PROCESSED = 1<<15
};


#ifdef __cplusplus
extern "C" {
#endif

int mArgParseRun(mArgParse *p);

#ifdef __cplusplus
}
#endif

#endif
