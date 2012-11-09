#include "memorycard_cmd.h"
#include "cmd_common.h"
#include "parcel.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/mount.h>
#include <dirent.h>
#include <sys/vfs.h>

#define LOG_TAG "auto_test"
#include <utils/Log.h>

#define SPRD_SD_DEV         "/dev/block/mmcblk0"
#define SPRD_SDCARD_PATH    "/sdcard"
#define SPRD_SD_TESTFILE    "/sdcard/iiad_test.txt"

#define MSG_TEST "hello world!"

typedef struct {
	CmdListener* listener;
} PrivInfo;

static int memorycard_cmd_mount_sd(CmdInterface* thiz)
{
	int ret;
	DIR* dir;
	struct dirent* ptr;
	char sd[21] = "/dev/block/";
	char* device = "mmcblk0p5";

	dir = opendir("/dev/block/");
	while((ptr = readdir(dir)) != NULL) {
		ret = strncmp(ptr->d_name, "mmcblk0", strlen("mmcblk0"));
		if (ret == 0) {
			if ((strcmp(ptr->d_name, device) < 0) && (strcmp(ptr->d_name, "mmcblk0p") > 0)) {
				device = ptr->d_name;
				LOGE("%s: sdcard: %s", __func__, device);
				break;
			}
		}
	}

	closedir(dir);
	ret = -1;
	mkdir(SPRD_SDCARD_PATH, 0755);

	strcat(sd, device);
	ret = mount(sd, SPRD_SDCARD_PATH, "vfat",
			MS_NOATIME | MS_NODEV | MS_NODIRATIME, "");

	if (!ret) {
		return 0;
	}

	ret = mount("/dev/block/mmcblk0", SPRD_SDCARD_PATH, "vfat",
			MS_NOATIME | MS_NODEV | MS_NODIRATIME, "");

	if (!ret) {
		return 0;
	}

	return -1;
}

static int memorycard_cmd_execute(CmdInterface* thiz, void* ctx)
{
	DECLES_PRIV(priv, thiz);

	char content[4];
	int passed = 0;
	struct statfs fs;
	char buf[32];
	int test_fd = -1;

	memset(content, 0, sizeof(content));
	Parcel* reply = parcel_create();
	parcel_set_main_cmd(reply, DIAG_AUTOTEST_F);
	parcel_set_sub_cmd(reply, AUTOTEST_MEMORYCARD);

	// umount /sdcard
	umount(SPRD_SDCARD_PATH);
	 
	// mount sd
	if (memorycard_cmd_mount_sd(thiz) < 0) {
		passed = 0;
		goto out;
	}

	if (statfs(SPRD_SDCARD_PATH, &fs) < 0) {
		passed = 0;
		goto out;
	}

	if (fs.f_blocks <= 0) {
		passed = 0;
		goto out;
	}

	unlink(SPRD_SD_TESTFILE);
	test_fd = open(SPRD_SD_TESTFILE, O_CREAT | O_RDWR, 0666);

	if (test_fd < 0) {
		passed = 0;
		goto out;
	}
	
	if (write(test_fd, MSG_TEST, strlen(MSG_TEST)) != strlen(MSG_TEST)) {
		passed = 0;
		goto out;
	}
	lseek(test_fd, 0, SEEK_SET);
	memset(buf, 0, sizeof(buf));
	read(test_fd, buf, sizeof(buf));
	if (strncmp(buf, MSG_TEST, strlen(MSG_TEST))) {
		passed = 0;
		goto out;
	}

	passed = 1;
out:
	if (passed) {
		*(uint8_t*)content = AUTOTEST_RET_PASS;
	} else {
		*(uint8_t*)content = AUTOTEST_RET_FAIL;
	}

	parcel_set_content(reply, content, 4);
	cmd_listener_reply(priv->listener, reply);

	parcel_destroy(reply);

	if (test_fd >= 0) {
		close(test_fd);
	}

	umount(SPRD_SDCARD_PATH);

	return 0;
}

static void memorycard_cmd_destroy(CmdInterface* thiz)
{
	SAFE_FREE(thiz);
}

CmdInterface* memorycard_cmd_create(CmdListener* listener)
{
	CmdInterface* thiz = (CmdInterface*)malloc(sizeof(CmdInterface) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->execute = memorycard_cmd_execute;
		thiz->destroy = memorycard_cmd_destroy;

		thiz->cmd = AUTOTEST_MEMORYCARD;
		priv->listener = listener;
	}

	return thiz;
}
