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

/*****************************
 * 設定データ
 *****************************/

/** 個数 */

#define CONFIG_RECENTFILE_NUM   10	//最近使ったファイル
#define CONFIG_RECENTDIR_NUM    8   //ディレクトリの履歴
#define CONFIG_NEWCANVAS_NUM    10	//新規作成の履歴と登録
#define CONFIG_GRID_RECENT_NUM  6	//グリッド履歴
#define CONFIG_POINTERBTT_NUM   16  //ポインタデバイスのボタン最大数
#define CONFIG_WATER_PRESET_NUM 5	//水彩プリセットの数
#define CONFIG_TEXTSIZE_RECENT_NUM 10 //テキストのサイズの履歴

#define CONFIG_POINTERBTT_MAXNO 15  //ポインタデバイスの最大ボタン番号


/** キャンバス新規作成の履歴/記録データ */

typedef struct
{
	int32_t w,h;
	uint16_t dpi;
	uint8_t unit, //0:px,1:mm,2:cm,3:inch
		bit;	//0=データなし, 8or16
}ConfigNewCanvas;

/** レイヤのテンプレートアイテム */

typedef struct
{
	mListItem i;
	uint8_t type, //0xff でフォルダ、0x80 が ON でテキストレイヤ
		blendmode,
		opacity,
		tone_density,
		flags;
	int16_t tone_lines,
		tone_angle;
	uint32_t col;
	uint16_t name_len, //NULL 含まない
		texture_len;
	char text[1];	//名前 (NULL含む) + テクスチャパス (NULL含む)
}ConfigLayerTemplItem;

enum
{
	CONFIG_LAYERTEMPL_F_TONE = 1<<0,		//トーン化
	CONFIG_LAYERTEMPL_F_TONE_WHITE = 1<<1	//トーン:背景を白に
};

/** グリッド設定 */

typedef struct
{
	uint16_t gridw, gridh,	//グリッドサイズ
		splith, splitv,		//分割数
		pxgrid_zoom;		//1pxグリッドを表示する倍率 (上位1bit:ON/OFF, 下位:1=100%)
	uint32_t col_grid,		//グリッド色:上位8bitが濃度 (100%=128)
		col_split,			//分割線色
		recent[CONFIG_GRID_RECENT_NUM];	//履歴リスト (0でなし。上位16bit:幅, 下位16bit:高さ)
}ConfigGrid;

/** キャンバスビュー設定 */

typedef struct
{
	int zoom;		//表示倍率 (1=0.1%)
	uint8_t flags,	//フラグ
		bttcmd[5];	//各ボタンの操作 ([0]L [1]Ctrl+L [2]Shift+L [3]R [4]M)
}ConfigCanvasView;

enum
{
	CONFIG_CANVASVIEW_F_TOOLBAR_VISIBLE = 1<<0,	//ツールバー表示
	CONFIG_CANVASVIEW_F_TOOLBAR_ALWAYS = 1<<1,	//ツールバーは常に表示
	CONFIG_CANVASVIEW_F_FIT = 1<<2			//全体表示
};

/** イメージビューア設定 */

typedef struct
{
	uint8_t	flags,	//フラグ
		bttcmd[5];	//各ボタンの操作
}ConfigImgViewer;

enum
{
	CONFIG_IMAGEVIEWER_F_FIT = 1<<0,	//全体表示
	CONFIG_IMAGEVIEWER_F_MIRROR = 1<<1	//左右反転表示
};

/** パネル関連の設定 */

typedef struct
{
	char pane_layout[5],	//ペイン配置 ("0123" の文字が配置順に並ぶ。'0'=キャンバス)
		panel_layout[16];	//パネル配置順 (各パネルの1文字が配置順に並ぶ。':' でペイン区切り)

	uint8_t color_type,		//カラー:色タイプ
		colpal_type,		//カラーパレット:タイプ
		colwheel_type,		//カラーホイール:タイプ
		option_type,		//オプション:タイプ
		toollist_sizelist_h;	//ツールリストのブラシサイズリストの高さ
	uint32_t brushopt_flags,	//ブラシ設定:フラグ
		water_preset[CONFIG_WATER_PRESET_NUM];	//水彩プリセット(各10bit)
}ConfigPanel;

enum
{
	CONFIG_PANEL_BRUSHOPT_F_HIDE_WATER = 1<<0,	//水彩
	CONFIG_PANEL_BRUSHOPT_F_HIDE_SHAPE = 1<<1,	//ブラシ形状
	CONFIG_PANEL_BRUSHOPT_F_HIDE_PRESSURE = 1<<2,	//筆圧
	CONFIG_PANEL_BRUSHOPT_F_HIDE_OTHRES = 1<<3	//色々
};

/** 変形ダイアログの設定 */

typedef struct
{
	int view_w,view_h;	//画面のサイズ
	uint8_t flags;
}ConfigTransform;

enum
{
	CONFIG_TRANSFORM_F_KEEP_ASPECT = 1
};

/** 保存設定 */

typedef struct
{
	uint16_t jpeg,
		webp;
	uint8_t png,
		tiff,
		psd;
	mPoint pt_tpcol; //透過色のイメージ位置 (x = -1 でなし,上書き保存時用)
}ConfigSaveOption;


/*-----------------*/


/** AppConfig */

typedef struct _AppConfig
{
	ConfigSaveOption save;		//画像保存設定
	ConfigGrid grid;			//グリッド設定
	ConfigPanel panel;			//パネル関連設定
	ConfigCanvasView canvview;	//キャンバスビュー設定
	ConfigImgViewer imgviewer;	//イメージビューア設定
	ConfigTransform transform;	//変形ダイアログの設定

	ConfigNewCanvas newcanvas_init,		//初期起動時のキャンバス
		newcanvas_recent[CONFIG_NEWCANVAS_NUM],	//新規作成の履歴
		newcanvas_record[CONFIG_NEWCANVAS_NUM];	//新規作成の登録

	mToplevelSaveState winstate_textdlg,	//テキストダイアログのウィンドウ状態
		winstate_filterdlg;					//フィルタダイアログ(キャンバスプレビュー時)
	int textdlg_toph;		//上部の高さ

	int savedup_type,		//複製保存時の保存形式 (0 で現在のファイルと同じ)
		undo_maxbufsize;	//アンドゥ最大バッファサイズ (0 ですべてファイルに出力)

	uint32_t fview,			//表示フラグ
		foption,			//オプションフラグ
		canvasbkcol,		//キャンバス背景色 (イメージ範囲外の色)
		rule_guide_col;		//定規ガイドの色と濃度 (A=128)

	uint16_t canvas_zoom_step_hi,	//キャンバス倍率:100%以上の1段階
		canvas_angle_step,			//キャンバス回転:1段階
		tone_lines_default,			//トーン化レイヤのデフォルト線数 (1=0.1)
		undo_maxnum,			//アンドゥ最大回数
		iconsize_toolbar,		//アイコンサイズ(ツールバー)
		iconsize_panel_tool,	//アイコンサイズ(ツール)
		iconsize_other,			//アイコンサイズ(ほか)
		textsize_recent[CONFIG_TEXTSIZE_RECENT_NUM]; //テキストのフォントサイズの履歴 (0で終了, 上位ビットON=px,OFF=pt)
	int16_t cursor_hotspot[2];	//描画カーソルのホットスポット位置(x,y)

	uint8_t loadimg_default_bits,	//画像読み込み時のデフォルトビット数
		canvas_scale_method,		//キャンバス拡大縮小の補間方法
		pointer_btt_default[CONFIG_POINTERBTT_NUM], //デフォルトデバイスの各ボタンのコマンド (0:消しゴム側, 1:左ボタン, ...)
		pointer_btt_pentab[CONFIG_POINTERBTT_NUM];  //筆圧情報があるデバイスの各ボタンのコマンド

	mStr strFont_panel,			//パネルフォント
		strTempDir,				//作業用ディレクトリ (トップ)
		strTempDirProc,			//作業用ディレクトリ (現在のプロセス用。"<tempdir>/azpainter-<procid>"。空で作成に失敗)
		strFileDlgDir,			//共通のファイルダイアログディレクトリ
		strImageViewerDir,		//イメージビューア開くディレクトリ
		strLayerFileDir,		//レイヤファイルディレクトリ
		strSelectFileDir,		//選択範囲ファイルディレクトリ
		strFontFileDir,			//フォントファイル選択ディレクトリ
		strUserTextureDir,		//ユーザーのテクスチャディレクトリ
		strUserBrushDir,		//ユーザーのブラシ画像ディレクトリ
		strCursorFile,			//描画カーソル画像ファイル
		strRecentFile[CONFIG_RECENTFILE_NUM],	//最近使ったファイル
		strRecentOpenDir[CONFIG_RECENTDIR_NUM],	//開いたディレクトリの履歴
		strRecentSaveDir[CONFIG_RECENTDIR_NUM];	//保存ディレクトリの履歴

	uint8_t *toolbar_btts;		//ツールバーのボタン (NULL でデフォルト)
	uint32_t toolbar_btts_size;	//確保サイズ
	
	uint8_t canvaskey[256];	//キャンバスキー (添字は生のキー番号。値はコマンドID)

	uint8_t fmodify_textword,	//単語リストが変更された
		fmodify_layertempl;		//レイヤテンプレートが更新された
	
	mList list_layertempl,	//レイヤテンプレートのリスト
		list_textword,		//テキスト描画の単語リスト
		list_filterparam;	//フィルタの保存パラメータ値
}AppConfig;


extern AppConfig  *g_app_config;
#define APPCONF   g_app_config


/*---- flags ----*/


/* canvas_resize_flags */
#define CONFIG_CANVAS_RESIZE_F_CROP  1  //範囲外を切り取る

/* foption */
enum
{
	CONFIG_OPTF_MES_SAVE_OVERWRITE = 1<<0,	//上書き保存確認
	CONFIG_OPTF_MES_SAVE_APD = 1<<1,		//上書き時、APD で保存するか確認
	CONFIG_OPTF_SAVE_APD_NOPICT = 1<<2,		//APD 保存時、一枚絵イメージを含めない
	CONFIG_OPTF_FILTERLIST_DBLCLK = 1<<3	//フィルタ一覧はダブルクリックで実行
};

/* fview */
enum
{
	CONFIG_VIEW_F_TOOLBAR = 1<<0,
	CONFIG_VIEW_F_STATUSBAR = 1<<1,
	CONFIG_VIEW_F_PANEL = 1<<2,			//パネル
	CONFIG_VIEW_F_BKGND_PLAID = 1<<3,	//背景を格子模様に
	CONFIG_VIEW_F_RULE_GUIDE = 1<<4,	//定規のガイド
	CONFIG_VIEW_F_GRID = 1<<5,			//グリッド
	CONFIG_VIEW_F_GRID_SPLIT = 1<<6,	//グリッド(分割線)
	CONFIG_VIEW_F_CURSOR_POS = 1<<7,	//ステータスバーにカーソル位置を表示
	CONFIG_VIEW_F_CANV_LAYER_NAME = 1<<8,	//キャンバス操作時、レイヤ名表示
	CONFIG_VIEW_F_FILEDLG_PREVIEW = 1<<9,	//ファイルダイアログ:プレビュー
	CONFIG_VIEW_F_FILTERDLG_PREVIEW = 1<<10,	//フィルタダイアログ:プレビュー
	CONFIG_VIEW_F_BOXSEL_POS = 1<<11	//切り貼り・矩形編集の選択時、座標を表示
};

/* キャンバスビューとイメージビューアのボタン番号 */
enum
{
	CONFIG_PANELVIEW_BTT_LEFT,
	CONFIG_PANELVIEW_BTT_LEFT_CTRL,
	CONFIG_PANELVIEW_BTT_LEFT_SHIFT,
	CONFIG_PANELVIEW_BTT_RIGHT,
	CONFIG_PANELVIEW_BTT_MIDDLE
};

