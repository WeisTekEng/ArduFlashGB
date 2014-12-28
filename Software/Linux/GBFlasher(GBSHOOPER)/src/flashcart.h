/*
============================================================================
Name        : flashcart.h
Author      : David Pello
Version     :
Copyright   : (C) David Pello 2012
Description : Ladecadence.net GameBoy FlashCart interface
============================================================================
*/

#ifndef __FLASHCART_H
#define __FLASHCART_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#include "communications.h"
#include "gbshooper.h"

/* Types */
/*********/

typedef struct
{
	uint8_t index;
	char name[30];
	uint32_t size;
} desc_t;

typedef struct
{
	uint8_t version_mayor;
	uint8_t version_minor;
} status_t;

typedef struct
{
	uint8_t manufacturer_id;
	uint8_t chip_id;
	char* manufacturer;
	char* chip;
} flash_id_t;

typedef struct
{
	char* title;
	char* cart;
	char* rom_size;
	char* ram_size;
	uint32_t rom_bytes;
	uint32_t ram_bytes;
	
} rom_header_t;

typedef struct
{
	int size;
	char* file;
	uint8_t progress;
	uint16_t ret;
	uint8_t stat;
} thread_args_t;


/* function prototypes */
/***********************/

uint16_t gbs_status(status_t* status);
uint16_t gbs_flash_id(flash_id_t* id);
uint16_t gbs_read_header(rom_header_t* header);
/* slow routines run in their own threads */
void* gbs_erase_flash(void* ptr);
void* gbs_write_flash(void* ptr);
void* gbs_read_flash(void* ptr);
void* gbs_write_ram(void* ptr);
void* gbs_read_ram(void* ptr);
void* gbs_erase_ram(void* ptr);

#endif
