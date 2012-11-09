#ifndef TP_CMD_H
#define TP_CMD_H

#include "cmd_interface.h"
#include "cmd_listener.h"

DECLES_BEGIN

CmdInterface* tp_cmd_create(CmdListener* listener);

DECLES_END

#endif // TP_CMD_H

