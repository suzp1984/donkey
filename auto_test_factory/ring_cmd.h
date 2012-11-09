#ifndef RING_CMD_H
#define RING_CMD_H

#include "cmd_interface.h"
#include "cmd_listener.h"

DECLES_BEGIN

CmdInterface* ring_cmd_create(CmdListener* listener);

DECLES_END

#endif // RING_CMD_H
