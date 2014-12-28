/*
============================================================================
Name        : gbshooper.h
Author      : David Pello
Version     :
Copyright   : (C) David Pello 2012
Description : Ladecadence.net GameBoy FlashCart interface
============================================================================
*/

#ifndef __GBSHOOPER_H
#define __GBSHOOPER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#ifndef __BUILD_WINDOWS__
#include <ftdi.h>
#include <unistd.h>
#include <inttypes.h>
#endif


/******************************************************************************/
/****************************** DEFINES ***************************************/
/******************************************************************************/

#define BUFFER_SIZE		256

#define GBS_ID			0x17	/* 23 decimal */
#define VER_MAYOR		0
#define VER_MINOR		1

#define BAUDRATE_115_2K	115200
#define BAUDRATE_230_4K	230400
#define BAUDRATE_1M		1000000
#define SLEEPTIME 		3	/* Tiempo de espera de transferencia (seg) */
#define ERASETIME 		60		/* Tiempo de espera para el borrado */

/* Tamaños */
#define S_0K			0
#define S_2K			2048
#define S_8K			8192
#define S_32K			32768
#define S_64K			65536
#define S_128K			131072
#define S_256K			262144
#define	S_512K			524288
#define S_1MB			1048576
#define S_2MB			2097152
#define S_4MB			4194304
#define S_1_1MB			1179648
#define S_1_2MB			1310720
#define S_1_5MB			1572864

/* Status */
#define STAT_OK			0x14	/* 10.4 ;-) */
#define STAT_ERROR		0xEE
#define STAT_TIMEOUT	0xAA

/* Tipos de paquetes */
#define TYPE_COMMAND	0x11
#define TYPE_DATA		0x22
#define TYPE_STAT		0x33
#define TYPE_INFO		0x44

/* Comandos */
#define CMD_ID			0x11
#define	CMD_READ_FLASH	0x22
#define CMD_READ_RAM	0x33
#define CMD_PRG_FLASH	0x44
#define CMD_PRG_RAM		0x55
#define CMD_ERASE_FLASH	0x66
#define CMD_ERASE_RAM	0x77
#define CMD_READ_HEADER	0x88
#define CMD_END			0xFF

/* Mensajes */
#define MSG_VERSION				"GB Shooper v%d.%d\n"
#define MSG_READY				"GB Shooper hardware READY\n"
#define MSG_HARD_VERSION 		"GB Shooper hardware version: %c.%c\n"
#define MSG_ID_OK				"ID OK, manufacturer: %s, chip: %s\n"
#define MSG_ERROR				"FFFFUUUU, Error in operation.\n"
#define MSG_FLASH_ERASING		"ERASING FLASH...\n"
#define MSG_FLASH_ERASED 		"FLASH ERASED\n"
#define MSG_FLASH_PROGRAMMING	"PROGRAMMING FLASH...\n"
#define MSG_FLASH_PROGRAMMED	"FLASH PROGRAMMED\n"
#define MSG_FLASH_READING	   	"READING FLASH...\n"
#define MSG_FLASH_READ    		"FLASH READ\n"
#define MSG_RAM_PROGRAMMING		"WRITING RAM...\n"
#define MSG_RAM_PROGRAMMED		"FLASH WROTE\n"
#define MSG_RAM_READING	   		"READING RAM...\n"
#define MSG_RAM_READ    		"RAM READ\n"
#define MSG_RAM_ERASING			"ERASING RAM...\n"
#define MSG_RAM_ERASED 			"RAM ERASED\n"


#define MSG_TIMEOUT				"TIMEOUT!\n"
#define NEWLINE					'\n'

/* Codigos de salida */
#define EXIT_WIN		0
#define EXIT_FAIL		1

/* IDs */
#define ID_MANUFACTURER	"ladecadence.net"
#define ID_PRODUCT		"GB Flasher"

/* Threads */
#define T_RUNNING	0xFF
#define T_END		0x00


/* function prototypes */
/***********************/

void gbs_help();
void gbs_version();

#endif
