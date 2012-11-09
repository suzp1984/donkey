#ifndef HEADSET_CMD_H
#define HEADSET_CMD_H

#include "cmd_interface.h"
#include "cmd_listener.h"

DECLES_BEGIN

CmdInterface* headset_cmd_create(CmdListener* listener);

DECLES_END

#endif // HEADSET_CMD_H
