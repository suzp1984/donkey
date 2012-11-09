#ifndef CHARGE_CMD_H
#define CHARGE_CMD_H

#include "cmd_interface.h"
#include "cmd_listener.h"

DECLES_BEGIN

CmdInterface* charge_cmd_create(CmdListener* listener);

DECLES_END

#endif // CHARGE_CMD_H
