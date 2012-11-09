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
	int  main_key_count;
	int  version[3];
} hwinfo_head_t;

typedef struct
{
	char main_name[32];
	int  count;
	int  offset;
} hwinfo_main_key_t;

typedef struct
{
	char sub_name[32];
	int  offset;
	int  words;
} hwinfo_sub_key_t;

#endif  // _HWINFO_D_H


