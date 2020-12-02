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

#ifndef MLIB_X11_UTIL_H
#define MLIB_X11_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

Atom mX11GetAtom(const char *name);

void mX11SetProperty32(Window id,const char *name,Atom proptype,void *val,int num);
void mX11SetPropertyCARDINAL(Window id,Atom prop,void *val,int num);
void mX11SetPropertyAtom(Window id,Atom prop,Atom *atoms,int num);
void mX11SetProperty8(Window id,Atom prop,Atom type,
	const void *buf,long size,mBool append);
void mX11SetPropertyCompoundText(Window id,Atom prop,const char *utf8,int len);

void mX11SetProperty_wm_pid(Window id);
void mX11SetProperty_wm_client_leader(Window id);

mBool mX11GetProperty32Array(Window id,Atom prop,Atom proptype,void *buf,int num);
void *mX11GetProperty32(Window id,Atom prop,Atom proptype,int *resnum);
void *mX11GetProperty8(Window id,Atom prop,mBool append_null,uint32_t *ret_size);

void mX11SetEventClientMessage(XEvent *ev,Window win,Atom mestype);
void mX11SendClientMessageToRoot(Window id,Atom mestype,void *data,int num);
void mX11SendClientMessageToRoot_string(const char *mestype,const char *mestype_begin,const char *sendstr);
void mX11SendStartupNotify_complete();
int mX11GetEventTimeout(int evtype,int timeoutms,XEvent *ev);

void mX11Send_NET_WM_STATE(Window id,int action,Atom data1,Atom data2);

mBool mX11SelectionConvert(Window id,Atom selection,Atom target,XEvent *ev);
Atom mX11GetSelectionTargetType(Window id,
	Atom atom_selection,Atom *target_list,int target_num);
void *mX11GetSelectionDat(Window id,Atom selection,Atom target,mBool append_null,uint32_t *ret_size);
char *mX11GetSelectionCompoundText(Window id,Atom selection);

Window mX11GetFrameWindow(Window id);

#ifdef __cplusplus
}
#endif

#endif
