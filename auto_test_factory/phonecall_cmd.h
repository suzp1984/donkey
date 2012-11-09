#ifndef PHONECALL_CMD_H
#define PHONECALL_CMD_H

#include "cmd_interface.h"
#include "cmd_listener.h"

DECLES_BEGIN

CmdInterface* phonecall_cmd_create(CmdListener* listener);

DECLES_END

#endif // PHONECALL_CMD_H
