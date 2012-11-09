#ifndef GSENSOR_CMD_H
#define GSENSOR_CMD_H

#include "cmd_interface.h"
#include "cmd_listener.h"

DECLES_BEGIN

CmdInterface* gsensor_cmd_create(CmdListener* listener);

DECLES_END

#endif // GSENSOR_CMD_H
