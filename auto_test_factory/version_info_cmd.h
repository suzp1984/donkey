#ifndef VERSION_INFO_CMD_H
#define VERSION_INFO_CMD_H

#include "cmd_interface.h"
#include "cmd_listener.h"

DECLES_BEGIN

CmdInterface* version_info_cmd_create(CmdListener* listener);

DECLES_END

#endif // VERSION_INFO_CMD_H
