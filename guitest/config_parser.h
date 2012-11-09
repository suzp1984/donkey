#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#define GUITEST_CONFIG "/data/etc/guitest"
//#define GUITEST_CONFIG "./event_config"

#define END_FILE 1
#define NEW_LINE 2

struct parse_state
{
	char* ptr;
	char* newline;
	char** events;
	int events_size;
	int width;
	int height;
//	void (*parse_line)(struct parse_state* state, int nargs, char** args);
};


int parse_config(char* s, struct parse_state* state);

#endif
