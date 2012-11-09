#ifndef PARCEL_H
#define PARCEL_H

#include "typedef.h"
#include <stdint.h>

DECLES_BEGIN

struct _Parcel;
typedef struct _Parcel Parcel;

Parcel* parcel_create();

Ret parcel_set_main_cmd(Parcel* thiz, uint8_t main_cmd);
Ret parcel_get_main_cmd(Parcel* thiz, uint8_t* main_cmd);
Ret parcel_set_sub_cmd(Parcel* thiz, uint8_t sub_cmd);
Ret parcel_get_sub_cmd(Parcel* thiz, uint8_t* sub_cmd);

Ret parcel_set_content(Parcel* thiz, char* content, size_t len);
size_t parcel_get_content(Parcel* thiz, char** content);

size_t parcel_get_buf(Parcel* thiz, char** buf);
Ret parcel_set_buf(Parcel* thiz, char* buf, size_t len);

uint16_t parcel_get_length(Parcel* thiz);

void parcel_destroy(Parcel* thiz);

DECLES_END

#endif // PARCEL_H
