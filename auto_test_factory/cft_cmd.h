#ifndef CFT_CMD_H
#define CFT_CMD_H

#include "cmd_interface.h"
#include "cmd_listener.h"

DECLES_BEGIN

CmdInterface* cft_cmd_create(CmdListener* listener);

DECLES_END

#endif // CFT_CMD_H
