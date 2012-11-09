#ifndef PL_SENSOR_CMD_H
#define PL_SENSOR_CMD_H

#include "cmd_interface.h"
#include "cmd_listener.h"

DECLES_BEGIN

CmdInterface* pl_sensor_cmd_create(CmdListener* listener);

DECLES_END

#endif // PL_SENSOR_CMD_H
