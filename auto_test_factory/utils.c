#include "utils.h"

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
    
#include <sys/types.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <linux/input.h>

#define ENG_TMP_FILE    "/data/eng.txt"

static int try_stop_service(const char* name)
{
	char cmdbuf[256];
	int fd;
	int ret = 0;

	memset(cmdbuf, 0, sizeof(cmdbuf));
	sprintf(cmdbuf, "stop %s", name);
	system(cmdbuf);

	usleep(500000);
	memset(cmdbuf, 0, sizeof(cmdbuf));
	sprintf(cmdbuf, "ps | grep %s > %s", name, ENG_TMP_FILE);
	system(cmdbuf);

	usleep(100000);
	fd = open(ENG_TMP_FILE, O_RDONLY);
	if (fd > 0) {
		memset(cmdbuf, 0, sizeof(cmdbuf));
		read(fd, cmdbuf, sizeof(cmdbuf));
		if (strstr(cmdbuf, name) == NULL)
			ret = 1;
	}

	return ret;
}

int start_service(const char* name)
{
	char cmdbuf[256];

	memset(cmdbuf, 0, sizeof(cmdbuf));
	sprintf(cmdbuf, "start %s", name);
	system(cmdbuf);

	usleep(500000);

	return 0;
}

int stop_service(const char* name)
{
	int count = 5;
	while(count > 0) {
		count--;
		if (try_stop_service(name) == 1)
			break;
	}

	if (count > 0) {
		return 1;
	} else {
		return 0;
	}

}

int open_input(const char* dev_name, int mode)
{
	int fd = -1;
	const char* dirname = "/dev/input";
	char devname[PATH_MAX];
	char* filename;
	DIR* dir;
	struct dirent* de;

	dir = opendir(dirname);
	if (dir == NULL) {
		return -1;
	}

	strcpy(devname, dirname);
	filename = devname + strlen(devname);
	*filename++ = '/';

	while((de = readdir(dir))) {
		if ((de->d_name[0] == '.' && de->d_name[1] == '\0') ||
				(de->d_name[0] == '.' && de->d_name[1] == '.'  &&
				 de->d_name[2] == '\0')) {
			continue;
		}
		strcpy(filename, de->d_name);
		fd = open(devname, mode);
		if(fd >= 0) {
			char name[80];
			if(ioctl(fd, EVIOCGNAME(sizeof(name) - 1), &name) < 1) {
				name[0] = '\0';
			}

			if(!strcmp(name, dev_name)) {
				LOGD("Using %s (name = %s)", devname, name);
				break;
			}

			close(fd);
			fd = -1;
		}
	}

	closedir(dir);

	return fd;
}

