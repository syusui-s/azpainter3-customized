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
 * MainWindow : ファイル関連
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_popup_progress.h"
#include "mlk_sysdlg.h"
#include "mlk_str.h"

#include "def_draw.h"
#include "def_draw_sub.h"
#include "def_widget.h"
#include "def_config.h"
#include "def_saveopt.h"
#include "def_mainwin.h"

#include "mainwindow.h"
#include "pv_mainwin.h"
#include "popup_thread.h"
#include "dialogs.h"
#include "filedialog.h"

#include "fileformat.h"
#include "undo.h"

#include "draw_main.h"
#include "draw_file.h"

#include "trid.h"
#include "trid_mainmenu.h"


//----------------

mlkbool SaveOptionDlg_run(mWindow *parent,uint32_t format,int bits);

//----------------


/* 最近使ったファイルに追加 */

static void _add_recent_file(MainWindow *p,const char *filename)
{
	mStrArrayAddRecent(APPCONF->strRecentFile, CONFIG_RECENTFILE_NUM, filename);

	//メニュー

	MainWindow_setMenu_recentfile(p);
}

/** 新規作成 */

void MainWindow_newCanvas(MainWindow *p)
{
	NewCanvasValue dat;

	//ダイアログ

	if(!NewCanvasDialog_run(MLK_WINDOW(p), &dat))
		return;

	//保存確認

	if(!MainWindow_confirmSave(p)) return;

	//新規

	if(!drawImage_newCanvas(APPDRAW, &dat))
	{
		MainWindow_errmes(MLKERR_ALLOC, NULL);
		
		drawImage_newCanvas(APPDRAW, NULL);
	}

	//更新

	MainWindow_updateNewCanvas(p, "");
}


//============================
// ファイルを開く
//============================


typedef struct
{
	const char *filename;
	char *errmes;	//確保されたエラー文字列
	LoadImageOption opt;
	uint32_t format;
}_thread_openfileinfo;


/* 読み込みスレッド処理
 *
 * return: MLKERR */

static int _thread_load(mPopupProgress *prog,void *data)
{
	_thread_openfileinfo *p = (_thread_openfileinfo *)data;
	uint32_t format;
	mlkerr ret = MLKERR_OK;

	format = p->format;

	if(format & FILEFORMAT_APD)
	{
		//APD

		if(format & FILEFORMAT_APD_v4)
			//ver 4
			ret = drawFile_load_apd_v4(APPDRAW, p->filename, prog);
		else if(format & FILEFORMAT_APD_v3)
			//ver 3
			ret = drawFile_load_apd_v3(APPDRAW, p->filename, prog);
		else
			//ver 1,2
			ret = drawFile_load_apd_v1v2(p->filename, prog);
	}
	else if(format & FILEFORMAT_ADW)
	{
		//ADW

		ret = drawFile_load_adw(p->filename, prog);
	}
	else if(format & FILEFORMAT_PSD)
	{
		//PSD

		ret = drawFile_load_psd(APPDRAW, p->filename, prog);
	}
	else
	{
		//画像ファイル
		
		ret = drawImage_loadFile(APPDRAW, p->filename, p->format,
			&p->opt, prog, &p->errmes);
	}

	return ret;
}

/** 画像ファイルを読み込み
 *
 * opt: 開く時のオプション (NULL でデフォルト) */

mlkbool MainWindow_loadImage(MainWindow *p,const char *filename,LoadImageOption *opt)
{
	_thread_openfileinfo dat;
	uint32_t format;
	int err;

	//ヘッダからフォーマット取得

	format = FileFormat_getFromFile(filename);

	if(format == FILEFORMAT_UNKNOWN)
	{
		MainWindow_errmes(MLKERR_UNSUPPORTED, NULL);
		return FALSE;
	}

	//情報

	dat.filename = filename;
	dat.format = format;
	dat.errmes = NULL;

	if(opt)
		dat.opt = *opt;
	else
	{
		dat.opt.bits = APPCONF->loadimg_default_bits;
		dat.opt.ignore_alpha = FALSE;
	}

	//読み込み中、新規キャンバスが作成されたかのフラグ

	APPDRAW->fnewcanvas = FALSE;

	//スレッド

	APPDRAW->in_thread_imgcanvas = TRUE;

	err = PopupThread_run(&dat, _thread_load);

	APPDRAW->in_thread_imgcanvas = FALSE;

	if(err == -1) return FALSE;

	//結果

	if(err == MLKERR_OK)
	{
		//---- 成功

		mStr str = MSTR_INIT;

		//フォーマット

		p->fileformat = format;

		//ファイル履歴追加
		// :filename は履歴内の文字列のポインタの場合があるので、
		// :これ以降ファイル名を参照する場合は、ファイル履歴の先頭を参照すること。

		_add_recent_file(p, filename);

		//ディレクトリ履歴

		mStrPathGetDir(&str, APPCONF->strRecentFile[0].buf);

		mStrArrayAddRecent(APPCONF->strRecentOpenDir, CONFIG_RECENTDIR_NUM, str.buf);

		mStrFree(&str);

		//更新

		MainWindow_updateNewCanvas(p, APPCONF->strRecentFile[0].buf);

		return TRUE;
	}
	else
	{
		//---- 失敗

		//エラーメッセージ

		MainWindow_errmes(err, dat.errmes);

		mFree(dat.errmes);

		//エラー時の処理

		if(drawImage_loadError(APPDRAW))
			MainWindow_updateNewCanvas(p, filename);
		   
		return FALSE;
	}
}

/** ファイルを開く (ダイアログ)
 *
 * recentno: ディレクトリ番号 (-2 で現在の編集ファイルと同じ) */

void MainWindow_openFileDialog(MainWindow *p,int recentno)
{
	mStr str = MSTR_INIT,dir = MSTR_INIT;
	LoadImageOption opt;

	if(recentno == -2)
		mStrPathGetDir(&dir, p->strFilename.buf);
	else
		mStrCopy(&dir, APPCONF->strRecentOpenDir + recentno);

	//ファイル名

	if(FileDialog_openImage_forCanvas(MLK_WINDOW(p), dir.buf, &str, &opt))
	{
		//保存確認後、読み込み

		if(MainWindow_confirmSave(p))
			MainWindow_loadImage(p, str.buf, &opt);
	}

	mStrFree(&str);
	mStrFree(&dir);
}


//*************************************


//============================
// ファイル保存 sub
//============================


//[type] = format
static const uint32_t g_save_type_format[] = {
	FILEFORMAT_APD, FILEFORMAT_PSD, FILEFORMAT_PNG,
	FILEFORMAT_JPEG, FILEFORMAT_BMP, FILEFORMAT_GIF,
	FILEFORMAT_TIFF, FILEFORMAT_WEBP, 0
};


/* フォーマットフラグからファイルタイプ取得 */

static int _save_format_to_type(uint32_t format)
{
	int i;
		
	for(i = 0; g_save_type_format[i]; i++)
	{
		if(format & g_save_type_format[i])
			return i;
	}

	return SAVE_FORMAT_TYPE_APD;
}

/* 上書き保存時の確認メッセージ
 *
 * return: [0]キャンセル [1]上書き保存 [2]別名保存 */

static int _save_confirm_message(MainWindow *p)
{
	uint32_t ret;

	//ADW 形式での上書き保存は不可 => 別名保存

	if(p->fileformat & FILEFORMAT_ADW)
		return 2;

	//

	MLK_TRGROUP(TRGROUP_MESSAGE);

	//APD 以外の場合、確認

	if(!(p->fileformat & FILEFORMAT_APD)
		&& (APPCONF->foption & CONFIG_OPTF_MES_SAVE_APD))
	{
	
		ret = mMessageBox(MLK_WINDOW(p),
			MLK_TR(TRID_MESSAGE_TITLE_CONFIRM),
			MLK_TR(TRID_MESSAGE_SAVE_OVERWRITE_APD),
			MMESBOX_SAVE | MMESBOX_CANCEL | MMESBOX_NOTSHOW, MMESBOX_CANCEL);

		//メッセージ表示しない

		if(ret & MMESBOX_NOTSHOW)
			APPCONF->foption ^= CONFIG_OPTF_MES_SAVE_APD;

		return (ret & MMESBOX_SAVE)? 1: 0;
	}

	//上書き保存メッセージ

	if(APPCONF->foption & CONFIG_OPTF_MES_SAVE_OVERWRITE)
	{
		mStr str = MSTR_INIT;

		mStrPathGetBasename(&str, p->strFilename.buf);
		mStrAppendText(&str, "\n\n");
		mStrAppendText(&str, MLK_TR(TRID_MESSAGE_SAVE_OVERWRITE));

		ret = mMessageBox(MLK_WINDOW(p),
			MLK_TR(TRID_MESSAGE_TITLE_CONFIRM), str.buf,
			MMESBOX_SAVE | MMESBOX_CANCEL | MMESBOX_NOTSHOW, MMESBOX_SAVE);

		mStrFree(&str);

		//メッセージ表示しない

		if(ret & MMESBOX_NOTSHOW)
			APPCONF->foption ^= CONFIG_OPTF_MES_SAVE_OVERWRITE;

		//キャンセル

		if(!(ret & MMESBOX_SAVE))
			return 0;
	}

	return 1;
}

/* 保存ファイル名とフォーマットを取得
 *
 * strpath: 保存ファイル名が入る
 * return: 保存フォーマット。[-1] キャンセル [-2] 上書き保存 */

static int _save_get_path_and_format(MainWindow *p,
	mStr *strpath,int savetype,int recentno)
{
	mStr dir = MSTR_INIT;
	int ret,type,format,fexist;

	//現在のファイルパスがあるか

	fexist = mStrIsnotEmpty(&p->strFilename);

	//------ 上書き保存 & 新規状態でない

	if(savetype == SAVEFILE_TYPE_OVERWRITE && fexist)
	{
		ret = _save_confirm_message(p);

		if(ret == 0)
			//キャンセル
			return -1;
		else if(ret == 1)
		{
			//上書き保存

			mStrCopy(strpath, &p->strFilename);
			return -2;
		}
	}

	//------- 別名保存/複製保存

	//初期ファイル名
	// :編集ファイルがあれば、strpath に拡張子を除いてセット

	if(fexist)
		mStrPathGetBasename_noext(strpath, p->strFilename.buf);

	//種類の初期選択

	type = SAVE_FORMAT_TYPE_APD;

	if(savetype == SAVEFILE_TYPE_DUP)
	{
		//複製時は指定フォーマット

		if(APPCONF->savedup_type)
			type = APPCONF->savedup_type - 1;
		else
		{
			//0 = 現在のファイルと同じ

			if(fexist)
				type = _save_format_to_type(p->fileformat);
		}
	}

	//ディレクトリ

	if(recentno == -2)
		mStrPathGetDir(&dir, p->strFilename.buf);
	else
		mStrCopy(&dir, APPCONF->strRecentSaveDir + recentno);

	//ダイアログ

	ret = mSysDlg_saveFile(MLK_WINDOW(p),
		"AzPainter (*.apd)\tapd\t"
		"PhotoShop (*.psd)\tpsd\t"
		"PNG (*.png)\tpng\t"
		"JPEG (*.jpg;*.jpeg)\tjpg;jpeg\t"
		"BMP (*.bmp)\tbmp\t"
		"GIF (*.gif)\tgif\t"
		"TIFF (*.tiff;*.tif)\ttiff;tif\t"
		"WEBP (*.webp)\twebp\t",
		type, dir.buf, 0, strpath, &type);

	mStrFree(&dir);

	if(!ret) return -1;

	//フォーマット取得

	format = g_save_type_format[type];

	return format;
}


//============================
// ファイル保存 main
//============================


typedef struct
{
	const char *filename;
	uint32_t format;
}_thdata_save;


/* 保存スレッド */

static int _thread_save(mPopupProgress *prog,void *data)
{
	_thdata_save *p = (_thdata_save *)data;
	uint32_t format;
	int falpha,dstbits;
	mlkerr ret;

	format = p->format;

	if(format & FILEFORMAT_APD)
	{
		//----- APD v4
		
		//合成イメージ

		mPopupProgressThreadSetMax(prog, 20);

		ret = drawImage_blendImageReal_normal(APPDRAW, 8, prog, 20);
		if(ret) return ret;

		//保存

		mPopupProgressThreadSetPos(prog, 0);

		ret = drawFile_save_apd_v4(APPDRAW, p->filename, prog);

		//合成イメージ再セット

		drawUpdate_blendImage_full(APPDRAW, NULL);
	}
	else if(format & FILEFORMAT_PSD)
	{
		//----- PSD

		dstbits = 8;

		//16bit

		if(APPDRAW->imgbits == 16 && (APPCONF->save.psd & SAVEOPT_PSD_F_16BIT))
			dstbits = 16;

		//合成イメージ

		mPopupProgressThreadSetMax(prog, 20);

		ret = drawImage_blendImageReal_normal(APPDRAW, dstbits, prog, 20);
		if(ret) return ret;

		//保存

		mPopupProgressThreadSetPos(prog, 0);

		ret = drawFile_save_psd(APPDRAW, p->filename, prog);

		//合成イメージ再セット

		drawUpdate_blendImage_full(APPDRAW, NULL);
	}
	else
	{
		//----- PNG/JPEG/BMP/GIF/TIFF/WEBP
		// :プログレスは、合成時と保存時で２周する。

		dstbits = 8;
		falpha = FALSE;

		//アルファチャンネル

		if((format & FILEFORMAT_PNG) && (APPCONF->save.png & SAVEOPT_PNG_F_ALPHA))
			falpha = TRUE;

		//16bit カラー

		if(APPDRAW->imgbits == 16)
		{
			if(format & FILEFORMAT_PNG)
			{
				//PNG
				if(APPCONF->save.png & SAVEOPT_PNG_F_16BIT)
					dstbits = 16;
			}
			else if(format & FILEFORMAT_TIFF)
			{
				//TIFF
				if(APPCONF->save.tiff & SAVEOPT_TIFF_F_16BIT)
					dstbits = 16;
			}
		}

		//合成イメージ

		mPopupProgressThreadSetMax(prog, 20);

		if(falpha)
			//アルファ付き
			ret = drawImage_blendImageReal_alpha(APPDRAW, dstbits, prog, 20);
		else
			//アルファなし
			ret = drawImage_blendImageReal_normal(APPDRAW, dstbits, prog, 20);

		if(ret) return ret;

		//保存

		ret = drawFile_save_imageFile(APPDRAW, p->filename, format, dstbits, falpha, prog);

		//合成イメージ再セット

		drawUpdate_blendImage_full(APPDRAW, NULL);
	}

	return ret;
}

/** ファイル保存
 *
 * savetype: [0]上書き保存 [1]別名保存 [2]複製保存
 * recentno: ディレクトリ履歴番号 (-2 で現在の編集ファイルと同じ)
 * return: FALSE でキャンセルされた */

mlkbool MainWindow_saveFile(MainWindow *p,int savetype,int recentno)
{
	mStr str = MSTR_INIT,strdir = MSTR_INIT;
	uint32_t format;
	int ret;
	_thdata_save dat;

	//パスとフォーマット取得

	ret = _save_get_path_and_format(p, &str, savetype, recentno);

	if(ret == -1)
	{
		mStrFree(&str);
		return FALSE;
	}

	format = (ret == -2)? p->fileformat: ret;

	//WEBP サイズ制限

	if((format & FILEFORMAT_WEBP)
		&& (APPDRAW->imgw > 16383 || APPDRAW->imgh > 16383))
	{
		mStrFree(&str);

		mMessageBoxErrTr(MLK_WINDOW(p), TRGROUP_ERRMES, TRID_ERRMES_SAVE_WEBP_SIZE);
		return FALSE;
	}

	//保存設定
	// :上書き保存で、一度保存されている場合は除く。

	if(!(ret == -2 && p->fsaved)
		&& (format & FILEFORMAT_NEED_SAVEOPTION))
	{
		if(!SaveOptionDlg_run(MLK_WINDOW(p), format, APPDRAW->imgbits))
		{
			mStrFree(&str);
			return FALSE;
		}
	}

	//スレッド

	dat.filename = str.buf;
	dat.format = format;

	APPDRAW->in_thread_imgcanvas = TRUE;

	ret = PopupThread_run(&dat, _thread_save);

	APPDRAW->in_thread_imgcanvas = FALSE;

	//失敗

	if(ret)
	{
		mStrFree(&str);

		if(ret == -100)
			//GIF 256 色を超えている
			mMessageBoxErrTr(MLK_WINDOW(p), TRGROUP_ERRMES, TRID_ERRMES_SAVE_GIF_COLOR);
		else
			MainWindow_errmes(ret, NULL);
		
		return TRUE;
	}

	//----- 成功

	//現在のファイル情報をセット

	if(savetype != SAVEFILE_TYPE_DUP)
	{
		mStrCopy(&p->strFilename, &str);

		p->fileformat = format;
		p->fsaved = TRUE;

		//タイトル

		MainWindow_setTitle(p);

		//undo データ変更フラグ OFF

		Undo_setModifyFlag_off();
	}

	//ファイル履歴

	_add_recent_file(p, str.buf);
	
	//ディレクトリ履歴

	mStrPathGetDir(&strdir, str.buf);

	mStrArrayAddRecent(APPCONF->strRecentSaveDir, CONFIG_RECENTDIR_NUM, strdir.buf);

	//

	mStrFree(&str);
	mStrFree(&strdir);

	return TRUE;
}

