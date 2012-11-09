#ifndef RECEIVE_CMD_H
#define RECEIVE_CMD_H

#include "cmd_interface.h"
#include "cmd_listener.h"

DECLES_BEGIN

CmdInterface* receive_cmd_create(CmdListener* listener);

DECLES_END

#endif // RECEIVE_CMD_H
