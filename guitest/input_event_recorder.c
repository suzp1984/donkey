#include <stdio.h>
#include <stdlib.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/poll.h>
#include <time.h>

#include "config_parser.h"
#include "events_player.h"
#include "uinput.h"

static void usage(char* argv)
{
	printf("Usage: %s [a310|a1|a810] \n", argv);
}

static char* gen_filename()
{
	char* fmt = "%F-%H-%M-%S";
	char outstr[50];
	char* ret;
	time_t t;
	struct tm* tmp;

	if((ret = malloc(50)) == NULL)
	{
		fprintf(stderr, "error when malloc to ret\n");
		exit(-1);
	}

	t = time(NULL);
	tmp = localtime(&t);

	if(strftime(outstr, sizeof(outstr), fmt, tmp) == 0)
	{
		fprintf(stderr, "strftime returned 0\n");
		exit(EXIT_FAILURE);
	}

	strcpy(ret, outstr);
	
	return ret;
}

static void show_event(struct input_event* event)
{
	printf("%d %d %d %d %d\n", event->time.tv_sec, event->time.tv_usec,  event->type, event->code, event->value);

	return;
}

static void store_event(int fd, struct input_event* event)
{
	if(fd > 0)
	{
		write(fd, event, sizeof(struct input_event));
	}

	return;
}

static int open_dev(const char* device_name)
{
	int fd = -1;
	fd = open(device_name, O_RDWR);
	if(fd < 0) {
		fprintf(stderr, "error opening: %s\n", device_name);
		return -1;
	}

	return fd;
}

int main(int argc, char* argv[])
{
	int i = 0;
	int fd;
	int config_fd;
	struct pollfd* mfds = (struct pollfd*)calloc(1, sizeof(struct pollfd));
	int mfd_count = 0;
	int nr;
	struct input_event event;
	int store_fd;

	//open config file
	struct stat st;
	char* data;
	int size;
	struct parse_state parse_result;

	if(stat(GUITEST_CONFIG, &st) != 0) {
		printf("unable to read the file state of %s.\n", GUITEST_CONFIG);
		return -1;
	}

	data = (char*)malloc(sizeof(char)*(st.st_size+1));

	config_fd = open(GUITEST_CONFIG, O_RDONLY);
	if(fd < 0) {
		free(data);
		return -1;
	}

	size = read(config_fd, data, st.st_size);
	
	if(size == st.st_size) {
		data[size] = '\0';
	}else{
		return -1;
	}

	parse_config(data, &parse_result);

	if(!strcmp(basename(argv[0]), "input_event_player")) {
		return events_player(argc, argv, parse_result);
	}

	for(i = 0; i < parse_result.events_size; i++)
	{
		printf("open %s\n", parse_result.events[i]);
		fd = open_dev(parse_result.events[i]);
		if(fd > 0)
		{
			struct pollfd* new_mfds = (struct pollfd*)realloc(mfds, sizeof(struct pollfd)*(mfd_count + 1));

			if(new_mfds == NULL)
			{
				fprintf(stderr, "realloc out of memeory\n");
				return -1;
			}

			mfds = new_mfds;

			mfds[i].fd = fd;
			mfds[i].events = POLLIN;
			mfd_count++;
		}
	}

	//open store file
	store_fd = open(gen_filename(), O_RDWR|O_CREAT);
	if(store_fd < 0)
	{
		fprintf(stderr, "fail to open record file\n");
		exit(-1);
	}
	
	while(1)
	{

		nr = poll(mfds, mfd_count, 0);
		if(nr <= 0)
			continue;

		for(i = 0; i < mfd_count; i++)
		{
			if(mfds[i].revents == POLLIN)
			{
				int ret = read(mfds[i].fd, &event, sizeof(event));
				if(ret == sizeof(event))
				{
					show_event(&event);
					store_event(store_fd, &event);
				}

			}

		}
	}

	free(parse_result.events);
	close(config_fd);
	return 0;
}
