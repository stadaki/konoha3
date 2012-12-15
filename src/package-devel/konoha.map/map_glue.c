/****************************************************************************
 * Copyright (c) 2012, the Konoha project authors. All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ***************************************************************************/

#include <minikonoha/minikonoha.h>
#include <minikonoha/sugar.h>
#include <minikonoha/klib.h>

#ifdef __cplusplus
extern "C"{
#endif

//typedef struct kMapVar kMap;
#define kMap struct kMapVar

struct kMapVar {
	kObjectHeader h;
	KHashMap *map;
};

#define Map_isUnboxData(o)    (KFlag_Is(uintptr_t,(o)->h.magicflag,kObjectFlag_Local1))
#define Map_setUnboxData(o,b) KFlag_Set(uintptr_t,(o)->h.magicflag,kObjectFlag_Local1,b)

static void kMap_Init(KonohaContext *kctx, kObject *o, void *conf)
{
	kMap *map = (kMap *)o;
	map->map = KLIB KHashMap_Init(kctx, 17);
	if(KType_Is(UnboxType, kObject_p0(map))) {
		Map_setUnboxData(map, true);
	}
}

static void MapUnboxEntry_Reftrace(KonohaContext *kctx, KHashMapEntry *p, void *thunk)
{
	KObjectVisitor *visitor = (KObjectVisitor *) thunk;
	KRefTrace(p->StringKey);
}

static void MapObjectEntry_Reftrace(KonohaContext *kctx, KHashMapEntry *p, void *thunk)
{
	KObjectVisitor *visitor = (KObjectVisitor *) thunk;
	KRefTraceNullable(p->StringKey);
	KRefTrace(p->ObjectValue);
}

static void kMap_Reftrace(KonohaContext *kctx, kObject *o, KObjectVisitor *visitor)
{
	kMap *map = (kMap *)o;
	if(KType_Is(UnboxType, kObject_p0(map))) {
		KLIB KHashMap_DoEach(kctx, map->map, (void *)visitor, MapUnboxEntry_Reftrace);
	}
	else {
		KLIB KHashMap_DoEach(kctx, map->map, (void *)visitor, MapObjectEntry_Reftrace);
	}
}

static void kMap_Free(KonohaContext *kctx, kObject *o)
{
	kMap *map = (kMap *)o;
	if(map->map != NULL) {
		KLIB KHashMap_Free(kctx, map->map, NULL);
	}
}

//static void kMap_p(KonohaContext *kctx, KonohaStack *sfp, int pos, KBuffer *wb)
//{
//	// TODO
//}

/* ------------------------------------------------------------------------ */
/* method */

static uintptr_t String_hashCode(KonohaContext *kctx, kString *s)
{
	return strhash(kString_text(s), kString_size(s));  // FIXME: slow

}

static KHashMapEntry* kMap_getEntry(KonohaContext *kctx, kMap *m, kString *key, kbool_t isNewIfNULL)
{
	uintptr_t hcode = String_hashCode(kctx, key);
	const char *tkey = kString_text(key);
	size_t tlen = kString_size(key);
	KHashMapEntry *e = KLIB KHashMap_get(kctx, m->map, hcode);
	while(e != NULL) {
		if(e->hcode == hcode && tlen == kString_size(e->StringKey) && strncmp(kString_text(e->StringKey), tkey, tlen) == 0) {
			return e;
		}
		e = e->next;
	}
	if(isNewIfNULL) {
		e = KLIB KHashMap_newEntry(kctx, m->map, hcode);
		KUnsafeFieldInit(e->StringKey, key);
		if(!Map_isUnboxData(m)) {
			KUnsafeFieldInit(e->ObjectValue, K_NULL);
		}
		return e;
	}
	return NULL;
}

//## method Boolean Map.has(T1 key);
static KMETHOD Map_has(KonohaContext *kctx, KonohaStack *sfp)
{
	kMap *m = (kMap *)sfp[0].asObject;
	KHashMapEntry *e = kMap_getEntry(kctx, m, sfp[1].asString, false/*new_if_NULL*/);
	KReturnUnboxValue((e != NULL));
}

//## T0 Map.get(String key);
static KMETHOD Map_get(KonohaContext *kctx, KonohaStack *sfp)
{
	kMap *m = (kMap *)sfp[0].asObject;
	KHashMapEntry *e = kMap_getEntry(kctx, m, sfp[1].asString, false/*new_if_NULL*/);
	if(Map_isUnboxData(m)) {
		uintptr_t u = (e == NULL) ? 0 : e->unboxValue;
		KReturnUnboxValue(u);
	}
	else if(e != NULL) {
		KReturn(e->ObjectValue);
	}
	KReturnDefaultValue();
}

//## method void Map.set(String key, T0 value);
static KMETHOD Map_set(KonohaContext *kctx, KonohaStack *sfp)
{
	kMap *m = (kMap *)sfp[0].asObject;
	KHashMapEntry *e = kMap_getEntry(kctx, m, sfp[1].asString, true/*new_if_NULL*/);
	if(Map_isUnboxData(m)) {
		e->unboxValue = sfp[2].unboxValue;
	}
	else {
		KFieldSet(m, e->ObjectValue, sfp[2].asObject);
	}
	KReturnVoid();
}

//## method void Map.remove(String key);
static KMETHOD Map_Remove(KonohaContext *kctx, KonohaStack *sfp)
{
	kMap *m = (kMap *)sfp[0].asObject;
	KHashMapEntry *e = kMap_getEntry(kctx, m, sfp[1].asString, false/*new_if_NULL*/);
	if(e != NULL) {
		KLIB KHashMap_Remove(m->map, e);
	}
	KReturnVoid();
}

static void MapEntry_appendKey(KonohaContext *kctx, KHashMapEntry *p, void *thunk)
{
	kArray *a = (kArray *)thunk;
	KLIB kArray_Add(kctx, a, p->StringKey);
}

//## T0[] Map.keys();
static KMETHOD Map_keys(KonohaContext *kctx, KonohaStack *sfp)
{
	INIT_GCSTACK();
	kMap *m = (kMap *)sfp[0].asObject;
	KClass *cArray = KClass_p0(kctx, KClass_Array, kObject_p0(m));
	kArray *a = (kArray *)(KLIB new_kObject(kctx, _GcStack, cArray, m->map->size));
	KLIB KHashMap_DoEach(kctx, m->map, (void *)a, MapEntry_appendKey);
	KReturnWith(a, RESET_GCSTACK());
}

//## Map<T> Map<T>.new();
static KMETHOD Map_new(KonohaContext *kctx, KonohaStack *sfp)
{
	KReturn(sfp[0].asObject);
}

/* ------------------------------------------------------------------------ */

#define _Public   kMethod_Public
#define _Const    kMethod_Const
#define _Im       kMethod_Immutable
#define _F(F)     (intptr_t)(F)

#define KType_Map cMap->typeId

static kbool_t map_defineMethod(KonohaContext *kctx, kNameSpace *ns, KTraceInfo *trace)
{
	kparamtype_t cparam = {KType_Object};
	KDEFINE_CLASS defMap = {0};
	SETSTRUCTNAME(defMap, Map);
	defMap.cflag     = KClassFlag_Final;
	defMap.cparamsize = 1;
	defMap.cParamItems = &cparam;
	defMap.init      = kMap_Init;
	defMap.reftrace  = kMap_Reftrace;
	defMap.free      = kMap_Free;

	KClass *cMap = KLIB kNameSpace_DefineClass(kctx, ns, NULL, &defMap, trace);
	int FN_key = KKMethodName_("key");
	int KType_Array0 = KClass_p0(kctx, KClass_Array, KType_0)->typeId;
	KDEFINE_METHOD MethodData[] = {
		_Public, _F(Map_new), KType_Map, KType_Map, KKMethodName_("new"), 0,
		_Public|_Im|_Const, _F(Map_has), KType_boolean, KType_Map, KKMethodName_("has"), 1, KType_String, FN_key,
		_Public|_Im|_Const, _F(Map_get), KType_0, KType_Map, KKMethodName_("get"), 1, KType_String, FN_key,
		_Public, _F(Map_set), KType_void, KType_Map, KKMethodName_("set"), 2, KType_String, FN_key, KType_0, KFieldName_("value"),
		_Public, _F(Map_Remove), KType_void, KType_Map, KKMethodName_("remove"), 1, KType_String, FN_key,
		_Public|_Im|_Const, _F(Map_keys), KType_Array0, KType_Map, KKMethodName_("keys"), 0,
		DEND,
	};
	KLIB kNameSpace_LoadMethodData(kctx, ns, MethodData, trace);
	return true;
}

/* ----------------------------------------------------------------------- */

static KMETHOD TypeCheck_MapLiteral(KonohaContext *kctx, KonohaStack *sfp)
{
	VAR_TypeCheck(stmt, expr, gma, reqty);
	kToken *termToken = expr->termToken;
	if(kExpr_IsTerm(expr) && IS_Token(termToken)) {
		DBG_P("termToken='%s'", kString_text(termToken->text));

	}
}

static kbool_t map_defineSyntax(KonohaContext *kctx, kNameSpace *ns, KTraceInfo *trace)
{
	SUGAR kNameSpace_AddSugarFunc(kctx, ns, KSymbol_BlockPattern, SugarFunc_TypeCheck, new_SugarFunc(ns, TypeCheck_MapLiteral));
	return true;
}

static kbool_t map_PackupNameSpace(KonohaContext *kctx, kNameSpace *ns, int option, KTraceInfo *trace)
{
	map_defineMethod(kctx, ns, trace);
	map_defineSyntax(kctx, ns, trace);
	return true;
}

static kbool_t map_ExportNameSpace(KonohaContext *kctx, kNameSpace *ns, kNameSpace *exportNS, int option, KTraceInfo *trace)
{
	return true;
}

KDEFINE_PACKAGE* map_Init(void)
{
	static KDEFINE_PACKAGE d = {0};
	KSetPackageName(d, "map", "1.0");
	d.PackupNameSpace    = map_PackupNameSpace;
	d.ExportNameSpace   = map_ExportNameSpace;
	return &d;
}

#ifdef __cplusplus
}
#endif
