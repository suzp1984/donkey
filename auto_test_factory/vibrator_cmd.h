#ifndef VIBRATE_CMD_H
#define VIBRATE_CMD_H

#include "cmd_interface.h"
#include "cmd_listener.h"

DECLES_BEGIN

CmdInterface* vibrator_cmd_create(CmdListener* listener);

DECLES_END

#endif // VIBRATE_CMD_H
