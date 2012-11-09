#include "sd_test.h"
#include "ui.h"
#include "minui.h"

#include <stdlib.h>
#include <sys/mount.h>
#include <dirent.h>
#include <sys/vfs.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define LOG_TAG "factorykit"
#include <utils/Log.h>

#define SPRD_SD_DEV         "/dev/block/mmcblk0"
#define SPRD_SDCARD_PATH    "/sdcard"
#define SPRD_SD_TESTFILE    "/sdcard/iiad_test.txt"

#define MSG_TEST "hello world!"

static int sd_test_mount(TestCase* thiz)
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
				break;
			}
		}
	}

	closedir(dir);
	ret = -1;
	mkdir(SPRD_SDCARD_PATH, 0755);

	strcat(sd, device);
	ret = mount(sd, SPRD_SDCARD_PATH, "vfat", 
			MS_NOATIME|MS_NODEV|MS_NODIRATIME, "");
	if (!ret) {
		return 0;
	}

	ret = mount("/dev/block/mmcblk0", SPRD_SDCARD_PATH, "vfat",
			MS_NOATIME|MS_NODEV|MS_NODIRATIME, "");
	if (!ret) {
		return 0;
	}

	return -1;
}

static int sd_test_run(TestCase* thiz)
{
	int passed = 0;
	struct statfs fs;
	int test_fd = -1;
	char buf[32];

	ui_screen_clean();
	
	gr_color(255, 255, 255, 255);
	ui_draw_title(UI_TITLE_START_Y, SD_TEST_CASE);

	umount(SPRD_SDCARD_PATH);
	if (sd_test_mount(thiz) < 0) {
		passed = 0;
		LOGE("%s: mount fail", __func__);
		goto out;
	}

	if (statfs(SPRD_SDCARD_PATH, &fs) < 0) {
		passed = 0;
		LOGE("%s: statfs %s fail", __func__, SPRD_SDCARD_PATH);
		goto out;
	}

	if (fs.f_blocks <= 0) {
		passed = 0;
		LOGE("%s: f_blocks <= 0", __func__);
		goto out;
	}

	unlink(SPRD_SD_TESTFILE);
	test_fd = open(SPRD_SD_TESTFILE, O_CREAT|O_RDWR, 0666);

	if (test_fd < 0) {
		passed = 0;
		LOGE("%s: open %s fail", __func__, SPRD_SD_TESTFILE);
		goto out;
	}

	if (write(test_fd, MSG_TEST, strlen(MSG_TEST)) != strlen(MSG_TEST)) {
		passed = 0;
		LOGE("%s: %s write fail", __func__, SPRD_SD_TESTFILE);
		goto out;
	}

	lseek(test_fd, 0, SEEK_SET);
	memset(buf, 0, sizeof(buf));
	read(test_fd, buf, sizeof(buf));
	if (strncmp(buf, MSG_TEST, strlen(MSG_TEST))) {
		passed = 0;
		LOGE("%s: %s read fail", __func__, SPRD_SD_TESTFILE);
		goto out;
	}

	passed = 1;
out:
	ui_quit_with_prompt(SD_TEST_CASE, passed);
	if (test_fd >= 0) {
		close(test_fd);
		test_fd = -1;
	}
	umount(SPRD_SDCARD_PATH);
	thiz->passed = passed;

	return 0;
}

static void sd_test_destroy(TestCase* thiz)
{
	SAFE_FREE(thiz);
}

TestCase* sd_test_create(int passed)
{
	TestCase* thiz = (TestCase*)malloc(sizeof(TestCase));

	if (thiz != NULL) {
		thiz->run = sd_test_run;
		thiz->destroy = sd_test_destroy;

		thiz->name = SD_TEST_CASE;
		thiz->passed = passed;
	}

	return thiz;
}
