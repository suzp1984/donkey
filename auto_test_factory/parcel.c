#include "parcel.h"
#include "cmd_common.h"

#include <stdlib.h>
#include <string.h>

#define LOG_TAG "auto_test"
#include <utils/Log.h>


struct _Parcel {
	char* buf;
	uint16_t length;
	uint8_t main_cmd;
	uint8_t sub_cmd;
	char* content;
};

Ret parcel_set_main_cmd(Parcel* thiz, uint8_t main_cmd)
{
	return_val_if_fail(thiz != NULL, RET_INVALID_PARAMS);
	
	thiz->main_cmd = main_cmd;
	*(uint8_t*)(thiz->buf + 7) = main_cmd;

	return RET_OK;
}

Ret parcel_get_main_cmd(Parcel* thiz, uint8_t* main_cmd)
{
	return_val_if_fail(thiz != NULL, RET_INVALID_PARAMS);

	*main_cmd = thiz->main_cmd;
	return RET_OK;
}

Ret parcel_set_sub_cmd(Parcel* thiz, uint8_t sub_cmd)
{
	return_val_if_fail(thiz != NULL, RET_INVALID_PARAMS);

	thiz->sub_cmd = sub_cmd;
	*(uint8_t*)(thiz->buf + 8) = sub_cmd;
	return RET_OK;
}

Ret parcel_get_sub_cmd(Parcel* thiz, uint8_t* sub_cmd)
{
	return_val_if_fail(thiz != NULL, RET_INVALID_PARAMS);
	
	*sub_cmd = thiz->sub_cmd;
	return RET_OK;
}

Ret parcel_set_content(Parcel* thiz, char* content, size_t len)
{
	return_val_if_fail(thiz != NULL, RET_INVALID_PARAMS);

	SAFE_FREE(thiz->content);
	thiz->content = (char*) malloc(len);

	if (len != 0) {
		memcpy(thiz->content, content, len);
		thiz->length = len + 8;
		thiz->buf = (char*)realloc(thiz->buf, thiz->length + 2);
		memcpy((thiz->buf + 9), content, len);
		thiz->buf[thiz->length + 1] = (char)PACKAGE_END;
		*(uint16_t*)(thiz->buf + 5) = len + 8;
	} else {
		thiz->length = 8;
	}

	return RET_OK;
}

size_t parcel_get_content(Parcel* thiz, char** content)
{
	return_val_if_fail(thiz != NULL, RET_INVALID_PARAMS);

	*content = thiz->content;
	return (size_t)thiz->length - 8;
}

size_t parcel_get_buf(Parcel* thiz, char** buf)
{
	return_val_if_fail(thiz != NULL, RET_INVALID_PARAMS);

	*buf = thiz->buf;

	return (size_t)(thiz->length + 2);
}

Ret parcel_set_buf(Parcel* thiz, char* buf, size_t len)
{
	return_val_if_fail(thiz != NULL && buf != NULL && len >= 10, RET_INVALID_PARAMS);
	
	char* ptr = buf;
	char* package_begin;
	char* package_end;
	int m = 0;

	while((uint8_t)*ptr != PACKAGE_HEAD && m != (int)len) {
		m++;
		ptr++;
	}

	if ((uint8_t)*ptr != PACKAGE_HEAD) {
		LOGE("%s: buf error\n", __func__);
		return RET_FAIL;
	}

	if (*(uint16_t*)(ptr + 5) + 2 + m > (int)len) {
		LOGE("%s: buf length error\n", __func__);
		return RET_FAIL;
	}

	package_begin = ptr;
	//printf("length is 0x%x\n", *(uint16_t*)(ptr + 5));
	package_end = ptr + *(uint16_t*)(ptr + 5) + 1;

	if ((uint8_t)*package_end != PACKAGE_END) {
		LOGE("package end is 0x%x\n", (uint8_t)*package_end);
		return RET_FAIL;
	}

	memcpy(&(thiz->length), &(ptr[5]), sizeof(uint16_t));
	memcpy(&(thiz->main_cmd), &(ptr[7]), sizeof(uint8_t));
	memcpy(&(thiz->sub_cmd), &(ptr[8]), sizeof(uint8_t));

	thiz->content = (char*)malloc(sizeof(char)*(thiz->length - 8));
	memcpy(thiz->content, (void*)&(ptr[9]), thiz->length - 8);

	SAFE_FREE(thiz->buf);
	thiz->buf = (char*) malloc(thiz->length + 2);
	memcpy(thiz->buf, package_begin, (thiz->length + 2));

	return RET_OK;
}

uint16_t parcel_get_length(Parcel* thiz) 
{
	return_val_if_fail(thiz != NULL, 0);

	return thiz->length;
}

void parcel_destroy(Parcel* thiz)
{
	return_if_fail(thiz != NULL);

	SAFE_FREE(thiz->content);
	SAFE_FREE(thiz->buf);

	SAFE_FREE(thiz);
}

Parcel* parcel_create()
{
	Parcel* thiz = (Parcel*) malloc(sizeof(Parcel));

	if (thiz != NULL) {
		thiz->buf = (char*)malloc(10);
		memset(thiz->buf, 0, 10);
		thiz->buf[0] = (char)PACKAGE_HEAD;
		thiz->buf[9] = (char)PACKAGE_END;
		*(uint16_t*)(thiz->buf + 5) = 8;

		thiz->length = 8;
		thiz->main_cmd = 0;
		thiz->sub_cmd = 0;
		thiz->content = NULL;
	}

	return thiz;
}

#ifdef PARCEL_TEST

#include <assert.h>
#include <string.h>

int main(int argc, char* argv[])
{
	char buf_sent[10]={PACKAGE_HEAD,0x00,0x00,0x00,0x00,0x08,0x00,DIAG_BOOT_MAIN_CMD,DIAG_BOOT_SUB_CMD_1,PACKAGE_END};
	char err1_buf[10]={0x01,0x00,0x00,0x00,0x00,0x18,0x00,DIAG_BOOT_MAIN_CMD,DIAG_BOOT_SUB_CMD_1,PACKAGE_END};
	char err2_buf[12]={PACKAGE_HEAD,0x00,0x00,0x00,0x00,0x08,0x00,DIAG_BOOT_MAIN_CMD,DIAG_BOOT_SUB_CMD_1,0x01,0x02,PACKAGE_END};
	char err3_buf[12]={PACKAGE_HEAD,0x00,0x00,0x00,0x00,0x08,0x00,DIAG_BOOT_MAIN_CMD,DIAG_BOOT_SUB_CMD_1,0x01,0x02,0x00};
	uint8_t main_cmd;
	uint8_t sub_cmd;

	char* buf = NULL;
	int i = 0;
	/*
	printf("buf_sent len is %d\n", strlen(buf_sent));
	printf("buf_sent sizeof is %d\n", sizeof(buf_sent));
	return 0; */

	Parcel* pack = parcel_create();
	assert(pack != NULL);
	assert(parcel_set_buf(pack, buf_sent, 10) == RET_OK);
	assert(parcel_get_main_cmd(pack, &main_cmd) == RET_OK);
	assert(main_cmd == DIAG_BOOT_MAIN_CMD);
	assert(parcel_get_sub_cmd(pack, &sub_cmd) == RET_OK);
	assert(sub_cmd == DIAG_BOOT_SUB_CMD_1);
	assert(parcel_get_length(pack) == 0x0008);
	assert(parcel_get_buf(pack, &buf) == 10);
	for (i = 0; i < 10; i++) {
		printf("buf[%d] = 0x%x\n", i, (uint8_t)buf[i]);
	}

	parcel_destroy(pack);

	// test set cmd
	printf("********** test err package ********\n");
	Parcel* err_pack = parcel_create();
	assert(err_pack != NULL);
	assert(parcel_set_buf(err_pack, err1_buf, 10) == RET_FAIL);
	assert(parcel_set_buf(err_pack, err2_buf, 12) == RET_FAIL);
	assert(parcel_set_buf(err_pack, err3_buf, 12) == RET_FAIL);

	parcel_destroy(err_pack);

	printf("********* test empty Parcel*******\n");
	Parcel* empty_pack = parcel_create();
	assert(parcel_get_buf(empty_pack, &buf) == 10);
	assert((uint8_t)buf[0] == PACKAGE_HEAD);
	assert((uint8_t)buf[9] == PACKAGE_END);
	assert(*(uint16_t*)(buf + 5) == 0x08);
	for (i = 0; i < 10; i++) {
		printf("buf[%d] = 0x%x\n", i, (uint8_t)buf[i]);
	}

	char cont_test[5] = {0x11,0x12,0x13,0x14,0x15};
	char* get_cont = NULL;
	assert(parcel_set_content(empty_pack, cont_test, 5) == RET_OK);
	assert(parcel_get_length(empty_pack) == 13);
	assert(parcel_get_content(empty_pack, &get_cont) == 5);
	for (i = 0; i < 5; i++) {
		printf("content[%d] = 0x%x\n", i, (uint8_t)get_cont[i]);
	}

	uint8_t cmd = 0;
	assert(parcel_set_main_cmd(empty_pack, 0x1a) == RET_OK);
	assert(parcel_get_main_cmd(empty_pack, &cmd) == RET_OK);
	assert(cmd == 0x1a);

	assert(parcel_set_sub_cmd(empty_pack, 0x2b) == RET_OK);
	assert(parcel_get_sub_cmd(empty_pack, &cmd) == RET_OK);
	assert(cmd == 0x2b);
	
	int length = parcel_get_length(empty_pack);
	assert(parcel_get_buf(empty_pack, &buf) == length + 2);
	for (i = 0; i < length + 2; i++) {
		printf("buf[%d] = 0x%x\n", i, (uint8_t)buf[i]);
	}

	parcel_destroy(empty_pack);
	return 0;
}

#endif  // PARCEL_TEST
