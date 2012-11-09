#ifndef CAMERA1_CMD_H
#define CAMERA1_CMD_H

#include "cmd_interface.h"
#include "cmd_listener.h"

DECLES_BEGIN

CmdInterface* camera1_cmd_create(CmdListener* listener);

DECLES_END

#endif // CAMERA1_CMD_H
