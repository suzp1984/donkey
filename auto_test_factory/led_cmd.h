#ifndef LED_CMD_H
#define LED_CMD_H

#include "cmd_interface.h"
#include "cmd_listener.h"

DECLES_BEGIN

CmdInterface* led_cmd_create(CmdListener* listener);

DECLES_END

#endif // LED_CMD_H
