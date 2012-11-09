#ifndef MEMORYCARD_CMD_H
#define MEMORYCARD_CMD_H

#include "cmd_interface.h"
#include "cmd_listener.h"

DECLES_BEGIN

CmdInterface* memorycard_cmd_create(CmdListener* listener);

DECLES_END

#endif // MEMORYCARD_CMD_H
