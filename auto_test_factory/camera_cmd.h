#ifndef CAMERA_CMD_H
#define CAMERA_CMD_H

#include "cmd_interface.h"
#include "cmd_listener.h"

DECLES_BEGIN

CmdInterface* camera_cmd_create(CmdListener* listener);

DECLES_END

#endif // CAMERA_CMD_H
