#ifndef CMD_LISTENER_H
#define CMD_LISTENER_H

#include "parcel.h"

DECLES_BEGIN

struct _CmdListener;
typedef struct _CmdListener CmdListener;

CmdListener* cmd_listener_create(void);
void cmd_listener_start(CmdListener* thiz);
void cmd_listener_quit(CmdListener* thiz);
void cmd_listener_reply(CmdListener* thiz, Parcel* parcel);
void cmd_listener_send_trace(CmdListener* thiz, char* trace);

void cmd_listener_destroy(CmdListener* thiz);

DECLES_END

#endif // CMD_LISTENER_H
