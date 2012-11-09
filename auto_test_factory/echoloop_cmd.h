#ifndef ECHOLOOP_CMD_H
#define ECHOLOOP_CMD_H

#include "cmd_interface.h"
#include "cmd_listener.h"

DECLES_BEGIN

CmdInterface* echoloop_cmd_create(CmdListener* listener);

DECLES_END

#endif // ECHOLOOP_CMD_H
