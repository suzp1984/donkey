#ifndef WIFI_CMD_H
#define WIFI_CMD_H

#include "cmd_interface.h"
#include "cmd_listener.h"

DECLES_BEGIN

CmdInterface* wifi_cmd_create(CmdListener* listener);

DECLES_END

#endif // WIFI_CMD_H
