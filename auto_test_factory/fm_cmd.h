#ifndef FM_CMD_H
#define FM_CMD_H

#include "cmd_interface.h"
#include "cmd_listener.h"

DECLES_BEGIN

CmdInterface* fm_cmd_create(CmdListener* listener);

DECLES_END

#endif // FM_CMD_H

