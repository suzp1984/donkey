#include "typedef.h"

#ifndef HWINFO_BUILD_H
#define HWINFO_BUILD_H

DECLES_BEGIN

struct _HwinfoBuilder;
typedef struct _HwinfoBuilder HwinfoBuilder;

typedef Ret (*HwinfoBuilderGetProduct)(HwinfoBuilder* thiz, void** product, void* ctx);
typedef void (*HwinfoBuilderDestroy)(HwinfoBuilder* thiz);

struct _HwinfoBuilder {
	HwinfoBuilderGetProduct get_product;
	HwinfoBuilderDestroy destroy;

	char priv[1];
};

static inline Ret hwinfo_build_get_product(HwinfoBuilder* thiz, void** product, void* ctx) {
	return_val_if_fail(thiz != NULL && thiz->get_product != NULL, RET_INVALID_PARAMS);

	return thiz->get_product(thiz, product, ctx);
}

static inline void hwinfo_build_destroy(HwinfoBuilder* thiz) {
	return_if_fail(thiz != NULL && thiz->destroy != NULL);

	thiz->destroy(thiz);
	return;
}

DECLES_END
#endif //HWINFO_BUILD_H
