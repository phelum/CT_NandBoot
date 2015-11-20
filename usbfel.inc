/*
 * Copyright (C) 2012  Henrik Nordstrom <henrik@henriknordstrom.net>
 *
 * Changed by Steven Saunderson (check <http://phelum.net/> for contact details).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Needs _BSD_SOURCE for htole and letoh  */
//#define _BSD_SOURCE
#define _NETBSD_SOURCE

#include <libusb.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "endian_compat.h"

struct  aw_usb_request {
	char signature[8];
	uint32_t length;
	uint32_t unknown1;	/* 0x0c000000 */
	uint16_t request;
	uint32_t length2;	/* Same as length */
	char	pad[10];
}  __attribute__((packed));

static const int AW_USB_READ = 0x11;
static const int AW_USB_WRITE = 0x12;

static const int AW_USB_FEL_BULK_EP_OUT=0x01;
static const int AW_USB_FEL_BULK_EP_IN=0x82;
static int aw_timeout = 60000;

void usb_bulk_send(libusb_device_handle *usb, int ep, const void *cdata, int length)
{
	uint8_t *data = (uint8_t*) cdata;
	int rc, sent;
	while (length > 0) {
		rc = libusb_bulk_transfer(usb, ep, data, length, &sent, aw_timeout);
		if (rc != 0) {
			fprintf(stderr, "libusb usb_bulk_send error %d\n", rc);
			exit(2);
		}
		length -= sent;
		data += sent;
	}
}

void usb_bulk_recv(libusb_device_handle *usb, int ep, void *cdata, int length)
{
	uint8_t *data = (uint8_t*) cdata;
	int rc, recv;
	while (length > 0) {
		rc = libusb_bulk_transfer(usb, ep, data, length, &recv, aw_timeout);
		if (rc != 0) {
			fprintf(stderr, "usb_bulk_recv error %d\n", rc);
//			exit(2);
			recv = 0;
		}
		length -= recv;
		data += recv;
	}
}

void aw_send_usb_request(libusb_device_handle *usb, int type, int length)
{
	struct aw_usb_request req;
	memset(&req, 0, sizeof(req));
	strcpy(req.signature, "AWUC");
	req.length = req.length2 = htole32(length);
	req.request = htole16(type);
	req.unknown1 = htole32(0x0c000000);
	usb_bulk_send(usb, AW_USB_FEL_BULK_EP_OUT, &req, sizeof(req));
}

void aw_read_usb_response(libusb_device_handle *usb)
{
	char buf[13];
	usb_bulk_recv(usb, AW_USB_FEL_BULK_EP_IN, &buf, sizeof(buf));
	assert(strcmp(buf, "AWUS") == 0);
}

void aw_usb_write(libusb_device_handle *usb, const void *data, size_t len)
{
	aw_send_usb_request(usb, AW_USB_WRITE, len);
	usb_bulk_send(usb, AW_USB_FEL_BULK_EP_OUT, data, len);
	aw_read_usb_response(usb);
}

void aw_usb_read(libusb_device_handle *usb, const void *data, size_t len)
{
	aw_send_usb_request(usb, AW_USB_READ, len);
	usb_bulk_send(usb, AW_USB_FEL_BULK_EP_IN, data, len);
	aw_read_usb_response(usb);
}

struct aw_fel_request {
	uint32_t request;
	uint32_t address;
	uint32_t length;
	uint32_t pad;
};

static const int AW_FEL_VERSION = 0x0001;
static const int AW_FEL_1_WRITE = 0x0101;
static const int AW_FEL_1_EXEC  = 0x0102;
static const int AW_FEL_1_READ  = 0x0103;

void aw_send_fel_request (libusb_device_handle *usb, int type,
						  uint32_t addr, uint32_t length, uint32_t pad = 0)
{
	struct aw_fel_request req;
	memset (&req, 0, sizeof (req));
	req.request = htole32 (type);
	req.address = htole32 (addr);
	req.length  = htole32 (length);
	req.pad     = htole32 (pad);
	aw_usb_write (usb, &req, sizeof (req));
}

void aw_send_fel_4uints (libusb_device_handle *usb, uint32_t param1,
						 uint32_t param2, uint32_t param3, uint32_t param4)
{
	struct aw_fel_request req;
	memset (&req, 0, sizeof (req));
	req.request = htole32 (param1);
	req.address = htole32 (param2);
	req.length  = htole32 (param3);
	req.pad     = htole32 (param4);
	aw_usb_write (usb, &req, sizeof (req));

	return;
}

int  aw_read_fel_status (libusb_device_handle *usb)
{
	char buf[8];
	char lit [] = "\xff\xff\x00\x00\x00\x00\x00\x00";

	aw_usb_read (usb, &buf, sizeof(buf));

	if (memcmp (buf, lit, 8)) {
		printf ("aw_read_fel_status error, got %02X %02X %02X %02X %02X %02X %02X %02X \n",
			buf [0], buf [1], buf [2], buf [3], buf [4], buf [5], buf [6], buf [7]);
	}

	return (memcmp (buf, lit, 8));
}

uint32_t aw_fel_get_version (libusb_device_handle *usb)
{
	struct aw_fel_version {
		char signature[8];
		uint32_t soc_id;		/* 0x00162300 */
		uint32_t unknown_0a;	/* 1 */
		uint16_t protocol;		/* 1 */
		uint8_t  unknown_12;	/* 0x44 */
		uint8_t  unknown_13;	/* 0x08 */
		uint32_t scratchpad;	/* 0x7e00 */
		uint32_t pad[2];		/* unused */
	} __attribute__((packed)) buf;

	aw_send_fel_request (usb, AW_FEL_VERSION, 0, 0);
	aw_usb_read (usb, &buf, sizeof(buf));
	aw_read_fel_status (usb);

	buf.soc_id = le32toh (buf.soc_id);
	buf.unknown_0a = le32toh (buf.unknown_0a);
	buf.protocol = le32toh (buf.protocol);
	buf.scratchpad = le16toh (buf.scratchpad);
	buf.pad [0] = le32toh (buf.pad [0]);
	buf.pad [1] = le32toh (buf.pad [1]);

	const char *soc_name = "unknown";
	switch ((buf.soc_id >> 8) & 0xFFFF) {
	case 0x1610: soc_name = "A?? flash mode ?"; break;
	case 0x1623: soc_name = "A10"; break;
	case 0x1625: soc_name = "A13"; break;
	case 0x1633: soc_name = "A31"; break;
	case 0x1651: soc_name = "A20"; break;
	case 0x1650: soc_name = "A23"; break;
	}

	printf ("%.8s soc=%08x(%s) %08x ver=%04x %02x %02x scratchpad=%08x %08x %08x\n",
			buf.signature, buf.soc_id, soc_name, buf.unknown_0a, buf.protocol,
			buf.unknown_12, buf.unknown_13, buf.scratchpad, buf.pad[0], buf.pad[1]);

	return ((buf.soc_id >> 8) & 0xFFFF);
}


void aw_fel_read(libusb_device_handle *usb, uint32_t offset, void *buf, size_t len)
{
	aw_send_fel_request(usb, AW_FEL_1_READ, offset, len);
	aw_usb_read(usb, buf, len);
	aw_read_fel_status(usb);
}


void aw_fel_write(libusb_device_handle *usb, uint32_t offset, void *buf, size_t len)
{
	aw_send_fel_request(usb, AW_FEL_1_WRITE, offset, len);
	aw_usb_write(usb, buf, len);
	aw_read_fel_status(usb);
}


void aw_fel_execute(libusb_device_handle *usb, uint32_t offset, uint32_t param1 = 0, uint32_t param2 = 0)
{
	aw_send_fel_request(usb, AW_FEL_1_EXEC, offset, param1, param2);
	aw_read_fel_status(usb);
}


void aw_fel_send_file (libusb_device_handle *usb, uint32_t offset, char *fid, 
						uint32_t chunk_size = 65536, uint32_t min_bytes = 0)
{
	char *buf = (char*) malloc (chunk_size);
	FILE		*fin;
	uint32_t	file_size, read_size, bytes_read;

	if (NULL == (fin = fopen (fid, "rb"))) {
		perror("Failed to open file to send: ");
		exit(1);
	}

	printf ("Sending %s...", fid);
	fflush (stdout);

	fseek (fin, 0, SEEK_END);
	file_size = ftell (fin);
	fseek (fin, 0, SEEK_SET);

	if (min_bytes < file_size)
		min_bytes = file_size;

	while (min_bytes > 0) {
		read_size = chunk_size;
		bytes_read = fread (buf, 1, read_size, fin);
		min_bytes -= bytes_read;
		while ((min_bytes > 0) && (bytes_read < chunk_size)) {
			buf [bytes_read++] = 0x00;
			min_bytes--;
		}
	  	aw_fel_write (usb, offset, buf, bytes_read);
		offset += bytes_read;
	}

	printf ("done\n");
	fclose (fin);
	free (buf);

	return;
}


static const int AW_FEL_2_RDWR  = 0x0201;
static const int AW_FEL2_RD     = 0x2000;
static const int AW_FEL2_WR     = 0x1000;
static const int AW_FEL2_IO     = AW_FEL2_RD | AW_FEL2_WR;
static const int AW_FEL2_FIRST  = 0x4000;
static const int AW_FEL2_LAST   = 0x8000;
static const int AW_FEL2_NAND   = 0x0020;
static const int AW_FEL2_DRAM   = 0x0000;

static const int AW_FEL_2_EXEC  = 0x0202;
static const int AW_FEL_2_0203  = 0x0203;
static const int AW_FEL_2_0204  = 0x0204;
static const int AW_FEL_2_0205  = 0x0205;


int  aw_pad_read (libusb_device_handle *usb, void *buf, size_t len)
{
	aw_usb_read (usb, buf, len);

	return aw_read_fel_status (usb);
}


int  aw_pad_write (libusb_device_handle *usb, void *buf, size_t len)
{
	aw_usb_write (usb, buf, len);

	return aw_read_fel_status (usb);
}


int  aw_fel2_read (libusb_device_handle *usb, uint32_t offset, void *buf, size_t len, uint32_t specs)
{
	specs &= ~AW_FEL2_IO;
	specs |=  AW_FEL2_RD;
	aw_send_fel_request (usb, AW_FEL_2_RDWR, offset, len, specs);
	aw_usb_read (usb, buf, len);

	return aw_read_fel_status (usb);
}


int  aw_fel2_write (libusb_device_handle *usb, uint32_t offset, void *buf, size_t len, uint32_t specs)
{
	specs &= ~AW_FEL2_IO;
	specs |=  AW_FEL2_WR;
	aw_send_fel_request (usb, AW_FEL_2_RDWR, offset, len, specs);
	aw_usb_write (usb, buf, len);

	return aw_read_fel_status (usb);
}


void aw_fel2_send_file (libusb_device_handle *usb, uint32_t offset, uint32_t specs,
						 char *fid, uint32_t chunk_size = 65536, uint32_t min_bytes = 0)
{
	char *buf = (char*) malloc (chunk_size);
	FILE		*fin;
	uint32_t	file_size, read_size, bytes_read;

	if (NULL == (fin = fopen (fid, "rb"))) {
		perror("Failed to open file to send: ");
		exit(1);
	}

	printf ("Sending %s...", fid);
	fflush (stdout);

	fseek (fin, 0, SEEK_END);
	file_size = ftell (fin);
	fseek (fin, 0, SEEK_SET);

	if (min_bytes < file_size)
		min_bytes = file_size;

	while (min_bytes > 0) {
		read_size = chunk_size;
		bytes_read = fread (buf, 1, read_size, fin);
		min_bytes -= bytes_read;
		while ((min_bytes > 0) && (bytes_read < chunk_size)) {
			buf [bytes_read++] = 0x00;
			min_bytes--;
		}
	  	aw_fel2_write (usb, offset, buf, bytes_read, specs);
		offset += bytes_read;
	}

	printf ("done\n");
	fclose (fin);
	free (buf);

	return;
}


void aw_fel2_exec (libusb_device_handle *usb, uint32_t offset = 0, uint32_t param1 = 0, uint32_t param2 = 0)
{
	aw_send_fel_request (usb, AW_FEL_2_EXEC, offset, param1, param2);
//	aw_read_fel_status (usb);

	return;
}


void aw_fel2_send_4uints (libusb_device_handle *usb, uint32_t param1, uint32_t param2, uint32_t param3, uint32_t param4)
{
	aw_send_fel_4uints (usb, param1, param2, param3, param4);
	aw_read_fel_status (usb);

	return;
}


void aw_fel2_0203 (libusb_device_handle *usb, uint32_t offset = 0, uint32_t param1 = 0, uint32_t param2 = 0)
{
	aw_send_fel_request (usb, AW_FEL_2_0203, offset, param1, param2);
//	aw_read_fel_status (usb);

	return;
}


void aw_fel2_0203_until_ok (libusb_device_handle *usb)
{
	char	buf [64];

	buf [0] = 0xff;

	while (memcmp (buf, "\x00\x01", 2)) {
		printf ("\r..Wait");
		fflush (stdout);
		usleep (10000);
		aw_fel2_0203 (usb);
		aw_pad_read (usb, buf, 0x20);
	}

	printf ("..done\n");

    return;
}


void aw_fel2_0204 (libusb_device_handle *usb, uint32_t length = 0, uint32_t param1 = 0, uint32_t param2 = 0)
{
	aw_send_fel_request (usb, AW_FEL_2_0204, length, param1, param2);
//	aw_read_fel_status (usb);

	return;
}


void aw_fel2_0205 (libusb_device_handle *usb, uint32_t param1 = 0, uint32_t param2 = 0, uint32_t param3 = 0)
{
	aw_send_fel_request (usb, AW_FEL_2_0205, param1, param2, param3);
	aw_read_fel_status (usb);

	return;
}


void hexdump(void *data, uint32_t offset, size_t size)
{
	size_t j;
	unsigned char *buf = (uint8_t*) data;
	for (j = 0; j < size; j+=16) {
		size_t i;
		printf("%08lx: ",(long int)offset + j);
		for (i = 0; i < 16; i++) {
			if ((j+i) < size) {
				printf("%02x ", buf[j+i]);
			} else {
				printf("__ ");
			}
		}
		printf(" ");
		for (i = 0; i < 16; i++) {
			if (j+i >= size) {
				printf(".");
			} else if (isprint(buf[j+i])) {
				printf("%c", buf[j+i]);
			} else {
				printf(".");
			}
		}
		printf("\n");
	}
}

int save_file(const char *name, void *data, size_t size)
{
	FILE *out = fopen(name, "wb");
	int rc;
	if (!out) {
		perror("Failed to open output file: ");
		exit(1);
	}
	rc = fwrite(data, size, 1, out);
	fclose(out);
	return rc;
}

void *load_file(const char *name, size_t *size)
{
	size_t bufsize = 8192;
	size_t offset = 0;
	char *buf = (char*) malloc(bufsize);
	FILE *in;
	if (strcmp(name, "-") == 0)
		in = stdin;
	else
		in = fopen(name, "rb");
	if (!in) {
		perror("Failed to open input file: ");
		exit(1);
	}
	
	while(1) {
		ssize_t len = bufsize - offset;
		ssize_t n = fread(buf+offset, 1, len, in);
		offset += n;
		if (n < len)
			break;
		bufsize <<= 1;
		buf = (char*) realloc (buf, bufsize);
	}
	if (size) 
		*size = offset;
	if (in != stdin)
		fclose(in);
	return buf;
}

void aw_fel_hexdump(libusb_device_handle *usb, uint32_t offset, size_t size)
{
	unsigned char buf[size];
	aw_fel_read(usb, offset, buf, size);
	hexdump(buf, offset, size);
}

void aw_fel_dump(libusb_device_handle *usb, uint32_t offset, size_t size)
{
	unsigned char buf[size];
	aw_fel_read(usb, offset, buf, size);
	fwrite(buf, size, 1, stdout);
}
void aw_fel_fill(libusb_device_handle *usb, uint32_t offset, size_t size, unsigned char value)
{
	unsigned char buf[size];
	memset(buf, value, size);
	aw_fel_write(usb, offset, buf, size);
}


