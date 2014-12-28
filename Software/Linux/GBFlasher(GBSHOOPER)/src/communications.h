/*
============================================================================
Name        : communications.h
Author      : David Pello
Version     :
Copyright   : (C) David Pello 2012
Description : Ladecadence.net GameBoy FlashCart interface
============================================================================
*/

#ifndef __COMMUNICATIONS_H
#define __COMMUNICATIONS_H

#include <inttypes.h>
#include <ftdi.h>

/* Types */
/*********/
typedef struct
{
	uint8_t type;
	uint8_t data;
} packet_t;

/* function prototypes */
/***********************/
uint16_t gbs_open_ftdi(struct ftdi_context* ftdic);
void gbs_close_ftdi(struct ftdi_context* ftdic);
void gbs_send_byte(struct ftdi_context* ftdic, uint8_t c);
void gbs_send_packet(struct ftdi_context* ftdic, packet_t* pkt);
uint8_t gbs_receive_byte (struct ftdi_context* ftdic, uint8_t* c, 
		uint16_t timeout);
uint16_t gbs_receive_packet(struct ftdi_context* ftdic, packet_t* packet, 
		uint16_t timeout);
void gbs_send_buffer(struct ftdi_context* ftdic, uint8_t* buffer);

#endif
