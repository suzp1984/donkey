#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include "hwinfo.h"

static void usage() {
	printf("Usage: %s [main_key] [sub_key]", __func__);
}

int main(int argc, char* argv[])
{
	if (argc < 3) {
		usage();
		return -1;
	}

	char* main_key = argv[1];
	char* sub_key = argv[2];

	hwinfo_head_t* hw_head;
	hwinfo_main_key_t* hw_mainkey;
	hwinfo_sub_key_t* hw_subkey;

	int i;
	int j;
	int fd;
	char* buff;
	char* sub_value;
	void* value_buffer;
	struct stat fstat;

	if (stat(BIN_FILE, &fstat) == -1) {
		return -1;
	}

	buff = (char*)malloc(fstat.st_size);

	if ((fd = open(BIN_FILE, O_RDONLY)) < 0) {
		return -2;
	}

	read(fd, buff, fstat.st_size);
	hw_head = (hwinfo_head_t*)buff;

	for(i = 0; i < hw_head->main_key_count; i++) {
		hw_mainkey = (hwinfo_main_key_t*)(buff + sizeof(hwinfo_head_t) + i * sizeof(hwinfo_main_key_t));

		if(strcmp(hw_mainkey->main_name, main_key)) {
			continue;
		}

		for (j = 0; j < hw_mainkey->count; j++) {
			hw_subkey = (hwinfo_sub_key_t*)(buff + hw_mainkey->offset + j * sizeof(hwinfo_sub_key_t));

			if (strcmp(hw_subkey->sub_name, sub_key)) {
				continue;
			}

			value_buffer = (void*)malloc(4 * hw_subkey->words);

			memcpy(value_buffer, (buff + hw_subkey->offset), 4 * hw_subkey->words);

			printf("the value as int is %d\n", *(int*)value_buffer);
			printf("the value as string is %s\n", (char*)value_buffer);

			free(value_buffer);
			return 0;
		}
	}

		/*
	hw_mainkey = (hwinfo_main_key_t*)(buff + sizeof(hwinfo_head_t));
	hw_subkey = (hwinfo_sub_key_t*)(buff + hw_mainkey->offset + 0*sizeof(hwinfo_sub_key_t));
	sub_value = (char*)malloc(hw_subkey->size );
	memcpy(sub_value, (buff + hw_subkey->offset), hw_subkey->size );

	printf("********** hwinfo ***********\n");
	printf("hwinfo main_key_count is %d\n", hw_head->main_key_count);
	printf("the first main_key's name is %s\n", hw_mainkey->main_name);
	printf("the first sub_key's name is %s\n", hw_subkey->sub_name);
	printf("the first sub_key's size is %d\n", hw_subkey->size);
	printf("the first sub_key's value is %s\n", sub_value); */

	close(fd);

	return 0;
}
