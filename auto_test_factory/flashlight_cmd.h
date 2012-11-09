#ifndef FLASHLIGHT_CMD_H
#define FLASHLIGHT_CMD_H

#include "cmd_interface.h"
#include "cmd_listener.h"

DECLES_BEGIN

CmdInterface* flashlight_cmd_create(CmdListener* listener);

DECLES_END

#endif // FLASHLIGHT_CMD_H
