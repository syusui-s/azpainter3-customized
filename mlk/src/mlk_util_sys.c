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

/*****************************************
 * システム関連ユーティリティ
 *****************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "mlk_platform.h"

//-----------

#if defined(MLK_PLATFORM_MACOS)
/* MacOS */
#include <mach-o/dyld.h>

#elif defined(MLK_PLATFORM_FREEBSD)
/* FreeBSD */
#include <sys/types.h>
#include <sys/sysctl.h>

#endif

//-----------

#include "mlk.h"
#include "mlk_charset.h"


/**@ プロセスごとに異なる名前を取得
 *
 * @d:Linux では、プロセスIDを文字列にしたもの。
 *
 * @r:確保された文字列 */

char *mGetProcessName(void)
{
	char m[32];
	
	snprintf(m, 32, "%d", (int)getpid());

	return mStrdup(m);
}

/**@ コマンド実行
 *
 * @g:システム関連
 *
 * @p:cmd 実行するコマンドの文字列 (UTF-8)
 * @r:成功したか */

mlkbool mExec(const char *cmd)
{
	char *str;
	pid_t pid;
	mlkbool ret = TRUE;

	str = mUTF8toLocale(cmd, -1, NULL);
	if(!str) return FALSE;
	
	//プロセス複製

	pid = fork();

	if(pid < 0)
		ret = FALSE;
	else if(pid == 0)
	{
		execl("/bin/sh", "sh", "-c", str, (char *)0);
		exit(-1);
	}

	mFree(str);

	return ret;
}

/**@ 自身の実行ファイルのパスを取得する
 *
 * @r:確保された文字列 (UTF-8)。エラー時は NULL。 */

char *mGetSelfExePath(void)
{
	char *buf,*dst;

	buf = (char *)mMalloc(2048);
	if(!buf) return NULL;

#if defined(MLK_PLATFORM_MACOS)
	/* MacOS */

	uint32_t len = 2048;

	if(_NSGetExecutablePath(buf, &len) != 0)
		len = 0;
	else
	{
		//パスを正規化
		
		dst = realpath(buf, NULL);
		if(!dst)
			len = 0;
		else
		{
			strcpy(buf, dst);
			free(dst);
		}
	}

#elif defined(MLK_PLATFORM_FREEBSD)
	/* FreeBSD */

	int mib[4];
	size_t len;

	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_PATHNAME;
	mib[3] = -1;
	
	len = 2048;

	if(sysctl(mib, 4, buf, &len, NULL, 0) != 0)
		len = 0;

#else
	/* Linux */

	ssize_t len;

	len = readlink("/proc/self/exe", buf, 2048);
	if(len == -1) len = 0;

#endif

	if(len == 0)
	{
		//エラー
		mFree(buf);
		return NULL;
	}
	else
	{
		dst = mLocaletoUTF8(buf, len, NULL);

		mFree(buf);

		return dst;
	}
}
