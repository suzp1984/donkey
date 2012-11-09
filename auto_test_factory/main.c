#include "cmd_listener.h"
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <cutils/properties.h>

#define LOG_TAG "auto_test"
#include <utils/Log.h>

#define AUTOTEST_MODE "autotest_mode"

int main(int argc, char* argv[])
{
	int fd_cmdline = -1;
	char cmdline[1024];
	int ret = 0;

	fd_cmdline = open("/proc/cmdline",O_RDONLY);
	if(fd_cmdline < 0) {
		LOGE("%s: open /proc/cmdline fail", __func__);
		return -1;
	}
	memset(cmdline, 0, sizeof(cmdline));
	ret = read(fd_cmdline, cmdline, sizeof(cmdline));
	close(fd_cmdline);
	LOGE("AUTOTEST_MODE: cmdline=%s\n", cmdline);
	if(ret > 0) {
		if(strstr(cmdline,"autotest_mode") == NULL)
			return -1;
	} else {
		return -1;
	}

	system("echo 0 >/proc/sys/kernel/printk");
	usleep(200000);

	property_set("ctl.stop", "zygote");

	CmdListener* listener = cmd_listener_create();
	LOGE("%s: CmdListener create", __func__);

	cmd_listener_start(listener);

	cmd_listener_destroy(listener);
	return 0;
}
