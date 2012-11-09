#ifndef BLUETOOTH_CMD_H
#define BLUETOOTH_CMD_H

#include "cmd_interface.h"
#include "cmd_listener.h"

DECLES_BEGIN

CmdInterface* bluetooth_cmd_create(CmdListener* listener);

DECLES_END

#endif // BLUETOOTH_CMD_H
