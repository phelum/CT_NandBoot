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

//static const int AW_USB_FEL_BULK_EP_OUT=0x01;
//static const int AW_USB_FEL_BULK_EP_IN=0x82;
static int AW_USB_FEL_BULK_EP_OUT=0x01;
static int AW_USB_FEL_BULK_EP_IN=0x82;
static int aw_timeout = 60000;
//static int aw_timeout = 1000;

void usb_bulk_send(libusb_device_handle *usb, int ep, const void *cdata, int length)
{
	uint8_t *data = (uint8_t*) cdata;
	int rc, sent;
	while (length > 0) {
		rc = libusb_bulk_transfer(usb, ep, data, length, &sent, aw_timeout);
		if (rc != 0) {
			fprintf(stderr, "libusb usb_bulk_send error %d (= %s)\n", rc, libusb_error_name (rc));
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
			fprintf(stderr, "usb_bulk_recv error %d (= %s)\n", rc, libusb_error_name (rc));
			exit(2);
//			recv = 0;
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
	case 0x1610: soc_name = "??? flash mode ?"; break;
	case 0x1623: soc_name = "A10"; break;
	case 0x1625: soc_name = "A13"; break;
	case 0x1633: soc_name = "A31"; break;
	case 0x1651: soc_name = "A20"; break;
	case 0x1650: soc_name = "A23"; break;
	case 0x1680: soc_name = "H3" ; break;
	}

	printf ("%.8s soc=%04x (%s) ?=%02x protocol=%02x ?=%02x ?=%02x scratchpad=%04x ?=%02x ?=%02x\n",
			buf.signature, buf.soc_id >> 8, soc_name, buf.unknown_0a, buf.protocol,
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
static const int AW_FEL_2_0206  = 0x0206;
static const int AW_FEL_2_0209  = 0x0209;
static const int AW_FEL_2_020A  = 0x020A;
static const int AW_FEL_2_020C  = 0x020C;
static const int AW_FEL_2_020D  = 0x020D;
static const int AW_FEL_2_020F  = 0x020F;
static const int AW_FEL_2_0230  = 0x0230;

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


#define FEX_CMD_fes_trans				0x0201
#define	FEX_CMD_fes_run 				0x0202
#define FEX_CMD_fes_down				0x0206
#define FEX_CMD_fes_up	    			0x0207
#define FEX_CMD_fes_verify    			0x0208
#define FEX_CMD_fes_query_storage		0x0209
#define FEX_CMD_fes_probe_hardware  	0x020A
#define FEX_CMD_fes_flash_set_on		0x020A
#define FEX_CMD_fes_flash_set_off		0x020B
#define FEX_CMD_fes_verify_value    	0x020C
#define FEX_CMD_fes_verify_status   	0x020D
#define FEX_CMD_fes_flash_size_probe	0x020E
#define FEX_CMD_fes_tool_mode			0x020F
#define FEX_CMD_fes_memset				0x0210
#define FEX_CMD_fes_pmu					0x0211
#define FEX_CMD_fes_unseqmem_read		0x0212
#define FEX_CMD_fes_unseqmem_write		0x0213
#define FEX_CMD_fes_force_erase			0x0220
#define FEX_CMD_fes_force_erase_key		0x0221
#define FEX_CMD_fes_reset_cpu			0x0214
#define FEX_CMD_fes_low_power_manger	0x0215
#define FEX_CMD_fes_query_secure		0x0230


#define  SUNXI_EFEX_DATA_TYPE_MASK		(0x7fff)
#define  SUNXI_EFEX_DRAM_MASK			(0x7f00)
#define  SUNXI_EFEX_DRAM_TAG			(0x7f00)
#define  SUNXI_EFEX_MBR_TAG				(0x7f01)
#define  SUNXI_EFEX_BOOT1_TAG			(0x7f02)
#define  SUNXI_EFEX_BOOT0_TAG			(0x7f03)
#define  SUNXI_EFEX_ERASE_TAG           (0x7f04)
#define  SUNXI_EFEX_PMU_SET             (0x7f05)
#define  SUNXI_EFEX_UNSEQ_MEM_FOR_READ  (0x7f06)
#define  SUNXI_EFEX_UNSEQ_MEM_FOR_WRITE (0x7f07)
#define  SUNXI_EFEX_FULLIMG_SIZE_TAG    (0x7f10)

#define  SUNXI_EFEX_FLASH_TAG           (0x8000)

#define  SUNXI_EFEX_TRANS_MASK			(0x30000)
#define  SUNXI_EFEX_TRANS_START_TAG		(0x20000)
#define  SUNXI_EFEX_TRANS_FINISH_TAG	(0x10000)

#define  SUNXI_EFEX_VERIFY_STATUS		(0)
#define  SUNXI_EFEX_VERIFY_ADDSUM		(1)
#define  SUNXI_EFEX_VERIFY_CRC32		(2)



struct aw_fex_req {
	uint32_t request;
	uint32_t sector;
	uint32_t len;
	uint32_t flags;
	char     tag [4];
};


void aw_fex_init_req	(aw_fex_req *req)
{
	memset (req, 0, sizeof (aw_fex_req));
	memcpy (req->tag, "AWUC", 4);

	return;
}


void aw_fex_down (libusb_device_handle *usb, uint32_t sector, uint32_t len, uint32_t flags, void *data)
{
	struct aw_fex_req req;

	aw_fex_init_req (&req);
	req.request = htole32 (AW_FEL_2_0206);
	req.sector  = htole32 (sector);
	req.len     = htole32 (len);
	req.flags   = htole32 (flags);
	usb_bulk_send (usb, AW_USB_FEL_BULK_EP_OUT, &req, sizeof (req));
	usb_bulk_send (usb, AW_USB_FEL_BULK_EP_OUT, data, len);
	aw_read_usb_response (usb);

	return;
}


void aw_fex_0209 (libusb_device_handle *usb, uint32_t sector, uint32_t len, uint32_t flags, void *data)
{
	struct aw_fex_req req;

	aw_fex_init_req (&req);
	req.request = htole32 (AW_FEL_2_0209);
	req.sector  = htole32 (sector);
	req.len     = htole32 (len);
	req.flags   = htole32 (flags);
	usb_bulk_send (usb, AW_USB_FEL_BULK_EP_OUT, &req, sizeof (req));
	usb_bulk_recv (usb, AW_USB_FEL_BULK_EP_IN, data, 0x04);
	aw_read_usb_response (usb);

	return;
}


void aw_fex_020A (libusb_device_handle *usb)
{
	struct aw_fex_req req;

	aw_fex_init_req (&req);
	req.request = htole32 (FEX_CMD_fes_flash_set_on);
	usb_bulk_send (usb, AW_USB_FEL_BULK_EP_OUT, &req, sizeof (req));
	aw_read_usb_response (usb);

	return;
}


void aw_fex_020B (libusb_device_handle *usb)
{
	struct aw_fex_req req;

	aw_fex_init_req (&req);
	req.request = htole32 (FEX_CMD_fes_flash_set_off);
	usb_bulk_send (usb, AW_USB_FEL_BULK_EP_OUT, &req, sizeof (req));
	aw_read_usb_response (usb);

	return;
}


void aw_fex_020C (libusb_device_handle *usb, uint32_t sector, off64_t len, void *data)
{
	struct aw_fex_req req;

	aw_fex_init_req (&req);
	req.request = htole32 (AW_FEL_2_020C);
	req.sector  = htole32 (sector);
	req.len     = htole32 (len & 0xFFFFFFFF);
	req.flags   = htole32 (len >> 32);
	usb_bulk_send (usb, AW_USB_FEL_BULK_EP_OUT, &req, sizeof (req));
	usb_bulk_recv (usb, AW_USB_FEL_BULK_EP_IN, data, 0x0C);
	aw_read_usb_response (usb);

	return;
}


void aw_fex_020D (libusb_device_handle *usb, uint32_t sector, uint32_t len, uint32_t flags, void *data)
{
	struct aw_fex_req req;

	aw_fex_init_req (&req);
	req.request = htole32 (AW_FEL_2_020D);
	req.sector  = htole32 (sector);
	req.len     = htole32 (len);
	req.flags   = htole32 (flags);
	usb_bulk_send (usb, AW_USB_FEL_BULK_EP_OUT, &req, sizeof (req));
	usb_bulk_recv (usb, AW_USB_FEL_BULK_EP_IN, data, 0x0C);
	aw_read_usb_response (usb);

	return;
}


void aw_fex_020F (libusb_device_handle *usb, uint32_t sector, uint32_t len, uint32_t flags)
{
	struct aw_fex_req req;

	aw_fex_init_req (&req);
	req.request = htole32 (AW_FEL_2_020F);
	req.sector  = htole32 (sector);
	req.len     = htole32 (len);
	req.flags   = htole32 (flags);
	usb_bulk_send (usb, AW_USB_FEL_BULK_EP_OUT, &req, sizeof (req));
	aw_read_usb_response (usb);

	return;
}


void aw_fex_0230 (libusb_device_handle *usb, uint32_t sector, uint32_t len, uint32_t flags, void *data)
{
	struct aw_fex_req req;

	aw_fex_init_req (&req);
	req.request = htole32 (AW_FEL_2_0230);
	req.sector  = htole32 (sector);
	req.len     = htole32 (len);
	req.flags   = htole32 (flags);
	usb_bulk_send (usb, AW_USB_FEL_BULK_EP_OUT, &req, sizeof(req));
	usb_bulk_recv (usb, AW_USB_FEL_BULK_EP_IN, data, 0x04);
	aw_read_usb_response (usb);

	return;
}


typedef struct tag_sys_file_hdr
{
	uint		crc;
	uint		count;
	short		file_hdr_size;
	short		chunk_hdr_size;
	uint		unk_0;
	uint		unk_1;
	uint		unk_2;
	uint		unk_3;
}__attribute__ ((packed)) rSysFileHdr;


typedef struct tag_sys_chunk_hdr
{
	uint		tag;				// 0x0000CAC1
	uint		num4096;
	uint		chunk_size;			// includes header
}__attribute__ ((packed)) rSysChunkHdr;


void aw_fex_send_file (libusb_device_handle *usb, char *fid, uint sector, uint flags, uint check)
{
	uint32_t	chunk_size = 65536;
	char 		*buf = (char*) malloc (chunk_size);
	FILE		*fin;
	off64_t 	file_size;
	uint		file_sectors;
	uint		nand_sec_size = 512;
	uint32_t	read_size, bytes_read, sectors2go, next_sector, sectors_read, work_flags;
	rSysFileHdr	SFH;
	rSysChunkHdr	SCH;

	if (NULL == (fin = fopen (fid, "rb"))) {
		perror("Failed to open file to send: ");
		exit(1);
	}

	work_flags = flags & 0xFFFF;
	if ((work_flags & 0xFF00) == 0x7F00)
		nand_sec_size = 1;  

	printf ("Sending %s...", fid);

	fseeko64 (fin, 0, SEEK_END);
	file_size = ftello64 (fin);
	file_sectors = (file_size + nand_sec_size - 1) / nand_sec_size;
	fseeko64 (fin, 0, SEEK_SET);
	
	if ((work_flags & 0xFF00) == 0x7F00)
		printf ("bytes = %d...", file_sectors);
	else
		printf ("sectors = %d...", file_sectors);
	fflush (stdout);

	sectors2go  = file_sectors;
	next_sector = sector;
//	work_flags |= SUNXI_EFEX_TRANS_START_TAG;

	if (check & 0x02) {
		fread (&SFH, 1, 0x1C, fin);
		printf ("\n");
		hexdump (&SFH, 0, 28);
		if ((SFH.file_hdr_size != 0x1C) || (SFH.chunk_hdr_size != 0x0C)) {
			printf ("Bad SFH tag\n");
			exit (4);
		}
		while (12 == fread (&SCH, 1, 12, fin)) {
			hexdump (&SCH, ftell (fin) - 12, 12);
			if (SCH.tag != 0xCAC1) {
				if (SCH.tag != 0xCAC3) {
					printf ("Bad SCH tag\n");
					exit (4);
				}
			}
			SCH.chunk_size -= 12;
			while (SCH.chunk_size > 0) {
				read_size = SCH.chunk_size;
				if (read_size > chunk_size)
					read_size = chunk_size;
				bytes_read = fread (buf, 1, read_size, fin);
				sectors_read = (bytes_read + nand_sec_size - 1) / nand_sec_size;
				sectors2go -= sectors_read;
				if (sectors2go == 0)
					work_flags |= SUNXI_EFEX_TRANS_FINISH_TAG;
			  	aw_fex_down (usb, next_sector, bytes_read, work_flags, buf);
				next_sector += sectors_read;
				SCH.chunk_size -= read_size;
			}
		}
	} else {
		while (sectors2go > 0) {
			read_size = chunk_size;
			bytes_read = fread (buf, 1, read_size, fin);
			sectors_read = (bytes_read + nand_sec_size - 1) / nand_sec_size;
			sectors2go -= sectors_read;
			if (sectors2go == 0)
				work_flags |= SUNXI_EFEX_TRANS_FINISH_TAG;
		  	aw_fex_down (usb, next_sector, bytes_read, work_flags, buf);
			next_sector += sectors_read;
//			work_flags &= ~SUNXI_EFEX_TRANS_START_TAG; // 0xFFFDFFF;
		}
	}

	printf ("done\n");
	fclose (fin);

	if (check & 0x01) {
		if ((flags & 0xFF00) == 0x7F00)
			aw_fex_020D (usb, sector, 0, flags & 0xFFFF, buf);		// verify status
		else
			aw_fex_020C (usb, sector, file_size, buf);				// verify value
		hexdump (buf, 0, 12);
	}

	free (buf);

	return;
}


void aw_fex_send_buf (libusb_device_handle *usb, void *buf, uint bytes, uint sector, uint flags, uint check)
{
	uint32_t	work_flags;

	printf ("Sending buffer...");

	printf ("bytes = %d...", bytes);
	fflush (stdout);

	work_flags = flags;
//	work_flags |= SUNXI_EFEX_TRANS_START_TAG;
	work_flags |= SUNXI_EFEX_TRANS_FINISH_TAG;
  	aw_fex_down (usb, sector, bytes, work_flags, buf);

	printf ("done\n");

	if (check) {
		if ((flags & 0xFF00) == 0x7F00)
			aw_fex_020D (usb, sector, 0, flags & 0xFFFF, buf);		// verify status
		else
			aw_fex_020C (usb, sector, bytes, buf);					// verify value
		hexdump (buf, 0, 12);
	}

	return;
}


int save_file (const char *name, void *data, size_t size)
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


void *load_file (const char *name, size_t *size)
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


void aw_fel_hexdump (libusb_device_handle *usb, uint32_t offset, size_t size)
{
	unsigned char buf[size];

	aw_fel_read(usb, offset, buf, size);
	hexdump(buf, offset, size);

	return;
}


void aw_fel_dump (libusb_device_handle *usb, uint32_t offset, size_t size)
{
	unsigned char buf[size];

	aw_fel_read(usb, offset, buf, size);
	fwrite(buf, size, 1, stdout);

	return;
}


void aw_fel_fill (libusb_device_handle *usb, uint32_t offset, size_t size, unsigned char value)
{
	unsigned char buf[size];

	memset(buf, value, size);
	aw_fel_write(usb, offset, buf, size);

	return;
}


