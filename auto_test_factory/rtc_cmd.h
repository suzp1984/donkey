#ifndef RTC_CMD_H
#define RTC_CMD_H

#include "cmd_interface.h"
#include "cmd_listener.h"

DECLES_BEGIN

CmdInterface* rtc_cmd_create(CmdListener* listener);

DECLES_END

#endif // RTC_CMD_H
