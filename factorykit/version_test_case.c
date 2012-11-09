#include "version_test_case.h"
#include "common.h"
#include "ui.h"
#include "minui.h"

#define LOG_TAG "factorykit"
#include <utils/Log.h>

#include "string.h"
#include <cutils/properties.h>
#include <sys/types.h>
#include <dirent.h>
#include "fk_sqlite.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>

#define PROP_DISPLAY_ID "ro.build.display.id"
#define PROP_ANDROID_VER "ro.build.version.release"
#define PROP_CUSTOMER_ID "ro.topwise.customer.id"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

typedef struct {
	int max_line_length;
	int start_y;
} PrivInfo;

static void version_test_output_multi_line(TestCase* thiz, char* buf)
{
	DECLES_PRIV(priv, thiz);
	char tmpbuf[priv->max_line_length];

	if ((int)strlen(buf) > priv->max_line_length) {
		char* ptr;
		char* end_ptr;
		ptr = buf;
		end_ptr = buf + strlen(buf);
		while (ptr < end_ptr) {
			memset(tmpbuf, 0, sizeof(tmpbuf));
			snprintf(tmpbuf, priv->max_line_length, ptr);
			gr_text(0, priv->start_y, tmpbuf);
			ptr += priv->max_line_length - 1;
			priv->start_y += CHAR_HEIGHT;
		}
	} else {
		gr_text(0, priv->start_y, buf);
		priv->start_y += CHAR_HEIGHT;
	}

	priv->start_y += CHAR_HEIGHT;
}

static int version_test_input_devices(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);

	int fd = -1;
	const char* dirname = "/dev/input";
	char devname[PATH_MAX];
	char* filename;
	DIR* dir;
	struct dirent* de;
	char buf[1024];
	char* ptr;
	char* end_ptr;
	char tmpbuf[priv->max_line_length];

	memset(buf, 0, 1024);
	sprintf(buf, "%s", "Input Devices: ");
	ptr = buf + strlen(buf);

	dir = opendir(dirname);
	if (dir == NULL) {
		return -1;
	}

	strcpy(devname, dirname);
	filename = devname + strlen(devname);
	*filename++ = '/';

	while ((de = readdir(dir))) {
		if ((de->d_name[0] == '.' && de->d_name[1] == '\0') ||
				(de->d_name[0] == '.' && de->d_name[1] == '.' &&
				 de->d_name[2] == '\0')) {
			continue;
		}

		strcpy(filename, de->d_name);
		fd = open(devname, O_RDONLY);

		if (fd >= 0) {
			char name[80];
			if (ioctl(fd, EVIOCGNAME(sizeof(name) - 1), &name) < 1) {
				name[0] = '\0';
			} else {
				// output dev name
				sprintf(ptr, "%s ", name);
				ptr += strlen(name) + 1;
			}

			close(fd);
			fd = -1;
		}
	}

	version_test_output_multi_line(thiz, buf);

	return 0;
}

static int version_test_run(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);
	char display_id[64];
	char tmpbuf[64];
	char* buf;
	int res;

	priv->start_y = 50;
	ui_screen_clean();
	
	gr_color(255, 255, 255, 255);
	ui_draw_title(UI_TITLE_START_Y, VERSION_TEST_CASE);

	gr_color(0, 255, 0, 255);
	// get display id
	memset(display_id, 0, sizeof(display_id));
	property_get(PROP_DISPLAY_ID, display_id, "");
	LOGE("%s: %s\n", __FUNCTION__, display_id);

	buf = (char*)malloc(strlen(display_id) + 10);
	memset(buf, 0, strlen(display_id) + 10);
	sprintf(buf, "%s %s", "Version:", display_id);

	version_test_output_multi_line(thiz, buf);
	free(buf);

	// output custom id
	memset(tmpbuf, 0, sizeof(tmpbuf));
	property_get(PROP_CUSTOMER_ID, tmpbuf, "");
	
	buf = (char*)malloc(strlen(tmpbuf) + 15);
	memset(buf, 0, strlen(tmpbuf) + 15);
	sprintf(buf, "%s %s", "Customer ID:", tmpbuf);
	version_test_output_multi_line(thiz, buf);
	free(buf);

	version_test_input_devices(thiz);

	gr_flip();

	res = ui_draw_handle_softkey(thiz->name);

	thiz->passed = res;
	LOGE("%s: vesion test passed is %d", __func__, thiz->passed);
	return 0;
}

static void version_test_destroy(TestCase* thiz)
{
	SAFE_FREE(thiz);
}

TestCase* version_test_case_create(int passed)
{
	TestCase* thiz = (TestCase*) malloc(sizeof(TestCase) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);

		thiz->run = version_test_run;
		thiz->destroy = version_test_destroy;

		thiz->name = VERSION_TEST_CASE;
		thiz->passed = passed;

		priv->max_line_length = gr_fb_width() / CHAR_WIDTH;
		priv->start_y = 50;
	}

	return thiz;
}
