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
 * GUI 内部定義
 *****************************************/

#ifndef MLK_PV_GUI_H
#define MLK_PV_GUI_H

typedef struct _mAppRunData  mAppRunData;
typedef struct _mTranslation mTranslation;

/** ループデータ */

struct _mAppRunData
{
	mAppRunData *run_back;
	mWindow *window;		//modal/popup のウィンドウ
	mWidget *widget_sub;
		//ポップアップ時、ポップアップウィンドウ以外でイベントを送るウィジェット。
		//mMenuBar から表示した場合は、mMenuBar 上のポインタイベントが必要となるため。
		//POINTER#MOTION, ENTER, LEAVE イベントが対象。
	uint8_t type,
		loop;	//0 でループ終了
};

enum
{
	MAPPRUN_TYPE_NORMAL,
	MAPPRUN_TYPE_MODAL,
	MAPPRUN_TYPE_POPUP
};

/** スタイル設定データ */

typedef struct
{
	char *fontstr,	//フォント (文字列フォーマット)
		*icontheme;	//アイコンテーマ名
}mAppStyleData;

/** ファイルダイアログ設定 */

typedef struct
{
	uint16_t width,
		height;
	uint8_t sort_type,
		flags;
}mAppFileDialogConf;

/** mAppBackend */

typedef struct _mAppBackend
{
	void *(*alloc_data)(void);
	void (*close)(void);
	mlkbool (*init)(void);
	int (*mainloop)(mAppRunData *,mlkbool);
	void (*run_event)(void);
	void (*thread_lock)(mlkbool lock);
	void (*thread_wakeup)(void);

	mPixCol (*rgbtopix)(uint8_t,uint8_t,uint8_t);
	mRgbCol (*pixtorgb)(mPixCol);
	uint32_t (*keycode_to_code)(uint32_t code);
	mlkbool (*keycode_getname)(mStr *str,uint32_t code);

	mlkbool (*clipboard_release)(void);
	mlkbool (*clipboard_setdata)(int type,const void *buf,uint32_t size,
		const char *mimetypes,mFuncEmpty handle);
	mlkbool (*clipboard_send)(const void *buf,int size);
	int (*clipboard_gettext)(mStr *str);
	int (*clipboard_getdata)(const char *mimetype,mFuncEmpty handle,void *param);
	char **(*clipboard_getmimetypelist)(void);
	char *(*clipboard_findmimetype)(const char *);

	void (*cursor_free)(mCursor);
	mCursor (*cursor_load)(const char *);
	mCursor (*cursor_create1bit)(const uint8_t *);
	mCursor (*cursor_createRGBA)(int width,int height,int hotspot_x,int hotspot_y,const uint8_t *buf);

	void (*pixbuf_setbufcol)(uint8_t *,mPixCol);
	void (*pixbuf_setbufxor)(uint8_t *,mPixCol);
	mPixCol (*pixbuf_getbuf)(uint8_t *);
	mPixbuf *(*pixbuf_alloc)(void);
	void (*pixbuf_deleteimg)(mPixbuf *pixbuf);
	mlkbool (*pixbuf_create)(mPixbuf *pixbuf,int w,int h);
	mlkbool (*pixbuf_resize)(mPixbuf *pixbuf,int w,int h,int neww,int newh);
	void (*pixbuf_render)(mWindow *win,int x,int y,int w,int h);

	mlkbool (*window_alloc)(void **);
	void (*window_destroy)(mWindow *p);
	void (*window_show)(mWindow *p,int f);
	void (*window_resize)(mWindow *p,int w,int h);
	void (*window_setcursor)(mWindow *p,mCursor cur);

	mlkbool (*toplevel_create)(mToplevel *p);
	void (*toplevel_settitle)(mToplevel *p,const char *title);
	void (*toplevel_seticon32)(mToplevel *p,mImageBuf *img);
	void (*toplevel_maximize)(mToplevel *p,int type);
	void (*toplevel_fullscreen)(mToplevel *p,int type);
	void (*toplevel_minimize)(mToplevel *p);
	void (*toplevel_start_drag_move)(mToplevel *p);
	void (*toplevel_start_drag_resize)(mToplevel *p,int type);
	void (*toplevel_get_save_state)(mToplevel *p,mToplevelSaveState *st);
	void (*toplevel_set_save_state)(mToplevel *p,mToplevelSaveState *st);

	mlkbool (*popup_show)(mPopup *p,int x,int y,int w,int h,mBox *box,uint32_t flags);
}mAppBackend;

/** mAppBase */

typedef struct _mAppBase
{
	void *bkend_data;		//バックエンドデータ
	mAppBackend bkend;		//バックエンド関数

	mAppStyleData style;	//スタイルデータ
	mAppFileDialogConf filedlgconf;	//ファイルダイアログ設定

	mList list_event,		//イベントリスト
		list_timer,			//タイマーリスト
		list_cursor_cache;	//カーソルキャッシュ

	mAppRunData *run_current;	//現在のrunデータ
	int run_level;				//run ネスト数

	mTranslation *trans_def,	//翻訳:埋め込み
		*trans_file;			//翻訳:ファイルから読込
	int trans_cur_group,		//現在のグループID (-1 でなし)
		trans_save_group;		//一時保存のグループID

	char *app_wmname,		//ウィンドウマネージャで使われる名前 (アプリ側での指定)
		*app_wmclass;		//ウィンドウマネージャで使われるクラス名

	//>> コマンドラインオプションで指定された値 (ロケール文字列)
	char *opt_arg0,
		*opt_trfile,
		*opt_dispname,
		*opt_wmname,
		*opt_wmclass;

	mFontSystem *fontsys;
	mFont *font_default;	//デフォルトフォント

	char *path_config,		//設定ファイル用のパス
		*path_data;			//データファイル用のパス

	mWidget *widget_root,		//ルートウィジェット
		*widget_grabpt,			//現在グラブ中のウィジェット (ポインタ)
		*widget_enter,			//現在 Enter 状態のウィジェット (グラブ中も変更する)
		*widget_enter_last,		//グラブ開始時の Enter 状態のウィジェット
		*widget_construct_top;
	mWindow *window_focus,		//現在フォーカスがあるウィンドウ
		*window_enter;			//一番最後に Enter イベントが来たウィンドウ

	int32_t pointer_last_win_fx,	//Enter & Motion イベントの一番最後の位置
		pointer_last_win_fy;		//(装飾含含むウィンドウ座標、24:8 固定少数)

	uint32_t flags;
}mAppBase;

extern mAppBase *g_mlk_app;
#define MLKAPP  g_mlk_app


enum MAPPBASE_FLAGS
{
	MAPPBASE_FLAGS_ENABLE_PENTABLET  = 1<<0,	//ペンタブ有効
	MAPPBASE_FLAGS_BLOCK_USER_ACTION = 1<<1,	//ユーザーアクションをブロック
	MAPPBASE_FLAGS_IN_WIDGET_DESTROY = 1<<2,	//mWidgetDestroy() 中
	MAPPBASE_FLAGS_IN_WIDGET_LAYOUT  = 1<<3,	//mWidgetLayout() 中
	MAPPBASE_FLAGS_WINDOW_ENTER_DECO = 1<<4,	//ウィンドウ装飾範囲内が Enter 状態
	MAPPBASE_FLAGS_DEBUG_MODE = 1<<5	//デバッグ時用 (グラブ無効)
};

enum MAPPBASE_FILEDIALOGCONF_FLAGS
{
	MAPPBASE_FILEDIALOGCONF_F_SORT_REV = 1<<0,		//ソート逆順
	MAPPBASE_FILEDIALOGCONF_F_SHOW_HIDDEN_FILES = 1<<1	//隠しファイル表示
};


/*------------------*/

/* mlk_gui.c */

void __mAppRunConstruct(void);
void __mAppDelConstructWidget(mWidget *wg);
mlkbool __mAppAfterEvent(void);
void __mAppPutEventDebug(mEvent *ev);

/* mlk_gui_style.c */

void __mGuiStyleFree(mAppStyleData *p);
void __mGuiStyleLoad(mAppBase *p);

/* mlk_eventlist.c */

void mEventListInit(void);
void mEventListEmpty(void);
void mEventFreeItem(void *item);

mEvent *mEventListAdd(mWidget *wg,int type,int size);
void mEventListAdd_base(mWidget *widget,int type);
void mEventListAdd_focus(mWidget *widget,mlkbool is_out,int from);

mEvent *mEventListGetEvent(void **itemptr);
void mEventListDelete_widget(mWidget *widget);
void mEventListCombineConfigure(void);

/* mlk_guitimer.c */

int mGuiTimerGetWaitTime_ms(void);
void mGuiTimerProc(void);

mlkbool mGuiTimerAdd(mWidget *wg,int id,uint32_t msec,intptr_t param);
mlkbool mGuiTimerAdd_app(int type,uint32_t msec,intptr_t param);

void mGuiTimerDeleteAll(void);
mlkbool mGuiTimerDelete(mWidget *wg,int id);
void mGuiTimerDelete_widget(mWidget *wg);
void mGuiTimerDelete_app(int type);

mlkbool mGuiTimerFind(mWidget *wg,int id);

#endif
