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

/*****************************************
 * <Linux> システム関連ユーティリティ
 *****************************************/

#include <stdlib.h>
#include <unistd.h>

#include "mDef.h"

#include "mUtilCharCode.h"


/***************//**

@defgroup util_sys mUtilSys
@brief システム関連ユーティリティ

@ingroup group_util
@{

@file mUtilSys.h

****************/

/** コマンド実行 */

mBool mExec(const char *cmd)
{
	char *path;
	pid_t pid;
	mBool ret = TRUE;

	path = mUTF8ToLocal_alloc(cmd, -1, NULL);
	if(!path) return FALSE;
	
	//プロセス複製

	pid = fork();

	if(pid < 0)
		ret = FALSE;
	else if(pid == 0)
	{
		execl("/bin/sh", "sh", "-c", path, (char *)0);
		exit(-1);
	}

	mFree(path);

	return ret;
}

/** @} */
