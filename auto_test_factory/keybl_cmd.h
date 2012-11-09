#ifndef KEYBL_CMD_H
#define KEYBL_CMD_H

#include "cmd_interface.h"
#include "cmd_listener.h"

DECLES_BEGIN

CmdInterface* keybl_cmd_create(CmdListener* listener);

DECLES_END

#endif // KEYBL_CMD_H
