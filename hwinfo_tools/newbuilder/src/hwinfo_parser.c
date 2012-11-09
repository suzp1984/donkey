#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hwinfo.h"
#include "hwinfo_parser.h"

static  char  *hw_buff = NULL;           //指向第一个主键
static  unsigned short main_key_count = 0;       //保存主键的个数


Ret hwinfo_parser_init(char *buff)
{
	hwinfo_head_t   *hw_head;

	if(buff)
	{
		hw_buff = buff;
		hw_head = (hwinfo_head_t*)hw_buff;

		main_key_count = hw_head->main_key_count;

		return HWINFO_PARSER_OK;
	}

	return HWINFO_PARSER_FAIL;
}


Ret hwinfo_parser_exit(void)
{
	hw_buff = NULL;
	main_key_count = 0;

	return HWINFO_PARSER_OK;
}


Ret hwinfo_parser_fetch(char *main_name, char *sub_name, unsigned short value[])
{
	hwinfo_main_key_t  *main_key = NULL;
	hwinfo_sub_key_t   *sub_key = NULL;
	int    i, j;

	//检查脚本buffer是否存在
	if(!hw_buff)
	{
		return HWINFO_PARSER_FAIL;
	}
	//检查主键名称和子键名称是否为空
	if((main_name == NULL) || (sub_name == NULL))
	{
		return HWINFO_PARSER_FAIL;
	}
	//检查数据buffer是否为空
	if(value == NULL)
	{
		return HWINFO_PARSER_FAIL;
	}
	
	for(i=0;i<main_key_count;i++)
	{
		char* main_key_n;
		main_key = (hwinfo_main_key_t *)(hw_buff + (sizeof(hwinfo_head_t)) + i * sizeof(hwinfo_main_key_t));
		main_key_n = (char*)malloc(main_key->main_name[1] + 1);
		memset(main_key_n, 0, main_key->main_name[1] + 1);
		memcpy(main_key_n, (char*)(hw_buff + main_key->main_name[0]), main_key->main_name[1]);
		if(strcmp(main_key_n, main_name))    //如果主键不匹配，寻找下一个主键
		{
			free(main_key_n);
			continue;
		}
		//主键匹配，寻找子键名称匹配
		for(j=0;j<main_key->count;j++)
		{
			char* sub_key_n;
			sub_key = (hwinfo_sub_key_t *)(hw_buff + main_key->offset + (j * sizeof(hwinfo_sub_key_t)));
			sub_key_n = (char*)malloc(sub_key->sub_name[1] + 1);
			memset(sub_key_n, 0, sub_key->sub_name[1] + 1);
			memcpy(sub_key_n, (char*)(hw_buff + sub_key->sub_name[0]), sub_key->sub_name[1]);
			if(strcmp(sub_key_n, sub_name))    //如果主键不匹配，寻找下一个主键
			{
				free(sub_key_n);
				continue;
			}

			memcpy(value, (hw_buff + sub_key->offset), 2 * sub_key->words);
			free(sub_key_n);
			return HWINFO_PARSER_OK;
		}

		free(main_key_n);
	}

	return HWINFO_PARSER_NOTFOUND;
}

int hwinfo_parser_getcount(char* main_name, char* sub_name)
{
	hwinfo_main_key_t  *main_key = NULL;
	hwinfo_sub_key_t   *sub_key = NULL;
	int    i, j;

	//检查脚本buffer是否存在
	if(!hw_buff)
	{
		return 0;
	}
	//检查主键名称和子键名称是否为空
	if((main_name == NULL) || (sub_name == NULL))
	{
		return 0;
	}

	
	for(i=0;i<main_key_count;i++)
	{
		char* main_key_n;
		main_key = (hwinfo_main_key_t *)(hw_buff + (sizeof(hwinfo_head_t)) + i * sizeof(hwinfo_main_key_t));
		main_key_n = (char*)malloc(main_key->main_name[1] + 1);
		memset(main_key_n, 0, main_key->main_name[1] + 1);
		memcpy(main_key_n, (char*)(hw_buff + main_key->main_name[0]), main_key->main_name[1]);

		if(strcmp(main_key_n, main_name))    //如果主键不匹配，寻找下一个主键
		{
			free(main_key_n);
			continue;
		}
		//主键匹配，寻找子键名称匹配
		for(j=0;j<main_key->count;j++)
		{
			char* sub_key_n;
			sub_key = (hwinfo_sub_key_t *)(hw_buff + main_key->offset + (j * sizeof(hwinfo_sub_key_t)));
			sub_key_n = (char*)malloc(sub_key->sub_name[1] + 1);
			memset(sub_key_n, 0, sub_key->sub_name[1] + 1);
			memcpy(sub_key_n, (char*)(hw_buff + sub_key->sub_name[0]), sub_key->sub_name[1]);
			if(strcmp(sub_key_n, sub_name))    //如果主键不匹配，寻找下一个主键
			{
				free(sub_key_n);
				continue;
			}

			free(sub_key_n);

			return sub_key->words;
		}

		free(main_key_n);
	}

	return 0;
}

#ifdef HWINFO_PARSER_TEST

static void Usage()
{
	printf("Usage: hwinfo_parser bin_file main_key sub_key\n");
}

int main(int argc, char** argv)
{
	if (argc < 2) {
		Usage();
		return -1;
	}

	int i;
	int m;
	int j;
	int fd;
	char* buff;
	void* value_buffer;
	int value_size;
	struct stat fstat;

	char* main_key = NULL;
	char* sub_key = NULL;

	if ( argc == 4) {
		main_key = argv[2];
		sub_key = argv[3];
	}

	hwinfo_head_t* hw_head;
	hwinfo_main_key_t* main_key_t;
	int main_key_count;

	if (stat(argv[1], &fstat) == -1) {
		return -1;
	}

	buff = (char*)malloc(fstat.st_size);

	if ((fd = open(argv[1], O_RDONLY)) < 0) {
		return -2;
	}

	read(fd, buff, fstat.st_size);

	if (hwinfo_parser_init(buff) != HWINFO_PARSER_OK) {
		printf("hwinfo_parser_init fail\n");
		return -1;
	}

	if (argc == 4) {
		value_size = 2 * hwinfo_parser_getcount(main_key, sub_key);

		value_buffer = malloc(value_size);
		hwinfo_parser_fetch(main_key, sub_key, value_buffer);

		printf("main_key:%s, sub_key:%s  string value: %s\n", main_key, sub_key, (char*)value_buffer);
		for (j = 0; j < value_size / 2; j++) {
			printf("main_key:%s, sub_key:%s  int[%d] value: %d\n", main_key, sub_key, j, *((int*)value_buffer + j));
		}

		free(value_buffer);

	}


	hw_head = (hwinfo_head_t*)buff;
	main_key_count = hw_head->main_key_count;

	for (i = 0; i < main_key_count; i++) {
		char* main_key_n;
		main_key_t = (hwinfo_main_key_t*)(buff + sizeof(hwinfo_head_t) + i * sizeof(hwinfo_main_key_t));
		main_key_n = (char*)malloc(main_key_t->main_name[1] + 1);
		memset(main_key_n, 0, main_key_t->main_name[1] + 1);
		memcpy(main_key_n, (char*)(buff + main_key_t->main_name[0]), main_key_t->main_name[1]);
		

		printf("main_key: %s\n", main_key_n);

		for (m = 0; m < main_key_t->count; m++) {
			char* sub_key_n;
			hwinfo_sub_key_t* sub_key_t = (hwinfo_sub_key_t*)(buff + main_key_t->offset + m * sizeof(hwinfo_sub_key_t));
			sub_key_n = (char*)malloc(sub_key_t->sub_name[1] + 1);
			memset(sub_key_n, 0, sub_key_t->sub_name[1] + 1);
			memcpy(sub_key_n, (char*)(buff + sub_key_t->sub_name[0]), sub_key_t->sub_name[1]);

			value_size = 2 * hwinfo_parser_getcount(main_key_n, sub_key_n);

			value_buffer = malloc(value_size);
			memset(value_buffer, 0, value_size);
			//printf("\nbefore: %s\n", value_buffer);
			hwinfo_parser_fetch(main_key_n, sub_key_n, value_buffer);
			//printf("\n%s\n", value_buffer);

			printf("\tsub_key: %s\n \t\tstring value: %s\n", sub_key_n, (char*)value_buffer);

			for (j = 0; j < value_size / 2; j++) {
				printf("\t\tshort[%d] value: %d\n", j, *((unsigned short *)value_buffer + j));
			}

			free(sub_key_n);
			free(value_buffer);
		}

		free(main_key_n);
	}

	hwinfo_parser_exit();
	return 0;
}

#endif //HWINFO_PARSER_TEST
