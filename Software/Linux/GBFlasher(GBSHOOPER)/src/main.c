/*
============================================================================
Name        : GBShooper.c
Author      : David Pello
Version     :
Copyright   : (C) David Pello 2012
Description : Ladecadence.net GameBoy FlashCart interface
============================================================================
*/

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

#include "gbshooper.h"
#include "communications.h"
#include "flashcart.h"

/******************************************************************************/
/***************************** VARIABLES **************************************/
/******************************************************************************/


/******************************************************************************/
/******************************* DATOS ****************************************/
/******************************************************************************/



/******************************************************************************/
/***************************** FUNCIONES **************************************/
/******************************************************************************/

void gbs_help() {
	printf(MSG_VERSION, VER_MAYOR, VER_MINOR);
	printf("David Pello 2012\n");
	printf("\nUsage:\n");
	printf("\n");
	printf("gbshooper <action> <options> [file]\n");
	printf("\n");
	printf("Actions:\n");
	printf("\t --version: prints the software version.\n");
	printf("\t --status: checks the hardware.\n");
	printf("\t --id: gets the ID of the flash chip.\n");
	printf("\t --read-header: gets header information, mapper and RAM/ROM sizes.\n");
	printf("\t --erase-flash: clears the contents of the flash chip.\n");
	printf("\t --read-flash: reads the contents of the flash chip ");
	printf("and writes it on [file].\n");
	printf("\t\toptions: \n");
	printf("\t\t  --size N: Specify ROM size:\n");
	printf("\t\t\t 1=32KB, 2=64KB, 3=128KB, 4=256KB, 5=512KB, 6=1MB, ");
	printf("7=2MB, 8=4MB\n");
	printf("\t\t If no size is specified, 32KB are read\n");
	printf("\t --write-flash: writes the flash with contents from [file].\n");
	printf("\t --read-ram: reads the contents of the save RAM ");
	printf("and writes it on [file].\n");
	printf("\t\toptions: \n");
	printf("\t\t  --size N: Specify RAM size:\n");
	printf("\t\t\t 1=8KB, 2=32KB, 3=1MB\n");
	printf("\t\t If no size is specified, 8KB are read\n");
	printf("\t --write-ram: writes the save RAM with contents from [file].\n");
	printf("\t --erase-ram: clears the contents of the save RAM with 0's.\n");
	printf("\t\toptions: \n");
	printf("\t\t  --size N: Specify RAM size:\n");
	printf("\t\t\t 1=8KB, 2=32KB, 3=1MB\n");
	printf("\t\t If no size is specified, 8KB are erased\n");
	printf("\t --help: show this help.\n");
printf("\n");
}

void gbs_version() {
	printf(MSG_VERSION, VER_MAYOR, VER_MINOR);
}


/******************************************************************************/
/************************* PROGRAMA PRINCIPAL *********************************/
/******************************************************************************/

int main(int argc, char* argv[]) {

	uint8_t erc;
	uint8_t s;
	uint64_t size;
	int t;

	pthread_t exec_thread;				/* process thread */

	/* sin parámetros, imprime ayuda y sale */
	if (argc == 1) {
		gbs_help();
		return EXIT_FAIL;
	}

	/* selecciona comando */
	if (strcmp(argv[1], "--version")==0) {
		gbs_version();
		return EXIT_WIN;
	}
	if (strcmp(argv[1],"--status")==0) {
		status_t status;
		erc = gbs_status(&status);
		if (erc != STAT_OK)
		{
			printf("Hardware error\n");
			return EXIT_FAIL;
		}
		printf(MSG_READY);
		printf(MSG_HARD_VERSION, status.version_mayor, status.version_minor);
		
		return EXIT_WIN;
	}
	if (strcmp(argv[1],"--id")==0) {
		flash_id_t id;
		gbs_flash_id(&id);
		printf("Flash manufacturer: %s\n", id.manufacturer);
		printf("Flash chip type: %s\n", id.chip);
		return EXIT_WIN;
	}
	if (strcmp(argv[1],"--help")==0) {
		gbs_help();
		return EXIT_WIN;
	}
	if (strcmp(argv[1],"--read-header")==0) {
		rom_header_t header;
		gbs_read_header(&header);
		printf("Cart name: %s\n", header.title);
		printf("Cart type: %s\n", header.cart);
		printf("Cart name: %s\n", header.rom_size);
		printf("Cart name: %s\n", header.ram_size);
		return EXIT_WIN;
	}
	if (strcmp(argv[1],"--erase-flash")==0) {
		thread_args_t args;

		printf(MSG_FLASH_ERASING);
		
		args.stat = T_RUNNING;
		t = pthread_create(&exec_thread, NULL, &gbs_erase_flash, (void*) &args);
		if (t) {
			printf("\n");
			printf(MSG_ERROR);
			return EXIT_FAIL;
		}
		while (args.stat != T_END)
		{
			sleep(1);
		}

		if (args.ret!=STAT_OK)
		{
			printf("\n");
			printf(MSG_ERROR);
			return EXIT_FAIL;
		}
		printf(MSG_FLASH_ERASED);
		return EXIT_WIN;
	}

	if (strcmp(argv[1],"--write-flash")==0) {
		if (argc < 3) {
			gbs_help();
			return EXIT_FAIL;
		} else {
			thread_args_t args;

			args.file = argv[2];
			args.stat = T_RUNNING;
			printf(MSG_FLASH_PROGRAMMING);
			t = pthread_create(&exec_thread, NULL, &gbs_write_flash, (void*) &args);
			if (t) {
				printf("\n");
				printf(MSG_ERROR);
				return EXIT_FAIL;
			}

			//gbs_write_flash(&args);
			while (args.stat != T_END)
			{
				printf("%d%%\r", args.progress);
				fflush(stdout);
			}
			if (args.ret!=STAT_OK)
			{
				printf("\n");
				printf(MSG_ERROR);
				return EXIT_FAIL;
			}

			printf("100%%\n");
			printf(MSG_FLASH_PROGRAMMED);
			return EXIT_WIN;
		}
	}

	if (strcmp(argv[1],"--read-flash")==0) {
		if (argc < 3) {
			gbs_help();
			return EXIT_FAIL;
		} else {
			thread_args_t args;

			if (strcmp(argv[2], "--size") == 0)
			{
				s = atoi(argv[3]);
				switch (s) {
					case 1:
						size = S_32K;
						break;
					case 2:
						size = S_64K;
						break;
					case 3:
						size = S_128K;
						break;
					case 4:
						size = S_256K;
						break;
					case 5:
						size = S_512K;
						break;
					case 6:
						size = S_1MB;
						break;
					case 7:
						size = S_2MB;
						break;
					case 8:
						size = S_4MB;
						break;
					default:
						size = S_32K;
						break;
				}
				args.file = argv[4];
				args.size = size;
			}
			else {
				args.file = argv[2];
				args.size = S_32K;
			}

			args.stat = T_RUNNING;
			printf(MSG_FLASH_READING);
			t = pthread_create(&exec_thread, NULL, &gbs_read_flash, (void*) &args);
			if (t) {
				printf("\n");
				printf(MSG_ERROR);
				return EXIT_FAIL;
			}

			while (args.stat != T_END)
			{
				printf("%d%%\r", args.progress);
				fflush(stdout);
			}
			if (args.ret!=STAT_OK)
			{
				printf("\n");
				printf(MSG_ERROR);
				return EXIT_FAIL;
			}

			printf("100%%\n");
			printf(MSG_FLASH_READ);
			return EXIT_WIN;


		}
	}

	if (strcmp(argv[1],"--write-ram")==0) {
		if (argc < 3) {
			gbs_help();
			return EXIT_FAIL;
		} else {
			thread_args_t args;

			args.file = argv[2];
			args.stat = T_RUNNING;
			printf(MSG_RAM_PROGRAMMING);
			t = pthread_create(&exec_thread, NULL, &gbs_write_ram, (void*) &args);
			if (t) {
				printf("\n");
				printf(MSG_ERROR);
				return EXIT_FAIL;
			}

			while (args.stat != T_END)
			{
				printf("%d%%\r", args.progress);
				fflush(stdout);
			}
			if (args.ret!=STAT_OK)
			{
				printf("\n");
				printf(MSG_ERROR);
				return EXIT_FAIL;
			}

			printf("100%%\n");
			printf(MSG_RAM_PROGRAMMED);
			return EXIT_WIN;

		}
	}

	if (strcmp(argv[1],"--read-ram")==0) {
		if (argc < 3) {
			gbs_help();
			return EXIT_FAIL;
		} else {
			thread_args_t args;

			if (strcmp(argv[2], "--size") == 0)
			{
				s = atoi(argv[3]);
				switch (s) {
					case 1:
						size = S_8K;
						break;
					case 2:
						size = S_32K;
						break;
					case 3:
						size = S_1MB;
						break;
					default:
						size = S_8K;
						break;
				}
				args.size = size;
				args.file = argv[4];
			}
			else {
				args.size = S_8K;
				args.file = argv[2];
			}

			args.stat = T_RUNNING;
			printf(MSG_RAM_READING);
			t = pthread_create(&exec_thread, NULL, &gbs_read_ram, (void*) &args);
			if (t) {
				printf("\n");
				printf(MSG_ERROR);
				return EXIT_FAIL;
			}

			while (args.stat != T_END)
			{
				printf("%d%%\r", args.progress);
				fflush(stdout);
			}
			if (args.ret!=STAT_OK)
			{
				printf("\n");
				printf(MSG_ERROR);
				return EXIT_FAIL;
			}

			printf("100%%\n");
			printf(MSG_RAM_READ);
			return EXIT_WIN;



		}
	}

	if (strcmp(argv[1],"--erase-ram")==0) {
		if (argc < 2) {
			gbs_help();
			return EXIT_FAIL;
		} else {
			thread_args_t args;

			if (strcmp(argv[2], "--size") == 0)
			{
				s = atoi(argv[3]);
				switch (s) {
					case 1:
						size = S_8K;
						break;
					case 2:
						size = S_32K;
						break;
					case 3:
						size = S_1MB;
						break;
					default:
						size = S_8K;
						break;
				}
				args.size = size;
			}
			else
				args.size = S_8K;

			args.stat = T_RUNNING;
			printf(MSG_RAM_ERASING);
			t = pthread_create(&exec_thread, NULL, &gbs_erase_ram, (void*) &args);
			if (t) {
				printf("\n");
				printf(MSG_ERROR);
				return EXIT_FAIL;
			}

			while (args.stat != T_END)
			{
				printf("%d%%\r", args.progress);
				fflush(stdout);
			}
			if (args.ret!=STAT_OK)
			{
				printf("\n");
				printf(MSG_ERROR);
				return EXIT_FAIL;
			}

			printf("100%%\n");
			printf(MSG_RAM_ERASED);
			return EXIT_WIN;
		}
	}	
	
	gbs_help();
	return EXIT_FAIL;

}
