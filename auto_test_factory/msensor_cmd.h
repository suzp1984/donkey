#ifndef MSENSOR_CMD_H
#define MSENSOR_CMD_H

#include "cmd_interface.h"
#include "cmd_listener.h"

DECLES_BEGIN

CmdInterface* msensor_cmd_create(CmdListener* listener);

DECLES_END

#endif // MSENSOR_CMD_H
