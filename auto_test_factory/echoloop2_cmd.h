#ifndef ECHOLOOP2_CMD_H
#define ECHOLOOP2_CMD_H

#include "cmd_interface.h"
#include "cmd_listener.h"

DECLES_BEGIN

CmdInterface* echoloop2_cmd_create(CmdListener* listener);

DECLES_END

#endif // ECHOLOOP2_CMD_H
