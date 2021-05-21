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

#ifndef MLK_OPENTYPE_H
#define MLK_OPENTYPE_H

typedef struct _mOTLayout mOTLayout;

#define MOPENTYPE_FEATURE_VERT  MLK_MAKE32_4('v','e','r','t')
#define MOPENTYPE_FEATURE_VRT2  MLK_MAKE32_4('v','r','t','2')
#define MOPENTYPE_FEATURE_RUBY  MLK_MAKE32_4('r','u','b','y')

typedef struct
{
	uint8_t *buf;
	int num;
	uint32_t offset,tag;
	uint16_t value;
}mOT_TABLE;

typedef struct _mOT_VORG
{
	uint8_t *buf;
	int16_t default_origin;
	uint8_t have_vorg;
}mOT_VORG;

typedef struct
{
	int16_t x,y,advx,advy;
}mOT_POS;

typedef struct
{
	mBufIO *io;
	int lookup_type;
	uint32_t offset_subst;
}mOT_LOOKUP_SUBTABLE;

typedef mlkerr (*mFuncOTLayoutLookupSubtable)(mOTLayout *p,mOT_LOOKUP_SUBTABLE *subst,void *param);


#ifdef __cplusplus
extern "C" {
#endif

void *mOpenType_searchData(uint16_t gid,uint8_t *buf,int datasize);

mlkerr mOpenType_readVORG(void *buf,uint32_t size,mOT_VORG *dst);
mlkerr mOpenType_readGSUB_single(void *buf,uint32_t size,uint32_t script_tag,uint32_t lang_tag,uint32_t feature_tag,uint8_t **ppdst);
mlkerr mOpenType_readGPOS_single(void *buf,uint32_t size,uint32_t script_tag,uint32_t lang_tag,uint32_t feature_tag,uint8_t **ppdst);

mlkbool mOpenType_getVORG_originY(mOT_VORG *p,uint16_t gid,int *origin);
uint16_t mOpenType_getGSUB_replaceSingle(uint8_t *buf,uint16_t gid);
mlkbool mOpenType_getGPOS_single(uint8_t *buf,uint16_t gid,mOT_POS *pos);

uint8_t *mOpenType_combineData_GSUB_single(uint8_t *srcbuf,uint8_t *addbuf);

/* mOTLayout */

mlkerr mOTLayout_new(mOTLayout **dst,void *buf,uint32_t size,mlkbool is_gpos);
void mOTLayout_free(mOTLayout *p);

mlkerr mOTLayout_getScriptList(mOTLayout *p,mOT_TABLE *dst);
uint32_t mOTLayout_readScriptLangTag(const mOT_TABLE *tbl,int index);
mlkerr mOTLayout_searchScriptList(mOTLayout *p,const uint32_t *tags,uint32_t *dsttag);
mlkerr mOTLayout_getScript(mOTLayout *p,uint32_t script_tag,mOT_TABLE *dst);
mlkerr mOTLayout_getLang(mOTLayout *p,uint32_t lang_tag,const mOT_TABLE *script,mOT_TABLE *dst);
mlkerr mOTLayout_getFeature(mOTLayout *p,uint32_t feature_tag,const mOT_TABLE *langsys,mOT_TABLE *dst);
mlkerr mOTLayout_getFeature2(mOTLayout *p,uint32_t script_tag,uint32_t lang_tag,uint32_t feature_tag,mOT_TABLE *dst);
mlkerr mOTLayout_getLookup(mOTLayout *p,const mOT_TABLE *feature,mFuncOTLayoutLookupSubtable func,void *param);

mlkerr mOTLayout_createGSUB_single(mOTLayout *p,const mOT_TABLE *feature,uint8_t **ppdst);
mlkerr mOTLayout_createGPOS_single(mOTLayout *p,const mOT_TABLE *feature,uint8_t **ppdst);

#ifdef __cplusplus
}
#endif

#endif
