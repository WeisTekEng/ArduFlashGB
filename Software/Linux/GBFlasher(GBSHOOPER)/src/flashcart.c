/*
============================================================================
Name        : flashcart.c
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

#include "flashcart.h"
#include "communications.h"
#include "gbshooper.h"


/* flash chip producers */
desc_t producers[] = {
	{0x01, "AMD"}, {0x02, "AMI"}, {0xe5, "Analog Devices"},
	{0x1f, "Atmel"}, {0x31, "Catalyst"}, {0x34, "Cypress"},
	{0x04, "Fujitsu"}, {0xE0, "Goldstar"}, {0x07, "Hitachi"},
	{0xad, "Hyundai"}, {0xc1, "Infineon"}, {0x89, "Intel"},
	{0xd5, "Intg. Silicon Systems"}, {0xc2, "Macronix"}, {0x29, "Microchip"},
	{0x2c, "Micron"}, {0x1c, "Mitsubishi"}, {0x10, "Nec"},
	{0x15, "Philips Semiconductors"}, {0xce, "Samsung"}, {0x62, "Sanyo"},
	{0x20, "SGS Thomson"}, {0xb0, "Sharp"}, {0xbf, "SST"},
	{0x97, "Texas Instruments"}, {0x98, "Toshiba"}, {0xda, "Winbond"},
	{0x19, "Xicor"}, {0xc9, "Xilinx"}
};

/* flash chip ids */
desc_t chip_ids[] = {
	{0xA4, "29F040B"}, {0xAD, "AM29F016"}
};

/* array of cart types - source GB CPU Manual */
desc_t carts[] = {
	{0x00, "ROM ONLY"}, {0x01, "ROM+MBC1"},
	{0x02, "ROM+MBC1+RAM"}, {0x03, "ROM+MBC1+RAM+BATT"},
	{0x05, "ROM+MBC2"}, {0x06, "ROM+MBC2+BATTERY"},
	{0x08, "ROM+RAM"}, {0x09, "ROM+RAM+BATTERY"},
	{0x11, "ROM+MBC3"},
	{0x0b, "ROM+MMMO1"}, {0x0c, "ROM+MMMO1+SRAM"},
	{0x0d, "ROM+MMMO1+SRAM+BATT"}, {0x0f, "ROM+MBC3+TIMER+BATT"},
	{0x10, "ROM+MBC3+TIMER+RAM+BAT"}, {0x12, "ROM+MBC3+RAM"},
		{0x13, "ROM+MBC3+RAM+BATT"}, {0x19, "ROM+MBC5"},
	{0x1a, "ROM+MBC5+RAM"}, {0x1b, "ROM+MBC5+RAM+BATT"},
	{0x1c, "ROM+MBC5+RUMBLE"}, {0x1d, "ROM+MBC5+RUMBLE+SRAM"},
	{0x1e, "ROM+MBC5+RUMBLE+SRAM+BATT"}, {0x1f, "Pocket Camera"},
	{0xfd, "Bandai TAMA5"}, {0xfe, "Hudson HuC-3"}
};

/* rom sizes */
desc_t rom_sizes[] = {
	{0x00, "32KB", S_32K}, {0x01, "64KB", S_64K}, {0x02, "128KB", S_128K}, 
	{0x03, "256KB", S_256K}, {0x04, "512KB", S_512K}, {0x05, "1MB", S_1MB}, 
	{0x06, "2MB", S_2MB}, {0x07, "4MB", S_4MB}, {0x52, "1.1MB", S_1_1MB}, 
	{0x53, "1.2MB", S_1_2MB}, {0x54, "1.5MB", S_1_5MB}
};

/* ram sizes */
desc_t ram_sizes[] = {
	{0x00, "0KB", S_0K}, {0x01, "2KB", S_2K}, {0x02, "8KB", S_2K}, 
	{0x03, "32KB", S_32K}, 	{0x04, "128KB", S_128K}
};


uint16_t gbs_status(status_t* status) {
	struct ftdi_context ftdic;
	uint16_t err;
	packet_t packet0, packet1, packet2, packet3;	/* packets */

	if (gbs_open_ftdi(&ftdic)==STAT_ERROR) {
		return STAT_ERROR;
	}

	/* pedimos la información */
	ftdi_usb_purge_rx_buffer(&ftdic);
	/* preparamos el paquete */
	packet0.type = TYPE_INFO;
	packet0.data = 0x00;
	/* lo enviamos */
	gbs_send_packet(&ftdic, &packet0);
	
	/* leemos la respuesta */
	err = gbs_receive_packet(&ftdic, &packet1, SLEEPTIME);
	if (err != STAT_OK)
			return STAT_ERROR;
	gbs_receive_packet(&ftdic, &packet2, SLEEPTIME);
	if (err != STAT_OK)
			return STAT_ERROR;

	gbs_receive_packet(&ftdic, &packet3, SLEEPTIME);
	if (err != STAT_OK)
			return STAT_ERROR;

	if (packet1.data != GBS_ID) {
		printf("%d\n", packet1.data);
		gbs_close_ftdi(&ftdic);
		return STAT_ERROR;
	}

	status->version_mayor = packet2.data;
	status->version_minor = packet3.data;

	gbs_close_ftdi(&ftdic);
	return STAT_OK;

}

uint16_t gbs_flash_id(flash_id_t* id) {

	struct ftdi_context ftdic;
	packet_t packet0, packet1, packet2;	/* packets */
	char str[30];
	uint16_t i;
	uint16_t producers_count = sizeof producers / sizeof producers[0];
	uint16_t ids_count = sizeof chip_ids / sizeof chip_ids[0];
	uint16_t info_prod_ok, info_chip_ok = STAT_ERROR;

	if (gbs_open_ftdi(&ftdic)==STAT_ERROR) {
		return STAT_ERROR;
	}

	/* pedimos la información */
	/* preparamos el paquete */
	packet0.type = TYPE_COMMAND;
	packet0.data = CMD_ID;
	/* lo enviamos */
	gbs_send_packet(&ftdic, &packet0);

	/* leemos la respuesta */
	gbs_receive_packet(&ftdic, &packet1, SLEEPTIME);
	gbs_receive_packet(&ftdic, &packet2, SLEEPTIME);

	i=0;

	strcpy (str,"");
	for (i = 0; i < producers_count; i++)
		if (packet1.data == producers[i].index) {
			strcpy (str, producers[i].name);
			id->manufacturer_id = packet1.data;
			info_prod_ok = STAT_OK;
		}
	if (strncmp(str, "", 30) == 0)
			snprintf(str, 30, "Unknown manufacturer: 0x%.2X", packet1.data);
	id->manufacturer = strdup(str);

	strcpy (str,"");
	for (i = 0; i < ids_count; i++)
		if (packet2.data == chip_ids[i].index) {
			strcpy (str, chip_ids[i].name);
			id->chip_id = packet2.data;
			info_chip_ok = STAT_OK;
		}
	if (strncmp(str, "", 30) == 0)
		snprintf(str, 30, "Unknown flash ID: 0x%.2X", packet2.data);
	id->chip = strdup(str);

	if ((info_prod_ok==STAT_OK) && (info_chip_ok==STAT_OK)) {
		gbs_close_ftdi(&ftdic);
		return STAT_OK;
	}
	else {
		gbs_close_ftdi(&ftdic);
		return STAT_ERROR;
	}

	gbs_close_ftdi(&ftdic);
}

uint16_t gbs_read_header(rom_header_t* header) {
	struct ftdi_context ftdic;
	packet_t packet0, packet1, packet2, packet3, packet4;	/* packets */
	char str[30];
	uint16_t i;
	uint16_t carts_count = sizeof carts / sizeof carts[0];
	uint16_t rom_sizes_count = sizeof rom_sizes / sizeof rom_sizes[0];
	uint16_t ram_sizes_count = sizeof ram_sizes / sizeof ram_sizes[0];
	uint16_t header_cart_ok = STAT_ERROR, 
			 header_rom_ok = STAT_ERROR, 
			 header_ram_ok = STAT_ERROR;
	char title[17];

	if (gbs_open_ftdi(&ftdic)==STAT_ERROR) {
		return STAT_ERROR;
	}

	/* pedimos la información */
	/* preparamos el paquete */
	packet0.type = TYPE_COMMAND;
	packet0.data = CMD_READ_HEADER;
	/* lo enviamos */
	gbs_send_packet(&ftdic, &packet0);

	/* leemos la respuesta */
	/* pkt1 = mapper, pkt2 = rom size, pkt3 = ram_size */
	gbs_receive_packet(&ftdic, &packet1, SLEEPTIME);
	gbs_receive_packet(&ftdic, &packet2, SLEEPTIME);
	gbs_receive_packet(&ftdic, &packet3, SLEEPTIME);

	/* receive name */
	for (i=0; i<16; i++)
	{
		gbs_receive_packet(&ftdic, &packet4, SLEEPTIME);
		title[i] = packet4.data;
	}

	header->title = strdup(title);

	i=0;

	/* comparamos con los arrays de valores conocidos */
	strcpy (str,"Unknown cart type");
	for (i = 0; i < carts_count; i++)
		if (packet1.data == carts[i].index) {
			strcpy (str, carts[i].name);
			header_cart_ok = STAT_OK;
		}

	header->cart = strdup(str);

	strcpy (str,"Unknown ROM size");
	header->rom_bytes = 0;
	for (i = 0; i < rom_sizes_count; i++)
		if (packet2.data == rom_sizes[i].index) {
			strcpy (str, rom_sizes[i].name);
			header->rom_bytes = rom_sizes[i].size;
			header_rom_ok = STAT_OK;
		}
	header->rom_size = strdup(str);
	
	strcpy (str,"Unknown RAM size");
	header->ram_bytes = 0;
	for (i = 0; i < ram_sizes_count; i++)
		if (packet3.data == ram_sizes[i].index) {
			strcpy (str, ram_sizes[i].name);
			header->ram_bytes = ram_sizes[i].size;
			header_ram_ok = STAT_OK;
		}
	header->ram_size = strdup(str);


	/* valores correctos ? */
	if ((header_cart_ok==STAT_OK) && (header_rom_ok==STAT_OK) 
			&& (header_ram_ok==STAT_OK)) {
		gbs_close_ftdi(&ftdic);
		return STAT_OK;
	}
	else {
		gbs_close_ftdi(&ftdic);
		// hardware ok?
		status_t s;
		if (gbs_status(&s) == STAT_OK)
		{
			//name can be garbage, add null at the end
			title[15] = '\0';
			return STAT_OK;
		}
		else
			return STAT_ERROR;
	}

	gbs_close_ftdi(&ftdic);
}


void* gbs_erase_flash (void* ptr) {

	struct ftdi_context ftdic;
	packet_t packet0, packet1;	/* packets */
	thread_args_t* args;

	args = (thread_args_t*) ptr;
	args->stat = T_RUNNING;

	if (gbs_open_ftdi(&ftdic)==STAT_ERROR) {
		args->ret = STAT_ERROR;
		args->stat = T_END;
		return NULL;
	}

	/* enviamos el comando */
	/* preparamos el paquete */
	packet0.type = TYPE_COMMAND;
	packet0.data = CMD_ERASE_FLASH;
	/* lo enviamos */
	gbs_send_packet(&ftdic, &packet0);
	/* leemos la respuesta */
	if (gbs_receive_packet(&ftdic, &packet1, ERASETIME) == STAT_TIMEOUT) {
		gbs_close_ftdi(&ftdic);
		args->ret = STAT_ERROR;
		args->stat = T_END;
		return NULL;
	}
	if (packet1.data == STAT_OK) {
		gbs_close_ftdi(&ftdic);
		args->ret = STAT_OK;
		args->stat = T_END;
		return NULL;
	}
	gbs_close_ftdi(&ftdic);
	
	args->ret = STAT_ERROR;
	args->stat = T_END;
	return NULL;
}

void* gbs_write_flash(void* ptr) {	
	struct ftdi_context ftdic;
	uint8_t buffer[BUFFER_SIZE];		/* buffer de envio/recepción */
	packet_t packet0, packet1;			/* packets */
	uint16_t stat, i;
	uint8_t check;
	FILE* r00m;
	uint64_t fsize;
	uint32_t chunk_counter;
	thread_args_t* args;

	args = (thread_args_t*) ptr;
	args->stat = T_RUNNING;

	if ((r00m = fopen(args->file, "rb")) == NULL) {
		args->ret = STAT_ERROR;
		args->stat = T_END;
		return NULL;
	}

	/* get file size in bytes */
	fseek(r00m, 0L, SEEK_END);
	fsize = ftell(r00m);
	/* back to start again */
	fseek(r00m, 0L, SEEK_SET);
	printf("ROM size: %ld bytes\n", fsize);	

	if (gbs_open_ftdi(&ftdic)==STAT_ERROR) {
		args->ret = STAT_ERROR;
		args->stat = T_END;
		return NULL;
	}


	/* comenzamos a grabar */
	chunk_counter = 0;
	packet0.type = TYPE_COMMAND;
	packet0.data = CMD_PRG_FLASH;
	gbs_send_packet(&ftdic, &packet0);

	stat = gbs_receive_packet(&ftdic, &packet1, SLEEPTIME);
	if (stat == STAT_TIMEOUT) {
		packet0.type = TYPE_COMMAND;
		packet0.data = CMD_END;
		gbs_send_packet(&ftdic, &packet0);
		fclose(r00m);
		gbs_close_ftdi(&ftdic);
		args->ret = STAT_ERROR;
		args->stat = T_END;
		return NULL;
	}
	if (packet1.data == STAT_OK) {
		while (!feof(r00m)) {
			/* calculate percentage */
			args->progress = (100*chunk_counter*BUFFER_SIZE)/fsize;

			/* leemos un bloque de bytes */
			stat = fread(&buffer, sizeof(uint8_t), BUFFER_SIZE, r00m);
			check = 0;
			/* calculamos la comprobación */
			for (i=0; i<BUFFER_SIZE; i++) {
				check+=buffer[i];
			}
			/* lo enviamos */
			gbs_send_buffer(&ftdic, (uint8_t*)&buffer);

			/* recibimos la comprobación */
			gbs_receive_packet(&ftdic, &packet1, SLEEPTIME);
			if (packet1.data != check) {	/* bad check */
				/* paramos */
				packet0.type = TYPE_COMMAND;
				packet0.data = CMD_END;
				gbs_send_packet(&ftdic, &packet0);
				fclose(r00m);
				gbs_close_ftdi(&ftdic);
				args->ret = STAT_ERROR;
				args->stat = T_END;
				return NULL;
			}

			/* more bytes to transfer? */
			stat = getc(r00m);
			if (!feof(r00m)) {
				fseek(r00m, -1, SEEK_CUR);
				/* seguimos grabando (si no hemos terminado ya) */
				packet0.type = TYPE_COMMAND;
				packet0.data = CMD_PRG_FLASH;
				gbs_send_packet(&ftdic, &packet0);
				chunk_counter++;
			}
			else 
				break;
			
		}

	}
	else {
		packet0.type = TYPE_COMMAND;
		packet0.data = CMD_END;
		gbs_send_packet(&ftdic, &packet0);
		fclose(r00m);
		gbs_close_ftdi(&ftdic);
		args->ret=STAT_ERROR;
		args->stat = T_END;
		return NULL;
	}

	packet0.type = TYPE_COMMAND;
	packet0.data = CMD_END;
	gbs_send_packet(&ftdic, &packet0);

	
	fclose(r00m);
	gbs_close_ftdi(&ftdic);
	args->ret = STAT_OK;
	args->stat = T_END;
	return NULL;

}

void* gbs_read_flash(void* ptr) {	
	struct ftdi_context ftdic;
	uint8_t buffer[BUFFER_SIZE];		/* buffer de envio/recepción */
	packet_t packet0, packet1, packet2;	/* packets */
	uint32_t i, n, chunks;
	uint8_t check;
	FILE* r00m;
	thread_args_t* args;

	args = (thread_args_t*) ptr;
	args->stat = T_RUNNING;

	if ((r00m = fopen(args->file, "wb")) == NULL) {
		args->ret=STAT_ERROR;
		args->stat = T_END;
		return NULL;
	}

	if (gbs_open_ftdi(&ftdic)==STAT_ERROR) {
		args->ret=STAT_ERROR;
		args->stat = T_END;
		return NULL;
	}

	/* numero de buffers a leer */
	chunks = args->size/BUFFER_SIZE;

	/* comenzamos a recibir */
	packet0.type  = TYPE_COMMAND;
	packet0.data = CMD_READ_FLASH;
	gbs_send_packet(&ftdic, &packet0);

	for (n=0; n<chunks; n++) {
		args->progress = n*BUFFER_SIZE*100/args->size;
		check = 0;
		/* leemos buffer */
		for (i=0; i<BUFFER_SIZE; i++) {
			gbs_receive_byte(&ftdic, &buffer[i], SLEEPTIME);
		}
		/* los escribimos en el archivo */
		fwrite(&buffer, sizeof(uint8_t), BUFFER_SIZE, r00m);

		/* calculamos la suma */
		for (i=0; i<BUFFER_SIZE; i++)
			check+=buffer[i];

		/* enviamos la suma */
		packet2.type  = TYPE_DATA;
		packet2.data  = check;
		gbs_send_packet(&ftdic, &packet2);
		/* respuesta */
		gbs_receive_packet(&ftdic, &packet1, SLEEPTIME);

		/* fallo en la comprobación? */
		if (packet1.data == CMD_END) {
			fclose(r00m);
			gbs_close_ftdi(&ftdic);
			args->ret=STAT_ERROR;
			args->stat = T_END;
			return NULL;
		}

		/* continuamos */
		if (n<chunks-1) {
			packet2.type = TYPE_COMMAND;
			packet2.data = CMD_READ_FLASH;
			gbs_send_packet(&ftdic, &packet2);
		}

	}

	packet0.type = TYPE_COMMAND;
	packet0.data = CMD_END;
	gbs_send_packet(&ftdic, &packet0);


	fclose(r00m);
	gbs_close_ftdi(&ftdic);
	
	args->ret = STAT_OK;
	args->stat = T_END;
	return NULL;
}

void* gbs_write_ram(void* ptr) {	
	struct ftdi_context ftdic;
	uint8_t buffer[BUFFER_SIZE];		/* buffer de envio/recepción */
	packet_t packet0, packet1;			/* packets */
	uint16_t stat, i;
	uint8_t check;
	FILE* r00m;
	uint64_t fsize;
	uint32_t chunk_counter;
	thread_args_t* args;

	args = (thread_args_t*) ptr;
	args->stat = T_RUNNING;


	if ((r00m = fopen(args->file, "rb")) == NULL) {
		args->ret=STAT_ERROR;
		args->stat = T_END;
		return NULL;
	}

	/* get file size in bytes */
	fseek(r00m, 0L, SEEK_END);
	fsize = ftell(r00m);
	/* back to start again */
	fseek(r00m, 0L, SEEK_SET);
	//printf("RAM size: %ld bytes\n", fsize);


	if (gbs_open_ftdi(&ftdic)==STAT_ERROR) {
		args->stat = T_END;
		args->ret=STAT_ERROR;
		return NULL;
	}


	/* comenzamos a grabar */
	chunk_counter = 0;
	packet0.type = TYPE_COMMAND;
	packet0.data = CMD_PRG_RAM;
	gbs_send_packet(&ftdic, &packet0);

	stat = gbs_receive_packet(&ftdic, &packet1, SLEEPTIME);
	if (stat == STAT_TIMEOUT) {
		packet0.type = TYPE_COMMAND;
		packet0.data = CMD_END;
		gbs_send_packet(&ftdic, &packet0);
		printf(MSG_TIMEOUT);
		fclose(r00m);
		gbs_close_ftdi(&ftdic);
		args->ret = STAT_ERROR;
		args->stat = T_END;
		return NULL;
	}
	if (packet1.data == STAT_OK) {

		while (!feof(r00m)) {
			/* calculate percentage */
			args->progress = (100*chunk_counter*BUFFER_SIZE)/fsize;

			/* leemos un bloque de bytes */
			stat = fread(&buffer, sizeof(uint8_t), BUFFER_SIZE, r00m);
			check = 0;
			/* calculamos la comprobación */
			for (i=0; i<BUFFER_SIZE; i++) {
				check+=buffer[i];
			}
			/* lo enviamos */
			gbs_send_buffer(&ftdic, (uint8_t*)&buffer);

			/* recibimos la comprobación */
			gbs_receive_packet(&ftdic, &packet1, SLEEPTIME);
			if (packet1.data != check) {	/* bad check */
				/* paramos */
				packet0.type = TYPE_COMMAND;
				packet0.data = CMD_END;
				gbs_send_packet(&ftdic, &packet0);
				fclose(r00m);
				gbs_close_ftdi(&ftdic);
				args->ret=STAT_ERROR;
				args->stat = T_END;
				return NULL;
			}

			/* more bytes to transfer? */
			stat = getc(r00m);
			if (!feof(r00m)) {
				fseek(r00m, -1, SEEK_CUR);
				/* seguimos grabando (si no hemos terminado ya) */
				packet0.type = TYPE_COMMAND;
				packet0.data = CMD_PRG_RAM;
				gbs_send_packet(&ftdic, &packet0);
				chunk_counter++;
			}
			else 
				break;
			
		}

	}
	else {
		packet0.type = TYPE_COMMAND;
		packet0.data = CMD_END;
		gbs_send_packet(&ftdic, &packet0);
		fclose(r00m);
		gbs_close_ftdi(&ftdic);
		args->ret=STAT_ERROR;
		args->stat = T_END;
		return NULL;
	}

	packet0.type = TYPE_COMMAND;
	packet0.data = CMD_END;
	gbs_send_packet(&ftdic, &packet0);


	fclose(r00m);
	gbs_close_ftdi(&ftdic);
	args->ret=STAT_OK;
	args->stat = T_END;
	return NULL;

}

void* gbs_read_ram(void* ptr) {	
	struct ftdi_context ftdic;
	uint8_t buffer[BUFFER_SIZE];		/* buffer de envio/recepción */
	packet_t packet0, packet1, packet2;	/* packets */
	uint32_t i, n, chunks;
	uint8_t check;
	FILE* r00m;
	thread_args_t* args;

	args = (thread_args_t*) ptr;
	args->stat = T_RUNNING;


	if ((r00m = fopen(args->file, "wb")) == NULL) {
		args->ret=STAT_ERROR;
		args->stat = T_END;
		return NULL;
	}

	if (gbs_open_ftdi(&ftdic)==STAT_ERROR) {
		args->ret=STAT_ERROR;
		args->stat = T_END;
		return NULL;
	}

	/* numero de buffers a leer */
	chunks = args->size/BUFFER_SIZE;

	/* comenzamos a recibir */
	packet0.type  = TYPE_COMMAND;
	packet0.data = CMD_READ_RAM;

	gbs_send_packet(&ftdic, &packet0);

	for (n=0; n<chunks; n++) {
		args->progress = n*BUFFER_SIZE*100/args->size;
		check = 0;
		/* leemos buffer */
		for (i=0; i<BUFFER_SIZE; i++) {
			gbs_receive_byte(&ftdic, &buffer[i], SLEEPTIME);
		}
		/* los escribimos en el archivo */
		fwrite(&buffer, sizeof(uint8_t), BUFFER_SIZE, r00m);

		/* calculamos la suma */
		for (i=0; i<BUFFER_SIZE; i++)
			check+=buffer[i];

		/* enviamos la suma */
		packet2.type  = TYPE_DATA;
		packet2.data  = check;
		gbs_send_packet(&ftdic, &packet2);
		/* respuesta */
		gbs_receive_packet(&ftdic, &packet1, SLEEPTIME);

		/* fallo en la comprobación? */
		if (packet1.data == CMD_END) {
			fclose(r00m);
			gbs_close_ftdi(&ftdic);
			args->ret=STAT_ERROR;
			args->stat = T_END;
			return NULL;
		}

		/* continuamos */
		if (n<chunks-1) {
			packet2.type = TYPE_COMMAND;
			packet2.data = CMD_READ_RAM;
			gbs_send_packet(&ftdic, &packet2);
		}
	}

	packet0.type = TYPE_COMMAND;
	packet0.data = CMD_END;
	gbs_send_packet(&ftdic, &packet0);

	fclose(r00m);
	gbs_close_ftdi(&ftdic);
	args->ret=STAT_OK;
	args->stat = T_END;
	return NULL;
}


void* gbs_erase_ram (void* ptr) {
	struct ftdi_context ftdic;
	packet_t packet0, packet1;			/* packets */
	uint16_t stat, i;
	uint32_t chunk_counter;
	thread_args_t* args;

	args = (thread_args_t*) ptr;
	args->stat = T_RUNNING;


	if (gbs_open_ftdi(&ftdic)==STAT_ERROR) {
		args->ret=STAT_ERROR;
		args->stat = T_END;
		return NULL;
	}

	/* comenzamos a grabar */
	chunk_counter = 0;
	packet0.type = TYPE_COMMAND;
	packet0.data = CMD_ERASE_RAM;
	gbs_send_packet(&ftdic, &packet0);

	stat = gbs_receive_packet(&ftdic, &packet1, SLEEPTIME);
	if (stat == STAT_TIMEOUT) {
		packet0.type = TYPE_COMMAND;
		packet0.data = CMD_END;
		gbs_send_packet(&ftdic, &packet0);
		printf(MSG_TIMEOUT);
		gbs_close_ftdi(&ftdic);
		args->ret=STAT_ERROR;
		args->stat = T_END;
		return NULL;
	}
	if (packet1.data == STAT_OK) {
		for (i=0; i<=args->size/BUFFER_SIZE; i++) {
			/* calculate percentage */
			args->progress = (100*chunk_counter*BUFFER_SIZE)/args->size;

			gbs_receive_packet(&ftdic, &packet1, SLEEPTIME);
			if (packet1.data != STAT_OK) {	/* bad */
				/* paramos */
				packet0.type = TYPE_COMMAND;
				packet0.data = CMD_END;
				gbs_send_packet(&ftdic, &packet0);
				gbs_close_ftdi(&ftdic);
				args->ret=STAT_ERROR;
				return NULL;
			}

			/* continue */
			packet0.type = TYPE_COMMAND;
			packet0.data = CMD_ERASE_RAM;
			gbs_send_packet(&ftdic, &packet0);
			chunk_counter++;

		}

	}
	else {
		packet0.type = TYPE_COMMAND;
		packet0.data = CMD_END;
		gbs_send_packet(&ftdic, &packet0);
		gbs_close_ftdi(&ftdic);
		args->ret=STAT_ERROR;
		args->stat = T_END;
		return NULL;
	}

	packet0.type = TYPE_COMMAND;
	packet0.data = CMD_END;
	gbs_send_packet(&ftdic, &packet0);

	gbs_close_ftdi(&ftdic);
	args->ret=STAT_OK;
	args->stat = T_END;
	return NULL;

}

