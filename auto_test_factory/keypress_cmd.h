#ifndef KEYPRESS_CMD_H
#define KEYPRESS_CMD_H

#include "cmd_interface.h"
#include "cmd_listener.h"

DECLES_BEGIN

CmdInterface* keypress_cmd_create(CmdListener* listener);

DECLES_END

#endif // KEYPRESS_CMD_H
