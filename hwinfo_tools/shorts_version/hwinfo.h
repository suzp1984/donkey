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

typedef struct
{
	unsigned short version[2];
	unsigned short size;
	unsigned short main_key_count;
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


