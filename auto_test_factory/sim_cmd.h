#ifndef SIM_CMD_H
#define SIM_CMD_H

#include "cmd_interface.h"
#include "cmd_listener.h"

DECLES_BEGIN

CmdInterface* sim_cmd_create(CmdListener* listener);

DECLES_END

#endif // SIM_CMD_H
