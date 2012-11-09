#ifndef OVER_CMD_H
#define OVER_CMD_H

#include "cmd_interface.h"
#include "cmd_listener.h"

DECLES_BEGIN

CmdInterface* over_cmd_create(CmdListener* listener);

DECLES_END

#endif // OVER_CMD_H
