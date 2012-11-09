/*
**********************************************************************************************************************
*											        eGon
*						                     the Embedded System
*									       script parser sub-system
*
*						  Copyright(C), 2006-2010, SoftWinners Microelectronic Co., Ltd.
*                                           All Rights Reserved
*
* File    : script.c
*
* By      : Jerry
*
* Version : V2.00
*
* Date	  :
*
* Descript:
**********************************************************************************************************************
*/
#ifndef  _HWINFO_D_H
#define  _HWINFO_D_H

#define BIN_FILE "broncho.bin"
#define MAGIC_NUM 0xd5f9

#define VERSION0 0x0000
#define VERSION1 0x0001

typedef struct {
	char* mainkey;
	char* subkey;
	char* value;
} HwinfoItem;

typedef struct
{
	unsigned short magic;
	unsigned short version[2];
	unsigned short size;
	unsigned short main_key_count;
	unsigned short reserved[3];
} hwinfo_head_t;

typedef struct
{
	unsigned short main_name[2];
	unsigned short offset;
	unsigned short count;
} hwinfo_main_key_t;

typedef struct
{
	unsigned short sub_name[2];
	unsigned short offset;
	unsigned short words; //short
} hwinfo_sub_key_t;

#endif  // _HWINFO_D_H


