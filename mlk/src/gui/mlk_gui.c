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
 * GUI メイン関数
 *****************************************/

#include <stdio.h>
#include <stdlib.h>

#include "mlk_gui.h"
#include "mlk_guicol.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_event.h"
#include "mlk_window_deco.h"

#include "mlk_translation.h"
#include "mlk_pixbuf.h"
#include "mlk_rectbox.h"
#include "mlk_font.h"
#include "mlk_fontinfo.h"
#include "mlk_fontconfig.h"
#include "mlk_str.h"
#include "mlk_list.h"
#include "mlk_charset.h"
#include "mlk_argparse.h"
#include "mlk_file.h"
#include "mlk_util.h"
#include "mlk_iniread.h"
#include "mlk_iniwrite.h"

#include "mlk_pv_gui.h"
#include "mlk_pv_widget.h"
#include "mlk_pv_window.h"
#include "mlk_pv_pixbuf.h"



//--------------------------

mAppBase *g_mlk_app = NULL;

//mlk_cursor.c
void __mCursorCacheInit(mList *list);

//mlk_x11_main.c
mlkbool __mGuiCheckX11(const char *name);
void __mGuiSetBackendX11(mAppBackend *p);

//--------------------------


//=============================
// sub
//=============================


static void _opt_trfile(mArgParse *p,char *arg)
{
	MLKAPP->opt_trfile = arg;
}

static void _opt_display(mArgParse *p,char *arg)
{
	MLKAPP->opt_dispname = arg;
}

static void _opt_name(mArgParse *p,char *arg)
{
	MLKAPP->opt_wmname = arg;
}

static void _opt_class(mArgParse *p,char *arg)
{
	MLKAPP->opt_wmclass = arg;
}

static void _opt_debug_mode(mArgParse *p,char *arg)
{
	MLKAPP->flags |= MAPPBASE_FLAGS_DEBUG_MODE;
}

/** コマンドラインオプション処理 */

static void _cmdline_option(mAppBase *p,int argc,char **argv,int *argtop)
{
	int ret;
	mArgParse ap;
	mArgParseOpt opts[] = {
		{"help-mlk", 0, 0, NULL},
		{"trfile", 0, MARGPARSEOPT_F_HAVE_ARG, _opt_trfile},
		{"display", 0, MARGPARSEOPT_F_HAVE_ARG, _opt_display},
		{"wmname", 0, MARGPARSEOPT_F_HAVE_ARG, _opt_name},
		{"wmclass", 0, MARGPARSEOPT_F_HAVE_ARG, _opt_class},
		{"debug-mode", 0, 0, _opt_debug_mode},
		{0,0,0,0}
	};

	ap.argc = argc;
	ap.argv = argv;
	ap.opts = opts;
	ap.flags = MARGPARSE_FLAGS_UNKNOWN_IS_ARG;

	ret = mArgParseRun(&ap);

	//ヘルプ

	if(ret == -1 || (opts[0].flags & MARGPARSEOPT_F_PROCESSED))
	{
		puts("[mlk options]\n"
"  --trfile=FILE   translation file (*.mtr)\n"
"  --display=NAME  display name\n"
"  --wmname=NAME   application name in window manager\n"
"  --wmclass=NAME  application class in window manager\n"
"  --debug-mode    for debug"
);
	
		mGuiEnd();
		exit(0);
	}

	//

	if(argtop) *argtop = ret;
}

/** デフォルトフォント作成 */

static void _create_default_font(mAppBase *p)
{
	mFontInfo info;

	if(p->style.fontstr)
	{
		//スタイル設定あり

		p->font_default = mFontCreate_text(p->fontsys, p->style.fontstr);
	}
	else
	{
		//デフォルト
		
		info.mask   = MFONTINFO_MASK_SIZE | MFONTINFO_MASK_WEIGHT | MFONTINFO_MASK_SLANT;
		info.size   = 10;
		info.weight = MFONTINFO_WEIGHT_REGULAR;
		info.slant  = MFONTINFO_SLANT_ROMAN;

		p->font_default = mFontCreate(p->fontsys, &info);

		p->style.fontstr = mStrdup("size=10");
	}
}

/** ループデータ追加 */

static void _add_rundata(mAppRunData *p,uint8_t type)
{
	p->run_back = MLKAPP->run_current;
	p->type = type;
	p->loop = 1;
	p->window = NULL;
	p->widget_sub = NULL;
	
	MLKAPP->run_current = p;
	MLKAPP->run_level++;
}


//=============================
// メイン関数
//=============================


/**@ GUI 終了処理
 *
 * @d:残っているすべてのウィジェットは、自動で削除される。 */

void mGuiEnd(void)
{
	mAppBase *p = MLKAPP;

	if(!p) return;

	//全ウィジェット削除
	
	if(p->widget_root)
	{
		mWidget *root = p->widget_root;
		
		while(root->first)
			mWidgetDestroy(root->first);
		
		mFree(root);
	}

	//カーソルキャッシュ

	mListDeleteAll(&p->list_cursor_cache);

	//バックエンド終了処理

	if(p->bkend.close)
		(p->bkend.close)();

	//フォント削除
	
	mFontFree(p->font_default);
	
	mFontSystem_finish(p->fontsys);
	mFontConfig_finish();

	//イベントリスト削除
	
	mEventListEmpty();

	//GUI タイマー削除

	mGuiTimerDeleteAll();

	//翻訳データ削除

	mTranslationFree(p->trans_def);
	mTranslationFree(p->trans_file);

	//文字列

	mFree(p->app_wmname);
	mFree(p->app_wmclass);
	mFree(p->path_config);
	mFree(p->path_data);

	//スタイルデータ

	__mGuiStyleFree(&p->style);

	//解放

	mFree(p);
	
	MLKAPP = NULL;
}

/**@ GUI 初期化
 *
 * @d:データの確保と共通の初期化処理を行う。\
 * バックエンドの初期化はまだ行われない。
 *
 * @p:argc 引数の数
 * @p:argv 引数の文字列。mlk で処理された分は先頭に配置される。
 * @p:argtop NULL 以外の場合、mlk で処理した分を除いた、引数の開始位置が入る。\
 *  mlk で処理したオプションがない場合、1。
 * @r:0 で成功、-1 でエラー */

int mGuiInit(int argc,char **argv,int *argtop)
{
	mAppBase *p;
	int ret;

	if(MLKAPP) return 0;

	//ロケール
	
	mInitLocale();
	
	//mAppBase 確保

	p = (mAppBase *)mMalloc0(sizeof(mAppBase));
	if(!p) return -1;

	MLKAPP = p;

	//

	p->opt_arg0 = argv[0];
	p->trans_cur_group = p->trans_save_group = -1;

	mEventListInit();

	//コマンドラインオプション
	
	_cmdline_option(p, argc, argv, argtop);

	//バッグエンド選択・関数セット

	ret = FALSE;

#if defined(MLK_HAVE_WAYLAND)
	
#endif

//#if defined(MLK_HAVE_X11)
	if(!ret)
	{
		ret = __mGuiCheckX11(p->opt_dispname);
		if(ret)
			__mGuiSetBackendX11(&p->bkend);
	}
//#endif

	if(!ret)
	{
		mError("open display\n");
		goto ERR;
	}

	//バックエンドデータ確保

	p->bkend_data = (p->bkend.alloc_data)();
	if(!p->bkend_data) goto ERR;

	//ルートウィジェット作成

	p->widget_root = mWidgetNew((mWidget *)1, 0);
	if(!p->widget_root) goto ERR;

	//フォントシステム

	if(!mFontConfig_init()) goto ERR;

	p->fontsys = mFontSystem_init();
	if(!p->fontsys) goto ERR;

	//カーソルキャッシュ

	__mCursorCacheInit(&p->list_cursor_cache);

	return 0;

ERR:
	mGuiEnd();
	return -1;
}

/**@ バックエンド初期化
 * 
 * @d:GUI バックエンドを初期化する。mGuiInit() 後に実行する。\
 *  また、スタイル設定の読み込みとセットなども行われる。\
 *  スタイル設定ファイル (az-mlk-style.conf) は、最初にアプリの設定ディレクトリから検索され、なければ、~/.config から検索される。\
 *  エラー時は自動で mGuiEnd() が呼ばれる。
 *
 * @r:0 で成功。ほか失敗。 */

int mGuiInitBackend(void)
{
	mAppBase *p = MLKAPP;

	//バックエンド初期化

	if(!p->bkend.init()) goto ERR;

	//スタイル設定読み込み

	mGuiCol_setDefaultRGB();

	__mGuiStyleLoad(p);

	//デフォルトフォント

	_create_default_font(p);

	//RGB -> PIX

	mGuiCol_RGBtoPix_all();

	return 0;

ERR:
	mGuiEnd();
	return -1;
}

/**@ ウィンドウマネージャで使われるクラス名をセット
 *
 * @d:mGuiInit() 〜 mGuiInitBkend() の間に行う。\
 * バックエンドで使用する場合がある。\
 * --wmname,--wmclass オプションがある場合、そちらが優先される。
 *
 * @p:name アプリ名。NULL で変更しない。
 * @p:classname クラス名。NULL で変更しない。\
 *  ウィンドウマネージャによっては、この名前がタスクバーに表示される。 */

void mGuiSetWMClass(const char *name,const char *classname)
{
	if(MLKAPP)
	{
		if(name)
		{
			mFree(MLKAPP->app_wmname);
			MLKAPP->app_wmname = mStrdup(name);
		}

		if(classname)
		{
			mFree(MLKAPP->app_wmclass);
			MLKAPP->app_wmclass = mStrdup(classname);
		}
	}
}

/**@ ペンタブレットを有効にする
 * 
 * @d:mGuiInit() 〜 mGuiInitBackend() 間の間に行う。\
 * バックエンドの初期化時に実際に適用される。 */

void mGuiSetEnablePenTablet(void)
{
	if(MLKAPP)
		MLKAPP->flags |= MAPPBASE_FLAGS_ENABLE_PENTABLET;
}

/**@ ユーザーアクション (キーやポインタ) のイベントをブロックするか
 *
 * @p:on  TRUE でブロックする。FALSE で解除。 */

void mGuiSetBlockUserAction(mlkbool on)
{
	if(on)
		MLKAPP->flags |= MAPPBASE_FLAGS_BLOCK_USER_ACTION;
	else
		MLKAPP->flags &= ~MAPPBASE_FLAGS_BLOCK_USER_ACTION;
}

/**@ GUI 用のデフォルトのフォントシステムを取得 */

mFontSystem *mGuiGetFontSystem(void)
{
	return MLKAPP->fontsys;
}

/**@ デフォルトのフォントを取得 */

mFont *mGuiGetDefaultFont(void)
{
	return MLKAPP->font_default;
}


//=================================
// メインループ
//=================================


/**@ メインループを抜ける */

void mGuiQuit(void)
{
	mAppRunData *run = MLKAPP->run_current;

	//loop = FALSE にすると、ループが終了して戻る

	if(run) run->loop = FALSE;
}

/**@ メインループを実行 (通常) */

void mGuiRun(void)
{
	mAppRunData dat;

	_add_rundata(&dat, MAPPRUN_TYPE_NORMAL);
	
	(MLKAPP->bkend.mainloop)(&dat, TRUE);
	
	MLKAPP->run_current = dat.run_back;
	MLKAPP->run_level--;
}

/**@ メインループ実行 (モーダルウィンドウ)
 *
 * @d:ダイアログ時に使う。\
 * modal のウィンドウ以外はアクティブにならない。 */

void mGuiRunModal(mWindow *modal)
{
	mAppRunData dat;

	_add_rundata(&dat, MAPPRUN_TYPE_MODAL);
	dat.window = modal;
	
	(MLKAPP->bkend.mainloop)(&dat, TRUE);
	
	MLKAPP->run_current = dat.run_back;
	MLKAPP->run_level--;
}

/**@ メインループ実行 (ポップアップウィンドウ)
 *
 * @d:popup ウィンドウ外がクリックされるなどした場合はループを抜ける。
 *
 * @p:send ポップアップウィンドウ以外でポインタイベントを送るウィジェット。NULL でなし。\
 * メニューバーから表示する場合、ポップアップ以外に、メニューバー上での操作も対象とする必要があるため。 */

void mGuiRunPopup(mPopup *popup,mWidget *send)
{
	mAppRunData dat;

	_add_rundata(&dat, MAPPRUN_TYPE_POPUP);

	dat.window = (mWindow *)popup;
	dat.widget_sub = send;
	
	(MLKAPP->bkend.mainloop)(&dat, TRUE);
	
	MLKAPP->run_current = dat.run_back;
	MLKAPP->run_level--;
}

/**@ 現在実行中のモーダルウィンドウを取得
 *
 * @r:NULL で、モーダルによるループ中ではない */

mWindow *mGuiGetCurrentModal(void)
{
	mAppRunData *run = MLKAPP->run_current;

	if(!run || run->type != MAPPRUN_TYPE_MODAL)
		return NULL;
	else
		return run->window;
}

/**@ スレッド時のロック
 *
 * @d:スレッド関数内で、GUI に関連する操作を行う前に、
 * ロックを行って、同時に値が操作されないようにする。 */

void mGuiThreadLock(void)
{
	if(MLKAPP->bkend.thread_lock)
		(MLKAPP->bkend.thread_lock)(TRUE);
}

/**@ スレッドのロックを解除 */

void mGuiThreadUnlock(void)
{
	if(MLKAPP->bkend.thread_lock)
		(MLKAPP->bkend.thread_lock)(FALSE);
}

/**@ スレッド内で、イベント待ちループを抜ける
 *
 * @d:スレッド関数内で実行する。\
 *  GUI のメインループ内で、ウィンドウのイベントを待っている状態の時 (poll など)、
 *  その状態を抜けて、GUI の処理を実行させる。 */

void mGuiThreadWakeup(void)
{
	if(MLKAPP->bkend.thread_wakeup)
		(MLKAPP->bkend.thread_wakeup)();
}


//=========================
// パス
//=========================


/**@ データ用ファイルのパスをセット
 *
 * @g:パス
 *
 * @d:プログラムで使用されるデータファイルのディレクトリを、
 * 実行ファイルからの相対パスで指定する。\
 * (ファイル構造さえ維持されていれば、どこにインストールされていても、そのまま実行できるようにするため)\
 * 実行時に直接パスを指定したい場合は、環境変数 MLK_APPDATADIR で絶対パスをセットする。
 *
 * @p:path 実行ファイルからの相対パス */

void mGuiSetPath_data_exe(const char *path)
{
	char *env;

	mFree(MLKAPP->path_data);
	MLKAPP->path_data = NULL;

	//

	env = getenv("MLK_APPDATADIR");

	if(env)
		//環境変数の絶対パス
		MLKAPP->path_data = mStrdup(env);
	else
	{
		//実行ファイルのディレクトリからの相対位置

		char *pc;
		mStr str = MSTR_INIT;
		
		pc = mGetSelfExePath();
		if(!pc) return;

		mStrSetText(&str, pc);
		mFree(pc);

		mStrPathRemoveBasename(&str);
		mStrPathJoin(&str, path);

		mStrPathNormalize(&str);

		MLKAPP->path_data = mStrdup(str.buf);

		mStrFree(&str);
	}
}

/**@ 設定ファイルのディレクトリのパスをセット (ホーム)
 *
 * @p:path ホームディレクトリからの相対パス */

void mGuiSetPath_config_home(const char *path)
{
	mStr str = MSTR_INIT;

	mFree(MLKAPP->path_config);

	//

	mStrPathSetHome_join(&str, path);

	MLKAPP->path_config = mStrdup(str.buf);

	mStrFree(&str);
}

//---- 取得

/**@ データディレクトリのパスを取得 */

const char *mGuiGetPath_data_text(void)
{
	return MLKAPP->path_data;
}

/**@ 設定ファイルディレクトリのパスを取得 */

const char *mGuiGetPath_config_text(void)
{
	return MLKAPP->path_config;
}

/**@ データディレクトリ内のパスを取得
 *
 * @p:path データディレクトリからの相対パス。\
 *  NULL でルートディレクトリを取得。 */

void mGuiGetPath_data(mStr *str,const char *path)
{
	mStrSetText(str, MLKAPP->path_data);
	mStrPathJoin(str, path);
}

/**@ 設定ファイルディレクトリ内のパスを取得
 *
 * @p:path 設定ファイルディレクトリからの相対パス */

void mGuiGetPath_config(mStr *str,const char *path)
{
	mStrSetText(str, MLKAPP->path_config);
	mStrPathJoin(str, path);
}

/**@ 内部用の特殊な記述で指定されたファイルのパスを取得
 *
 * @d:通常パスの場合は、そのまま複製して返す。\
 * パスが "!/" で始まっている場合、以降はデータディレクトリからの相対パスとして扱い、
 * 絶対パスを取得して返す。
 * 
 * @r:常に、確保された文字列 */

char *mGuiGetPath_data_sp(const char *path)
{
	if(path[0] == '!' && path[1] == '/')
	{
		mStr str = MSTR_INIT;

		mGuiGetPath_data(&str, path + 2);
		return str.buf;
	}
	else
		return mStrdup(path);
}


//---- 作成・コピーなど


/**@ 設定ファイルのディレクトリを作成
 *
 * @d:パスの中で、存在しないディレクトリはすべて作成される。
 *
 * @p:subpath NULL で、設定ファイルのルートディレクトリを作成する。\
 * ルートディレクトリ以下のサブディレクトリを作成する場合、相対パス名を指定する。
 *  
 * @r:MLKERR_OK で、作成に成功。MLKERR_EXIST で、すでに存在する。他はエラー。 */

mlkerr mGuiCreateConfigDir(const char *subpath)
{
	mStr str = MSTR_INIT;
	mlkerr ret;

	if(!(MLKAPP->path_config)) return MLKERR_UNKNOWN;

	mGuiGetPath_config(&str, subpath);

	ret = mCreateDir_parents(str.buf, -1);

	mStrFree(&str);

	return ret;
}

/**@ データディレクトリから設定ファイルディレクトリにファイルをコピー
 *
 * @d:設定ファイルディレクトリ内にファイルが存在していない場合のみコピーする。
 *
 * @p:srcpath ソースのファイル名。データディレクトリからの相対パス。
 * @p:dstdir コピー先のパスとファイル名。設定ファイルディレクトリからの相対パス。
 * @r:コピーが行われたか */

mlkbool mGuiCopyFile_dataToConfig(const char *srcpath,const char *dstpath)
{
	mStr str_data = MSTR_INIT, str_conf = MSTR_INIT;
	mlkbool ret = FALSE;

	mGuiGetPath_config(&str_conf, dstpath);

	if(!mIsExistFile(str_conf.buf))
	{
		mGuiGetPath_data(&str_data, srcpath);

		mCopyFile(str_data.buf, str_conf.buf);

		mStrFree(&str_data);

		ret = TRUE;
	}
	
	mStrFree(&str_conf);

	return ret;
}


//=================================
//
//=================================


/**@ システムで維持している設定情報を INI ファイルに書き込み
 *
 * @d:以下、グループ名。\
 *  mlk_filedialog = ファイルダイアログの設定 */

void mGuiWriteIni_system(void *fp)
{
	mAppBase *p = MLKAPP;
	FILE *pw = (FILE *)fp;

	mIniWrite_putGroup(pw, "mlk_filedialog");
	
	mIniWrite_putInt(pw, "width", p->filedlgconf.width);
	mIniWrite_putInt(pw, "height", p->filedlgconf.height);
	mIniWrite_putInt(pw, "sort_type", p->filedlgconf.sort_type);
	mIniWrite_putInt(pw, "flags", p->filedlgconf.flags);
}

/**@ システムで維持する設定情報を INI ファイルから読み込み */

void mGuiReadIni_system(mIniRead *ini)
{
	mAppBase *p = MLKAPP;

	mIniRead_setGroup(ini, "mlk_filedialog");

	p->filedlgconf.width = mIniRead_getInt(ini, "width", 0);
	p->filedlgconf.height = mIniRead_getInt(ini, "height", 0);
	p->filedlgconf.sort_type = mIniRead_getInt(ini, "sort_type", 0);
	p->filedlgconf.flags = mIniRead_getInt(ini, "flags", 0);
}


//=================================
// 翻訳
//=================================


/**@ デフォルトの埋め込み用翻訳データのみセットする
 *
 * @g:翻訳
 *
 * @d:翻訳ファイルを使わず、特定の言語のみ固定で使う場合。 */

void mGuiSetTranslationEmbed(const void *buf)
{
	MLKAPP->trans_def = mTranslationLoadDefault(buf);
}

/**@ 翻訳データを読み込み
 *
 * @d:埋め込みデータからデフォルトの翻訳データを作成し、
 * データディレクトリの指定パス内から、必要な言語の翻訳ファイルを読み込む。\
 * コマンドライン引数で翻訳ファイルが指定されている場合は、強制的にそのファイルが読み込まれる。
 *
 * @p:defbuf デフォルト言語の埋め込みデータ
 * @p:lang   読み込む言語名 (ja_JP など)。\
 *  NULL で、環境変数からシステムの言語が指定される。
 * @p:path   データディレクトリ内の翻訳ファイル検索パス。\
 *  データディレクトリからの相対パスで指定する。\
 *  NULL でデータディレクトリのルート位置。 */

void mGuiLoadTranslation(const void *defbuf,const char *lang,const char *path)
{
	mAppBase *app = MLKAPP;

	//埋め込みデータ

	app->trans_def = mTranslationLoadDefault(defbuf);

	//翻訳ファイル読み込み

	if(app->opt_trfile)
	{
		//コマンドラインでファイル指定あり
		
		app->trans_file = mTranslationLoadFile(app->opt_trfile, TRUE);
	}
	else
	{
		//データディレクトリから言語名で検索

		mStr str = MSTR_INIT;

		mGuiGetPath_data(&str, path);

		app->trans_file = mTranslationLoadFile_lang(str.buf, lang, NULL);

		mStrFree(&str);
	}
}

/**@ 翻訳のカレントグループをセット */

void mGuiTransSetGroup(uint16_t id)
{
	mTranslationSetGroup(MLKAPP->trans_def, id);
	mTranslationSetGroup(MLKAPP->trans_file, id);

	MLKAPP->trans_cur_group = id;
}

/**@ 現在のグループを記録 */

void mGuiTransSaveGroup(void)
{
	MLKAPP->trans_save_group = MLKAPP->trans_cur_group;
}

/**@ 記録されたグループを復元 */

void mGuiTransRestoreGroup(void)
{
	mAppBase *p = MLKAPP;

	if(p->trans_save_group != -1)
	{
		mGuiTransSetGroup(p->trans_save_group);
	
		p->trans_save_group = -1;
	}
}

/**@ 翻訳のカレントグループから文字列を取得
 *
 * @r:見つからなかった場合、空文字列 */

const char *mGuiTransGetText(uint16_t id)
{
	const char *pc;

	pc = mTranslationGetText(MLKAPP->trans_file, id);
	if(!pc)
	{
		pc = mTranslationGetText(MLKAPP->trans_def, id);
		if(!pc) pc = "";
	}

	return pc;
}

/**@ 翻訳のカレントグループから文字列を取得
 *
 * @r:見つからなかった場合、NULL */

const char *mGuiTransGetTextRaw(uint16_t id)
{
	const char *pc;

	pc = mTranslationGetText(MLKAPP->trans_file, id);
	if(!pc)
		pc = mTranslationGetText(MLKAPP->trans_def, id);

	return pc;
}

/**@ 翻訳のカレントグループから、埋め込みデータのデフォルト言語の文字列を取得
 *
 * @r:見つからなかった場合、空文字列 */

const char *mGuiTransGetTextDefault(uint16_t id)
{
	const char *pc;

	pc = mTranslationGetText(MLKAPP->trans_def, id);

	return (pc)? pc: "";
}

/**@ 翻訳から、グループIDと文字列IDを指定して、文字列を取得
 *
 * @d:この場合、カレントグループは変更されない。
 *
 * @r:見つからなかった場合、空文字列 */

const char *mGuiTransGetText2(uint16_t groupid,uint16_t strid)
{
	const char *pc;

	pc = mTranslationGetText2(MLKAPP->trans_file, groupid, strid);
	if(!pc)
	{
		pc = mTranslationGetText2(MLKAPP->trans_def, groupid, strid);
		if(!pc) pc = "";
	}

	return pc;
}

/**@ 翻訳から、グループIDと文字列IDを指定して、文字列を取得
 *
 * @r:見つからなかった場合、NULL */

const char *mGuiTransGetText2Raw(uint16_t groupid,uint16_t strid)
{
	const char *pc;

	pc = mTranslationGetText2(MLKAPP->trans_file, groupid, strid);
	if(!pc)
		pc = mTranslationGetText2(MLKAPP->trans_def, groupid, strid);

	return pc;
}


//=================================
// ほか
//=================================


/**@ レイアウトサイズを計算する
 *
 * @g:ほか
 *
 * @d:計算対象のすべてのウィジェットで、calc_hint ハンドラを実行して、
 * レイアウトサイズを計算する。 */

void mGuiCalcHintSize(void)
{
	mWidget *p;

	if(MLKAPP->widget_root->fui & MWIDGET_UI_FOLLOW_CALC)
	{
		//一番下位の対象ウィジェット取得

		p = __mWidgetGetTreeBottom_uiflag(NULL, MWIDGET_UI_FOLLOW_CALC);

		while(p)
		{
			//MWIDGET_UI_CALC が ON の場合のみ実行

			__mWidgetCalcHint(p);

			//次の対象ウィジェット取得 (FOLLOW フラグを OFF に)

			p = __mWidgetGetTreePrev_uiflag(p,
					MWIDGET_UI_FOLLOW_CALC, MWIDGET_UI_CALC);
		}
	}
}

/**@ すべてのウィンドウの更新範囲を画面に転送
 *
 * @d:各ウィンドウで更新範囲がある場合、その部分を画面に転送する。
 *
 * @r:一つでもウィンドウを更新したか (ウィンドウ装飾を含む)。 */

mlkbool mGuiRenderWindows(void)
{
	mWindow *p;
	mPixbuf *pixbuf;
	mRect rc;
	mWindowDecoInfo info;
	mlkbool ret = FALSE;
	
	for(p = MLK_WINDOW(MLKAPP->widget_root->first); p; p = MLK_WINDOW(p->wg.next))
	{
		if((p->wg.fstate & MWIDGET_STATE_VISIBLE) && p->win.pixbuf)
		{
			//----- ウィンドウ装飾の描画
			//(ウィジェットはすでに描画されているので上書きとなる)

			if((p->wg.fui & MWIDGET_UI_DECO_UPDATE)
				&& p->win.deco.draw)
			{
				pixbuf = p->win.pixbuf;
			
				__mPixbufSetClipRoot(pixbuf, NULL);
				mPixbufSetOffset(pixbuf, 0, 0, NULL);
				
				__mWindowDecoSetInfo(p, &info);

				(p->win.deco.draw)(MLK_WIDGET(p), pixbuf, &info);
				
				ret = TRUE;
			}
		
			//----- pixbuf 転送
		
			if(p->wg.fui & MWIDGET_UI_UPDATE)
			{
				rc = p->win.update_rc;

				if(mRectClipBox_d(&rc, 0, 0, p->win.win_width, p->win.win_height))
				{
					(MLKAPP->bkend.pixbuf_render)(p,
						rc.x1, rc.y1,
						rc.x2 - rc.x1 + 1, rc.y2 - rc.y1 + 1);

					ret = TRUE;
				}
			}
		}

		//

		p->wg.fui &= ~(MWIDGET_UI_UPDATE | MWIDGET_UI_DECO_UPDATE);
	}

	return ret;
}

/**@ ウィジェットの描画処理を行う
 *
 * @d:更新のあるウィジェットを、ウィンドウのイメージに描画する。\
 * 上位のウィジェットから順に描画される。\
 * 非表示状態の場合は、スキップされる。 */

void mGuiDrawWidgets(void)
{
	mWidget *p = NULL;
	mWindow *win;
	mPixbuf *pixbuf;
	mRect rc;
	mPoint pt;
	
	while(1)
	{
		//次の対象ウィジェット取得
	
		p = __mWidgetGetTreeNext_draw(p);
		if(!p) break;
		
		//ウィンドウ座標での範囲
		// :ウィンドウの範囲内にクリッピングされる。FALSE で範囲外。

		if(__mWidgetGetWindowRect(p, &rc, &pt, TRUE))
		{
			//描画

			win = p->toplevel;
			pixbuf = win->win.pixbuf;

			if(pixbuf && p->draw && __mPixbufSetClipRoot(pixbuf, &rc))
			{
				mPixbufSetOffset(pixbuf, pt.x, pt.y, NULL);

				(p->draw)(p, pixbuf);

				__mPixbufSetClipRoot(pixbuf, NULL);
				mPixbufSetOffset(pixbuf, 0, 0, NULL);
			}
		}
	}
}

/**@ すべてのウィンドウを強制的に再描画させる
 *
 * @d:すべてのウィンドウで mWidgetRedraw() を行い、全体を再描画させる。\
 * (ここでは、実際には描画されず、再描画の情報だけセットされる)\
 * GUI 色の変更時などに使う。 */

void mGuiUpdateAllWindows(void)
{
	mWidget *p;

	for(p = MLKAPP->widget_root->first; p; p = p->next)
		mWidgetRedraw(p);
}


//---- スタイル


/**@ デフォルトのフォントの情報を取得 */

void mGuiGetDefaultFontInfo(mFontInfo *info)
{
	mFontInfoSetFromText(info, MLKAPP->style.fontstr);
}

/**@ 現在のスタイルのアイコンテーマ名を取得
 *
 * @d:スタイルファイルで指定がない場合、各ディレクトリで最初に見つかったテーマが指定されている。\
 *  ※ MLK_NO_ICONTHEME マクロが定義されている場合は、検索されない。\
 *  見つからなかった場合は、NULL。 */

const char *mGuiGetIconTheme(void)
{
	return MLKAPP->style.icontheme;
}

/**@ ファイルダイアログ時、隠しファイルを表示させる
 *
 * @d:デフォルトで OFF になっている。 */

void mGuiFileDialog_showHiddenFiles(void)
{
	MLKAPP->filedlgconf.flags |= MAPPBASE_FILEDIALOGCONF_F_SHOW_HIDDEN_FILES;
}


//===============================
// 内部処理用
//===============================


/** ウィジェットの構成ハンドラ実行
 *
 * レイアウトを行う前や、内部イベント処理後に行う */

void __mAppRunConstruct(void)
{
	mAppBase *app = MLKAPP;
	mWidget *wg;

	if(app->widget_construct_top)
	{
		//ウィジェットのリストになっている
	
		for(wg = app->widget_construct_top; wg; wg = wg->construct_next)
		{
			wg->fui &= ~MWIDGET_UI_CONSTRUCT;

			if(wg->construct)
				(wg->construct)(wg);
		}

		//リストをクリア

		app->widget_construct_top = NULL;
	}
}

/** wg を、構成変更のウィジェットリストから削除 (ウィジェット削除時) */

void __mAppDelConstructWidget(mWidget *wg)
{
	mWidget *p,*prev = NULL;

	for(p = MLKAPP->widget_construct_top; p; p = p->construct_next)
	{
		if(p == wg)
		{
			if(prev)
				//一つ前と次をつなげる
				prev->construct_next = p->construct_next;
			else
				//先頭の場合
				MLKAPP->widget_construct_top = p->construct_next;
			
			break;
		}
	
		prev = p;
	}
}

/** バックエンドごとのイベントを処理をした後に行う共通処理
 *
 * return: ウィンドウイメージが一つでも転送された */

mlkbool __mAppAfterEvent(void)
{
	mAppBase *p = MLKAPP;

	//内部イベントと構成
	//(mListView でヘッダに拡張アイテムがある場合など、
	// 構成とイベントが複数続く場合があるため、データがある限り実行)

	while(p->run_current->loop
		&& (p->widget_construct_top || p->list_event.top))
	{
		MLKAPP->bkend.run_event();

		__mAppRunConstruct();
	}

	//描画と転送

	mGuiDrawWidgets();

	return mGuiRenderWindows();
}


#ifdef MLK_DEBUG_PUT_EVENT

#define _PUT_EVENT(...)  fprintf(stderr, __VA_ARGS__)

/** 内部イベントのデバッグ情報出力 */

void __mAppPutEventDebug(mEvent *ev)
{
	static uint32_t no = 0;

	fprintf(stderr, "@ev[%u]: ", no++);

	switch(ev->type)
	{
		case MEVENT_POINTER:
		case MEVENT_POINTER_MODAL:
			_PUT_EVENT("%s: %p: act(%d) x(%d) y(%d) fx(%.2f) fy(%.2f) btt(%d) rawbtt(%d) state(0x%x)\n",
				(ev->type == MEVENT_POINTER)? "POINTER": "POINTER_MODAL",
				ev->widget,
				ev->pt.act, ev->pt.x, ev->pt.y, ev->pt.fixed_x / 256.0, ev->pt.fixed_y / 256.0,
				ev->pt.btt, ev->pt.raw_btt, ev->pt.state);
			break;
		case MEVENT_PENTABLET:
		case MEVENT_PENTABLET_MODAL:
			_PUT_EVENT("%s: %p: act(%d) x(%.2f) y(%.2f) btt(%d) rawbtt(%d) pressure(%.2f) state(0x%x) flags(0x%x)\n",
				(ev->type == MEVENT_PENTABLET)? "PENTABLET": "PENTABLET_MODAL",
				ev->widget,
				ev->pentab.act, ev->pentab.x, ev->pentab.y, ev->pentab.btt, ev->pentab.raw_btt,
				ev->pentab.pressure, ev->pentab.state, ev->pentab.flags);
			break;
		case MEVENT_ENTER:
			_PUT_EVENT("ENTER: %p: x(%d) y(%d) fx(%.2f) fy(%.2f)\n",
				ev->widget, ev->enter.x, ev->enter.y, ev->enter.fixed_x / 256.0, ev->enter.fixed_y / 256.0);
			break;
		case MEVENT_LEAVE:
			_PUT_EVENT("LEAVE: %p\n", ev->widget);
			break;
		case MEVENT_SCROLL:
			_PUT_EVENT("SCROLL: %p: hval(%d) vval(%d) hstep(%d) vstep(%d) state(0x%x)\n",
				ev->widget, ev->scroll.horz_val, ev->scroll.vert_val,
				ev->scroll.horz_step, ev->scroll.vert_step, ev->scroll.state);
			break;
		case MEVENT_NOTIFY:
			_PUT_EVENT("NOTIFY: %p: type(%d) from(%p) id(%d) param1(%ld) param2(%ld)\n",
				ev->widget, ev->notify.notify_type, ev->notify.widget_from,
				ev->notify.id, ev->notify.param1, ev->notify.param2);
			break;
		case MEVENT_COMMAND:
			_PUT_EVENT("COMMAND: %p: id(%d) from(%d) param(%ld)\n",
				ev->widget, ev->cmd.id, ev->cmd.from, ev->cmd.param);
			break;
		case MEVENT_KEYDOWN:
		case MEVENT_KEYUP:
			_PUT_EVENT("KEY%s: %p: key(0x%x) state(0x%x) raw_keysym(0x%x) raw_code(%d)\n", 
				(ev->type == MEVENT_KEYDOWN)?"DOWN":"UP", ev->widget,
				ev->key.key, ev->key.state, ev->key.raw_keysym, ev->key.raw_code);
			break;
		case MEVENT_CHAR:
			_PUT_EVENT("CHAR: %p: '%c'\n", ev->widget, ev->ch.ch);
			break;
		case MEVENT_STRING:
			_PUT_EVENT("STRING: %p: '%s' (%d)\n", ev->widget, ev->str.text, ev->str.len);
			break;
		case MEVENT_TIMER:
			_PUT_EVENT("TIMER: %p: id(%d) param(%ld)\n",
				ev->widget, ev->timer.id, ev->timer.param);
			break;
		case MEVENT_FOCUS:
			_PUT_EVENT("FOCUS: %p: type(%s) from(%d)\n", ev->widget, (ev->focus.is_out)?"out":"in", ev->focus.from);
			break;
		case MEVENT_WINDECO:
			_PUT_EVENT("WINDECO: %p: type(%d) x(%d) y(%d) btt(%d)\n",
				ev->widget, ev->deco.decotype,
				ev->deco.x, ev->deco.y, ev->deco.btt);
			break;
		case MEVENT_CONFIGURE:
			_PUT_EVENT("CONFIGURE: %p: size(%dx%d) state(0x%x) state_mask(0x%x) flags(0x%x)\n",
				ev->widget, ev->config.width, ev->config.height,
				ev->config.state, ev->config.state_mask, ev->config.flags);
			break;
		case MEVENT_MAP:
			_PUT_EVENT("MAP: %p\n", ev->widget);
			break;
		case MEVENT_UNMAP:
			_PUT_EVENT("UNMAP: %p\n", ev->widget);
			break;
		case MEVENT_CLOSE:
			_PUT_EVENT("CLOSE: %p\n", ev->widget);
			break;
		case MEVENT_PANEL:
			_PUT_EVENT("PANEL: id(%d) act(%d) param1(%ld) param2(%ld)\n",
				ev->panel.id, ev->panel.act, ev->panel.param1, ev->panel.param2);
			break;

		//- MEVENT_MENU_POPUP は直接ハンドラ関数が呼ばれるため、ここに来ない。

		//内部で使われるイベント
		default:
			_PUT_EVENT("<%d>\n", ev->type);
			break;
	}

	fflush(stderr);
}

#endif
