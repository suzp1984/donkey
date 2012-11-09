#include "vibrator_test_case.h"
#include "ui.h"
#include "minui.h"
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define VIBRATOR_ENABLE_DEV         "/sys/class/timed_output/vibrator/enable"

typedef struct {
	int thread_run;
} PrivInfo;

static void vibrator_on(int on) 
{
	int fd;
	char buf[8];

	fd = open(VIBRATOR_ENABLE_DEV, O_RDWR);
	if (fd < 0) {
		return;
	}

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", on);

	write(fd, buf, strlen(buf));

	close(fd);
}

static void* vibrator_thread_route(void* ctx) 
{
	TestCase* thiz = (TestCase*)ctx;
	DECLES_PRIV(priv, thiz);

	while(priv->thread_run == 1) {
		vibrator_on(500);
		sleep(1);
	}

	return NULL;
}

static int vibrator_test_run(TestCase* thiz)
{
	int passed;
	pthread_t t;
	DECLES_PRIV(priv, thiz);

	ui_screen_clean();
	
	gr_color(255, 255, 255, 255);
	ui_draw_title(UI_TITLE_START_Y, VIBRATOR_TEST_CASE);

	priv->thread_run = 1;
	pthread_create(&t, NULL, vibrator_thread_route, (void*)thiz);

	passed = ui_draw_handle_softkey(thiz->name);
	thiz->passed = passed;

	priv->thread_run = 0;
	pthread_join(t, NULL);

	return 0;
}

static void vibrator_test_destroy(TestCase* thiz)
{
	SAFE_FREE(thiz);
}

TestCase* vibrator_test_case_create(int passed)
{
	TestCase* thiz = (TestCase*) malloc(sizeof(TestCase) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);

		thiz->run = vibrator_test_run;
		thiz->destroy = vibrator_test_destroy;

		thiz->passed = passed;
		thiz->name = VIBRATOR_TEST_CASE;

		priv->thread_run = 0;
	}

	return thiz;
}
