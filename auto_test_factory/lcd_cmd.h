#ifndef LCD_CMD_H
#define LCD_CMD_H

#include "cmd_interface.h"
#include "cmd_listener.h"

DECLES_BEGIN

CmdInterface* lcd_cmd_create(CmdListener* listener);

DECLES_END

#endif // LCD_CMD_H
