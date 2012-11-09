#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include "hwinfo.h"
#include "hwinfo_parser.h"

static  char  *hw_buff = NULL;           //指向第一个主键
static  int   main_key_count = 0;       //保存主键的个数

static  int   _test_str_length(char *str)
{
	int length = 0;

	while(str[length++])
	{
		if(length > 32)
		{
			length = 32;
			break;
		}
	}

	return length;
}


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


Ret hwinfo_parser_fetch(char *main_name, char *sub_name, int value[])
{
	char   main_bkname[32], sub_bkname[32];
	char   *main_char, *sub_char;
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
	//保存主键名称和子键名称，如果超过31字节则截取31字节
	main_char = main_name;
	if(_test_str_length(main_name) > 31)
	{
		memset(main_bkname, 0, 32);
		strncpy(main_bkname, main_name, 31);
		main_char = main_bkname;
	}
	sub_char = sub_name;
	if(_test_str_length(sub_name) > 31)
	{
		memset(sub_bkname, 0, 32);
		strncpy(sub_bkname, sub_name, 31);
		sub_char = sub_bkname;
	}
	
	for(i=0;i<main_key_count;i++)
	{
		main_key = (hwinfo_main_key_t *)(hw_buff + (sizeof(hwinfo_head_t)) + i * sizeof(hwinfo_main_key_t));
		if(strcmp(main_key->main_name, main_char))    //如果主键不匹配，寻找下一个主键
		{
			continue;
		}
		//主键匹配，寻找子键名称匹配
		for(j=0;j<main_key->count;j++)
		{
			sub_key = (hwinfo_sub_key_t *)(hw_buff + main_key->offset + (j * sizeof(hwinfo_sub_key_t)));
			if(strcmp(sub_key->sub_name, sub_char))    //如果主键不匹配，寻找下一个主键
			{
				continue;
			}

			memcpy(value, (hw_buff + sub_key->offset), 4 * sub_key->words);
			return HWINFO_PARSER_OK;
		}
	}

	return HWINFO_PARSER_NOTFOUND;
}

int hwinfo_parser_getcount(char* main_name, char* sub_name)
{
	char   main_bkname[32], sub_bkname[32];
	char   *main_char, *sub_char;
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

	//保存主键名称和子键名称，如果超过31字节则截取31字节
	main_char = main_name;
	if(_test_str_length(main_name) > 31)
	{
		memset(main_bkname, 0, 32);
		strncpy(main_bkname, main_name, 31);
		main_char = main_bkname;
	}
	sub_char = sub_name;
	if(_test_str_length(sub_name) > 31)
	{
		memset(sub_bkname, 0, 32);
		strncpy(sub_bkname, sub_name, 31);
		sub_char = sub_bkname;
	}
	
	for(i=0;i<main_key_count;i++)
	{
		main_key = (hwinfo_main_key_t *)(hw_buff + (sizeof(hwinfo_head_t)) + i * sizeof(hwinfo_main_key_t));
		if(strcmp(main_key->main_name, main_char))    //如果主键不匹配，寻找下一个主键
		{
			continue;
		}
		//主键匹配，寻找子键名称匹配
		for(j=0;j<main_key->count;j++)
		{
			sub_key = (hwinfo_sub_key_t *)(hw_buff + main_key->offset + (j * sizeof(hwinfo_sub_key_t)));
			if(strcmp(sub_key->sub_name, sub_char))    //如果主键不匹配，寻找下一个主键
			{
				continue;
			}

			return sub_key->words;
		}
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

	char* main_key;
	char* sub_key;

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
		value_size = 4 * hwinfo_parser_getcount(main_key, sub_key);

		value_buffer = malloc(value_size);
		hwinfo_parser_fetch(main_key, sub_key, value_buffer);

		printf("main_key:%s, sub_key:%s  string value: %s\n", main_key, sub_key, (char*)value_buffer);
		for (j = 0; j < value_size / 4; j++) {
			printf("main_key:%s, sub_key:%s  int[%d] value: %d\n", main_key, sub_key, j, *((int*)value_buffer + j));
		}

		free(value_buffer);

	}


	hw_head = (hwinfo_head_t*)buff;
	main_key_count = hw_head->main_key_count;

	for (i = 0; i < main_key_count; i++) {
		main_key_t = (hwinfo_main_key_t*)(buff + sizeof(hwinfo_head_t) + i * sizeof(hwinfo_main_key_t));
		printf("main_key: %s\n", main_key_t->main_name);

		for (m = 0; m < main_key_t->count; m++) {
			hwinfo_sub_key_t* sub_key_t = (hwinfo_sub_key_t*)(buff + main_key_t->offset + m * sizeof(hwinfo_sub_key_t));
			value_size = 4 * hwinfo_parser_getcount(main_key_t->main_name, sub_key_t->sub_name);
			value_buffer = malloc(value_size);
			memset(value_buffer, 0, value_size);
			//printf("\nbefore: %s\n", value_buffer);
			hwinfo_parser_fetch(main_key_t->main_name, sub_key_t->sub_name, value_buffer);
			//printf("\n%s\n", value_buffer);

			printf("\tsub_key: %s\n \t\tstring value: %s\n", sub_key_t->sub_name, (char*)value_buffer);

			for (j = 0; j < value_size / 4; j++) {
				printf("\t\tint[%d] value: %d\n", j, *((int*)value_buffer + j));
			}

			free(value_buffer);
		}
	}

	hwinfo_parser_exit();
	return 0;
}

#endif //HWINFO_PARSER_TEST
