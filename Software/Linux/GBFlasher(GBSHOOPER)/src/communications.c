/*
============================================================================
Name        : communications.c
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

#include "communications.h"
#include "gbshooper.h"

/**************************** COMUNICACION ************************************/
uint16_t gbs_open_ftdi(struct ftdi_context* ftdic) {

	uint16_t ret, i, found;
	struct ftdi_device_list* devlist,* curdev;
	struct usb_device* gbs_dev = NULL;
	char manufacturer[128], description[128];


	if (ftdi_init(ftdic) < 0)
	{
		fprintf(stderr, "ftdi_init failed\n");
		return STAT_ERROR;
	}

	if ((ret = ftdi_usb_find_all(ftdic, &devlist, 0x0403, 0x6001)) < 0)
	{
		fprintf(stderr, "ftdi_usb_find_all failed: %d (%s)\n", ret, 
				ftdi_get_error_string(ftdic));
		return STAT_ERROR;
	}

	i = 0;
	found = 0;
	for (curdev = devlist; curdev != NULL; i++)
	{
		if ((ret = ftdi_usb_get_strings(ftdic, curdev->dev, 
						manufacturer, 128, description, 128, NULL, 0)) < 0)
		{
			fprintf(stderr, "ftdi_usb_get_strings failed: %d (%s)\n", ret, 
					ftdi_get_error_string(ftdic));
			return STAT_ERROR;
		}

		if ((strcmp(manufacturer, ID_MANUFACTURER) == 0) 
				&& (strcmp(description, ID_PRODUCT) == 0)) {
			gbs_dev = curdev->dev;
			found = 1;
		}
		curdev = curdev->next;
	}

	if (found) {
		fprintf(stderr, "Found GB Shooper device.\n");
		if ((ret = ftdi_usb_open_dev(ftdic, gbs_dev)) < 0) {
			ftdi_list_free(&devlist);
			ftdi_deinit(ftdic);
			return STAT_ERROR;
		}
		ftdi_set_baudrate(ftdic, BAUDRATE_230_4K);
		ftdi_set_line_property(ftdic, BITS_8, STOP_BIT_1,  NONE);
		ftdi_setflowctrl(ftdic, SIO_DISABLE_FLOW_CTRL);
		ftdi_list_free(&devlist);
		ftdic->max_packet_size=512;
		return STAT_OK;
	}
	else {
		fprintf(stderr, "No GB Shooper device found!\n");
		ftdi_list_free(&devlist);
		ftdi_deinit(ftdic);
		return STAT_ERROR;
	}

	return STAT_ERROR;
}

void gbs_close_ftdi(struct ftdi_context* ftdic) {
	ftdi_usb_close(ftdic);
	ftdi_deinit(ftdic);
}

void gbs_send_byte(struct ftdi_context* ftdic, uint8_t c) {
	ftdi_write_data(ftdic, &c, 1);
	usleep(50);
}

void gbs_send_packet(struct ftdi_context* ftdic, packet_t* pkt) {
	ftdi_write_data(ftdic, &pkt->type, 1);
	usleep(50);
	ftdi_write_data(ftdic, &pkt->data, 1);
}

uint8_t gbs_receive_byte (struct ftdi_context* ftdic, uint8_t* c, 
							uint16_t timeout) {
	time_t tp = time (NULL);
	uint16_t bytesReceived = 0;

	do {
		bytesReceived = ftdi_read_data (ftdic, c, 1);
		if (bytesReceived != 0)
			break;
	} while (time (NULL) - tp < timeout);

	if (bytesReceived == 0)
		return STAT_TIMEOUT;

	if (bytesReceived > 0)
		return STAT_OK;

	if (bytesReceived < 0)
		fprintf(stderr, "ERROR LIBUSB: %s\n",  ftdi_get_error_string (ftdic));

	return STAT_ERROR;
}


uint16_t gbs_receive_packet(struct ftdi_context* ftdic, packet_t* packet, 
							uint16_t timeout) {

	time_t tp = time (NULL);
	uint16_t bytes_received, bytes_left = 0;
	uint16_t remaining = 2;
	do {
		bytes_left = 2;

		bytes_received =
			ftdi_read_data (ftdic, (unsigned char*)&packet[2-remaining],
					bytes_left);
		remaining -= bytes_received;
		tp = time (NULL);

	} while (time (NULL) - tp < timeout && remaining != 0);

	
	/* printf("ERROR LIBUSB: %s\n",  ftdi_get_error_string (ftdic)); */


	if (remaining > 0)
		return STAT_TIMEOUT;
	else
		return STAT_OK;

}



void gbs_send_buffer(struct ftdi_context* ftdic, uint8_t* buffer) {
	ftdi_write_data(ftdic, buffer, BUFFER_SIZE);
}

