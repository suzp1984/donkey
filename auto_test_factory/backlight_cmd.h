#ifndef BACKLIGHT_CMD_H
#define BACKLIGHT_CMD_H

#include "cmd_interface.h"
#include "cmd_listener.h"

DECLES_BEGIN

CmdInterface* backlight_cmd_create(CmdListener* listener);

DECLES_END

#endif // BACKLIGHT_CMD_H

