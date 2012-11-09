#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>

#include "config_parser.h"

static int set_event(struct parse_state* state)
{
	state->events = (char**)realloc(state->events, sizeof(char*)*(state->events_size+1));

	state->events[state->events_size - 1] = state->newline;
	return 0;
}

static int set_resolution(struct parse_state* state)
{
	char* tmp = state->newline;

	for(;;) {
		if(*tmp == '\0')
			break;
		if(*tmp == 'x') {
			*tmp = '\0';
			//printf("the width is %d \n", atoi(s));
			state->width = atoi(state->newline);
			tmp++;
			//printf("the height is %d \n", atoi(tmp));
			state->height = atoi(tmp);
			return 0;
		}

		tmp++;
	}

	return 0;
}

static int get_nextline(struct parse_state* state)
{
	char* begin_line = state->ptr;
	state->newline = state->ptr;
	char* tmp;

	for(;;) {
		switch(*state->ptr) {
			case 0:
				return END_FILE;
			case '\n':
				*state->ptr = '\0';
				state->ptr++;
				while(*state->newline && *state->newline != '\n' && !isdigit(*state->newline) 
						&& *state->newline != '/')
				{
					state->newline++;
				}
				
				tmp = state->newline;
				if(isdigit(*tmp)) {
					while(isdigit(*tmp))
					{
						tmp++;
						if(*tmp == 'x' || *tmp == '*') {
							tmp++;
						}
					}

				//	tmp++;
					*tmp = '\0';
					set_resolution(state);
				}

				if(*tmp == '/') {
					for(;;) {
						if(*tmp == '#' || *tmp == ' ') {
							*tmp = '\0';
							break;
						}

						if(*tmp == '\0')
							break;
						tmp++;
					}
					state->events_size++;
					set_event(state);
				}

				return NEW_LINE;
			default:
				state->ptr++;
		}
	}
}

int parse_config(char* s, struct parse_state* state)
{
	int i = 0;
	char* tmp;
	state->ptr = s;
	state->events = (char**)malloc(sizeof(char*));
	state->events_size = 0;
	
	for(;;) {
		switch (get_nextline(state)) {
			case END_FILE:
				printf("*****************\n");
				for(i = 0; i < state->events_size; i++) {
					printf("%s \n", state->events[i]);
				}
				printf("the width is %d\n", state->width);
				printf("the height is %d\n", state->height);
				//free(state->events);
				return 0;
			case NEW_LINE:
				break;
		}
	}
}

#ifdef DEBUG_CONFIG_PARSER

int main()
{
	int fd;
	int size;
	struct stat st;
	char* data;
	struct parse_state state;

	if(stat(GUITEST_CONFIG, &st) != 0) {
		printf("unable to read the file state of %s.\n", GUITEST_CONFIG);
		return -1;
	}

	data = (char*)malloc(sizeof(char)*(st.st_size+1));
	
	fd = open(GUITEST_CONFIG, O_RDONLY);
	if(fd < 0) {
		free(data);
		return -1;
	}

	size = read(fd, data, st.st_size);
	
	if(size == st.st_size) {
		data[size] = '\0';
	}else{
		return -1;
	}

	parse_config(data, &state);

	free(data);
	close(fd);
	return 0;
}
#endif
