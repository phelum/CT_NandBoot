/*
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
 *
 *
 * This program connects to a Cubietruck in FEL mode via the OTG USB port
 * and changes the NAND so the CT will boot from NAND.  The user accessible
 * NAND (MBR and partitions) is not changed.  This has to be loaded by the
 * user (after booting from an SD card).  If already loaded, the CT will
 * boot automatically after this program has finished.
 *
 * - Steven Saunderson (check <http://phelum.net/> for contact details).
 */

#include <unistd.h>
#include "usbfel.inc"

#define uchar unsigned char
#define uint  uint32_t
#define PutNulls(b,l) memset (b, 0, l)

	bool	bShowURBs = true;
	int		rc;
	libusb_device_handle *handle = NULL;
	int detached_iface = -1;


libusb_device_handle* open_usb (void)
{
	handle = libusb_open_device_with_vid_pid (NULL, 0x1f3a, 0xefe8);

	if (!handle) {
		switch (errno) {
		case EACCES:
			fprintf (stderr, "ERROR: You don't have permission to access Allwinner USB FEL device\n");
			break;
		default:
			fprintf (stderr, "ERROR: Allwinner USB FEL device not found!\n");
			break;
		}
		exit (1);
	}

	rc = libusb_claim_interface (handle, 0);

#if defined(__linux__)
	if (rc != LIBUSB_SUCCESS) {
		libusb_detach_kernel_driver (handle, 0);
		detached_iface = 0;
		rc = libusb_claim_interface (handle, 0);
	}
#endif

	assert (rc == 0);

	return handle;
}


int close_usb (libusb_device_handle* handle)
{
	libusb_close (handle);

#if defined(__linux__)
	if (detached_iface >= 0) {
		libusb_attach_kernel_driver (handle, detached_iface);
		detached_iface = -1;
	}
#endif
	
	return 0;
}


void ShowURB (int urb)
{
    if (bShowURBs)
		printf ("URB %d\n", urb);

    return;
}


int	read_log (void *dest, int bytes, char *name)
{
	FILE  *fin;
	uchar *out = (uchar*) dest;
	char  line [1024];
	char  *in = line;
	int   nbyte;

	if (NULL == (fin = fopen(name, "rb"))) {
		perror("Failed to open input file: ");
		exit(1);
	}

	for (*in = 0x00; bytes > 0; in++) {

		if (*in == 0x00) {
			if (NULL == fgets (line, 1024, fin)) {
				perror ("Early EOF on file: ");
				exit (1);
			}

			in = strchr (line, ':');	// might have offset at start
			if (in)
				in++;
			else
				in = line;
		}

		if (*in <= ' ')
			continue;
		
		sscanf (in++, "%02x", &nbyte);
		*out++ = (uchar) nbyte;
		bytes--;
	}

	fclose (fin);

	return 0;
}


int	stage_1_prep (libusb_device_handle *handle, uchar *buf)
{
	int  x;

    ShowURB (5);
	x = aw_fel_get_version (handle);

	if (x != 0x1651) {								// 0x1651 = Cubietruck
		printf ("Expected ID 0x1651, got %04X\n", x);
		exit (1);
	}

	ShowURB (14);
	aw_fel_get_version (handle);					

			// URB 23 - 27

	ShowURB (23);
	aw_fel_read (handle, 0x7e00, buf, 256);			// get 256 0xCC

	for (x = 0; x < 256; x++) {
		if (buf [x] != 0xCC) {
			printf ("Non 0xCC at entry %d\n", x);
			exit (1);
		}
	}

	ShowURB (32);
	aw_fel_get_version (handle);					

	ShowURB (41);
	PutNulls (buf, 4);
	aw_fel_write (handle, 0x7e00, buf, 256);		

	ShowURB (50);
	aw_fel_get_version (handle);					

	return 0;
}


int	install_fes_1_1 (libusb_device_handle *handle, uchar *buf)
{
	int  x;
	FILE *fin;

	ShowURB (63);
	read_log (buf, 0x200, (char*) "pt1_000063");
	aw_fel_write (handle, 0x7010, buf, 0x200);

	ShowURB (72);
	PutNulls (buf, 16);
  	aw_fel_write (handle, 0x7210, buf, 0x10);

	ShowURB (77);
//			Load buffer as per URB 81 (0xae0 = 2784) FES_1-1 with nulls after.
//			We do lots of sanity checks here (relic from testing).
	PutNulls (buf, 65536);
	read_log (buf, 0x0ae0, (char*) "pt1_000081");		// data from log

	if (NULL == (fin = fopen ("fes_1-1.fex", "rb"))) {
		perror("Failed to open file to send: ");
		exit(1);
	}

	fread (buf + 2784, 1, 2784, fin);                   // data from file
	fclose (fin);

	for (x = 0; x < 2784; x++) {
		if (buf [x] != buf [x + 2784]) {
			printf ("Buf mismatch at entry %d\n", x);
			exit (1);
		}
	}

// 	aw_fel_write (handle, 0x7220, buf, 0x0ae0);
	aw_fel_send_file (handle, 0x7220, (char*) "fes_1-1.fex", 4000, 2784);

	aw_fel_read (handle, 0x7220, buf + 2784, 2784);		// sanity test
	for (x = 0; x < 2784; x++) {
		if (buf [x] != buf [x + 2784]) {
			printf ("Readback mismatch at entry %d\n", x);
			exit (1);
		}
	}

	ShowURB (87);
	aw_fel_execute (handle, 0x7220);				

	usleep (500000);	// need this to avoid error on next USB I/O

	ShowURB (96);
	aw_fel_read (handle, 0x7210, buf, 16);			// expect 'DRAM' then nulls

	if (memcmp (buf, "DRAM\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16)) {
		perror ("Compare to DRAM lit failed");
		exit (1);
	}

	return 0;
}


int	install_fes_1_2 (libusb_device_handle *handle, uchar *buf)
{

	ShowURB (105);
	PutNulls (buf, 16);
  	aw_fel_write (handle, 0x7210, buf, 0x10);

	ShowURB (114);
//			load buffer as per URB 114 (0xae4 = 2788) FES_1-2
//	memset (buf, 0, 65536);
//	read_log (buf, 0x0ae4, (char*) "dump114");
//	if (NULL == (fin = fopen ("fes_1-2.fex", "rb"))) {
//		perror("Failed to open file to send: ");
//		exit(1);
//	}
//	fread (buf + 2788, 1, 2788, fin);
//	fclose (fin);
//	for (x = 0; x < 2788; x++) {
//		if (buf [x] != buf [x + 2788]) {
//			printf ("Buf mismatch at entry %d\n", x);
//			exit (1);
//		}
//	}
//	aw_fel_write (handle, 0x2000, buf, 0x0ae4);
	aw_fel_send_file (handle, 0x2000, (char*) "fes_1-2.fex");

	ShowURB (120);
	aw_fel_execute (handle, 0x2000);				

	ShowURB (129);
	aw_fel_read (handle, 0x7210, buf, 16);			// expect 'DRAM', 0x01 then nulls

	if (memcmp (buf, "DRAM\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16)) {
		perror ("Compare to DRAM1 lit failed");
		exit (1);
	}

	ShowURB (138);
	aw_fel_read (handle, 0x7010, buf, 0x200);		// expect as per URB 138
	read_log (buf + 0x200, 0x0200, (char*) "pt1_000138");

	if (memcmp (buf, buf + 0x200, 0x200)) {
		perror ("Compare to pt1_000138 failed");
		exit (1);
	}

	return 0;
}


int	send_crc_table (libusb_device_handle *handle, uchar *buf)
{

	ShowURB (147);
	read_log (buf, 0x2000, (char*) "pt1_000147");
  	aw_fel_write (handle, 0x40100000, buf, 0x2000);			

	ShowURB (153);
    //			read it back to make sure it's correct
  	aw_fel_read (handle, 0x40100000, buf + 0x2000, 0x2000);	
	if (memcmp (buf, buf + 0x2000, 0x2000)) {
		perror ("Compare to pt1_000147 failed");
		exit (1);
	}

	return 0;
}


int	install_fes_2 (libusb_device_handle *handle, uchar *buf)
{

	ShowURB (165);
	PutNulls (buf, 16);
  	aw_fel_write (handle, 0x7210, buf, 0x10);

	ShowURB (174);
//			load buffer as per URB 174 (0x14448 = 83016)	FES_000000000000
//	read_log (buf, 0x10000, (char*) "dump174");
// 	aw_fel_write (handle, 0x40200000, buf, 0x10000);

//	ShowURB (183);
//			load buffer as per URB 183
//	read_log (buf, 0x4448, (char*) "dump183");
//	aw_fel_write (handle, 0x40210000, buf, 0x04448);
	aw_fel_send_file (handle, 0x40200000, (char*) "fes.fex");

	ShowURB (192);
//			load buffer as per URB 192 (0x7ac = 1964)		FES_200000000000
//	read_log (buf, 0x07ac, (char*) "dump192");
// 	aw_fel_write (handle, 0x7220, buf, 0x07ac);
	aw_fel_send_file (handle, 0x7220, (char*) "fes_2.fex");

	ShowURB (198);
	aw_fel_execute (handle, 0x7220);						

	return 0;
}


int	stage_2_prep (libusb_device_handle *handle, uchar *buf)
{
	int  x;

	ShowURB (5);
	aw_fel_get_version (handle);

	ShowURB (14);
	aw_fel_get_version (handle);

	ShowURB (24);
 	aw_fel2_read (handle, 0x7e00, buf, 0x100, AW_FEL2_DRAM);

	for (x = 0; x < 4; x++) {
		if (buf [x] != 0x00) {
			printf ("Non 0x00 at entry %d\n", x);
			exit (1);
		}
	}

	for (x = 4; x < 256; x++) {
		if (buf [x] != 0xCC) {
			printf ("Non 0xCC at entry %d\n", x);
			exit (1);
		}
	}

	ShowURB (32);
	aw_fel_get_version (handle);

	ShowURB (42);
	aw_fel2_write (handle, 0x7e00, buf, 0x100, AW_FEL2_DRAM);

	return 0;
}


int	install_fed_nand (libusb_device_handle *handle, uchar *buf)
{

	ShowURB (51);
	read_log (buf, 0x2760, (char*) "pt2_000054");
	aw_fel2_write (handle, 0x40a00000, buf, 0x2760, AW_FEL2_DRAM);

	ShowURB (60);
	aw_fel2_send_file (handle, 0x40360000, AW_FEL2_DRAM, (char*) "magic_de_start.fex");

	ShowURB (69);
	aw_fel2_send_file (handle, 0x40430000, AW_FEL2_DRAM, (char*) "FED_NAND_0000000");

	ShowURB (123);
	aw_fel2_send_file (handle, 0x40360000, AW_FEL2_DRAM, (char*) "magic_de_end.fex");

	ShowURB (132);
	aw_fel2_exec (handle, 0x40430000, 0x31);

	ShowURB (135);
	aw_fel2_send_4uints (handle, 0x40a00000, 0x40a01000, 0, 0);

	ShowURB (140);
	aw_fel2_0203_until_ok (handle);

	ShowURB (150);
	aw_fel2_0204 (handle, 0x0400);

	ShowURB (153);
	aw_pad_read (handle, buf, 0x0400);          // what info is here ?
	hexdump (buf, 0, 256);

	return 0;
}


int DownloadPartition (libusb_device_handle *handle, char *fid,
						uint32_t sector, uint32_t sectors = 0)
{
	char	*buf = (char*) malloc (65536);
	FILE	*fin;
	off64_t file_size;			// allow for file > 2GB, all untested !!!
	uint	file_sectors;
	uint	nand_sec_size = 512;
	uint	usb_rec_size, usb_rec_secs;
	uint	read_bytes, read_secs, bytes_read;
	uint	sector_key, sector_limit;
	uint	usb_flags;

	usb_rec_size = 65536;
	usb_rec_secs = usb_rec_size / nand_sec_size;
	usb_rec_size = usb_rec_secs * nand_sec_size;

	if (NULL == (fin = fopen64 (fid, "rb"))) {
		perror("Failed to open file to send: ");
		exit(1);
	}

	aw_fel2_send_file (handle, 0x40360000, AW_FEL2_DRAM, (char*) "magic_cr_start.fex");

	printf ("Sending %s...", fid);
	fflush (stdout);

	fseeko64 (fin, 0, SEEK_END);
	file_size = ftello64 (fin);
	file_sectors = (file_size + nand_sec_size - 1) / nand_sec_size;
	fseeko64 (fin, 0, SEEK_SET);

	sector_key = sector;
	if (sectors < file_sectors)
		sectors = file_sectors;
	sector_limit = sector_key + sectors;

	while (sector_key < sector_limit) {
		read_secs = sector_key - sector_limit;
		if (read_secs > usb_rec_secs)
			read_secs = usb_rec_secs;
		read_bytes = read_secs * nand_sec_size;

		bytes_read = fread (buf, 1, read_bytes, fin);
		if (bytes_read < read_bytes)
			PutNulls (buf + bytes_read, read_bytes - bytes_read);

		usb_flags = AW_FEL2_NAND | AW_FEL2_WR;
		if (sector_key == sector)
			usb_flags |= AW_FEL2_FIRST;
		if (sector_key + read_secs == sector_limit)
			usb_flags |= AW_FEL2_LAST;
		aw_fel2_write (handle, sector_key, buf, read_bytes, usb_flags);

		sector_key += read_secs;
	}

	printf ("done\n");
	fclose (fin);

	aw_fel2_send_file (handle, 0x40360000, AW_FEL2_DRAM, (char*) "magic_cr_end.fex");

	free (buf);

	return 0;
}


int	send_partitions_and_MBR (libusb_device_handle *handle, uchar *buf)
{
	return 0;									// nothing here !!!

	PutNulls (buf, 12);
	aw_fel2_write (handle, 0x40023c00, buf, 0x0c, AW_FEL2_DRAM);	// reset CRC
	DownloadPartition (handle, (char*) "RFSFAT16_BOOTLOADER_FEX00", 0x8000);
	aw_fel2_read  (handle, 0x40023c00, buf, 0x0c, AW_FEL2_DRAM);	// read CRC

	PutNulls (buf, 12);
	aw_fel2_write (handle, 0x40023c00, buf, 0x0c, AW_FEL2_DRAM);	// reset CRC
	DownloadPartition (handle, (char*) "RFSFAT16_ROOTFS_FEX000000", 0x028000);
//	aw_fel2_read  (handle, 0x40023c00, buf, 0x0c, AW_FEL2_DRAM);	// read CRC

//	PutNulls (buf, 12);
//	aw_fel2_write (handle, 0x40023c00, buf, 0x0c, AW_FEL2_DRAM);	// reset CRC
	DownloadPartition (handle, (char*) "1234567890___MBR", 0x00);
	aw_fel2_read  (handle, 0x40023c00, buf, 0x0c, AW_FEL2_DRAM);	// read CRC

	aw_fel2_0205  (handle, 0x02);									// 2 partitions ?

	return 0;
}


int	install_uboot (libusb_device_handle *handle, uchar *buf)
{
	ShowURB (113241);
	aw_fel2_send_file (handle, 0x40600000, AW_FEL2_DRAM, (char*) "UBOOT_0000000000");

	ShowURB (113303);
	read_log (buf, 0x2760, (char*) "pt2_113307");				// = pt2_000054
	aw_fel2_write (handle, 0x40400000, buf, 0x2760, AW_FEL2_DRAM);

	ShowURB (113547);
	read_log (buf, 0x00ac, (char*) "pt2_113316");
	aw_fel2_write (handle, 0x40410000, buf, 0x00ac, AW_FEL2_DRAM);

	ShowURB (113322);
	aw_fel2_send_file (handle, 0x40360000, AW_FEL2_DRAM, (char*) "magic_de_start.fex");

	ShowURB (113331);
	aw_fel2_send_file (handle, 0x40430000, AW_FEL2_DRAM, (char*) "UPDATE_BOOT1_000");

	ShowURB (113384);
	aw_fel2_send_file (handle, 0x40360000, AW_FEL2_DRAM, (char*) "magic_de_end.fex");

	ShowURB (113394);
	aw_fel2_exec (handle, 0x40430000, 0x11);

	ShowURB (113397);
	aw_fel2_send_4uints (handle, 0x40600000, 0x40400000, 0x40410000, 0);

	ShowURB (113402);
	aw_fel2_0203_until_ok (handle);

	ShowURB (113502);
	aw_fel2_0204 (handle, 0x0400);

	ShowURB (113505);
	aw_pad_read (handle, buf, 0x0400);
	printf ("%s\n", &buf [24]);
	if (strcmp ((char*) &buf [24], "updateBootxOk000"))
		hexdump (buf, 0, 1024);

	return 0;
}


int	install_boot0 (libusb_device_handle *handle, uchar *buf)
{
	ShowURB (113514);
	aw_fel2_send_file (handle, 0x40360000, AW_FEL2_DRAM, (char*) "magic_de_start.fex");

	ShowURB (113523);
	aw_fel2_send_file (handle, 0x40600000, AW_FEL2_DRAM, (char*) "BOOT0_0000000000");

	ShowURB (113532);
	aw_fel2_send_file (handle, 0x40360000, AW_FEL2_DRAM, (char*) "magic_de_end.fex");

	ShowURB (113541);
	read_log (buf, 0x2760, (char*) "pt2_113541");				// = pt2_000054
	aw_fel2_write (handle, 0x40400000, buf, 0x2760, AW_FEL2_DRAM);

	ShowURB (113547);
	read_log (buf, 0x00ac, (char*) "pt2_113550");
	aw_fel2_write (handle, 0x40410000, buf, 0x00ac, AW_FEL2_DRAM);

	ShowURB (113559);
	aw_fel2_send_file (handle, 0x40360000, AW_FEL2_DRAM, (char*) "magic_de_start.fex");

	ShowURB (113565);
	aw_fel2_send_file (handle, 0x40430000, AW_FEL2_DRAM, (char*) "UPDATE_BOOT0_000");

	ShowURB (113610);
	aw_fel2_send_file (handle, 0x40360000, AW_FEL2_DRAM, (char*) "magic_de_end.fex");

	ShowURB (113619);
	aw_fel2_exec (handle, 0x40430000, 0x11);

	ShowURB (113622);
	aw_fel2_send_4uints (handle, 0x40600000, 0x40400000, 0x40410000, 0);

	ShowURB (113628);
	aw_fel2_0203_until_ok (handle);

	ShowURB (113655);
	aw_fel2_0204 (handle, 0x0400);

	ShowURB (113658);
	aw_pad_read (handle, buf, 0x0400);
	printf ("%s\n", &buf [24]);
	if (strcmp ((char*) &buf [24], "updateBootxOk000"))
		hexdump (buf, 0, 1024);

	return 0;
}


int	restore_system (libusb_device_handle *handle, uchar *buf)
{
	ShowURB (113664);
	aw_fel_get_version (handle);

	ShowURB (113673);
	aw_fel2_write (handle, 0x7e04, (char*) "\xcd\xa5\x34\x12", 0x04, AW_FEL2_DRAM);

	ShowURB (113682);
	aw_fel2_send_file (handle, 0x40360000, AW_FEL2_DRAM, (char*) "magic_de_start.fex");

	ShowURB (113691);
	aw_fel2_send_file (handle, 0x40430000, AW_FEL2_DRAM, (char*) "FET_RESTORE_0000");

	ShowURB (113703);
	aw_fel2_send_file (handle, 0x40360000, AW_FEL2_DRAM, (char*) "magic_de_end.fex");

	ShowURB (113709);
	aw_fel2_exec (handle, 0x40430000, 0x11);

	ShowURB (113712);
	PutNulls (buf, 16);
	aw_pad_write (handle, buf, 0x10);

//	aw_fel_get_version (handle);					// added by me, still in flash mode
                                                    // even though restore done.
	return 0;
}


int	stage_1 (libusb_device_handle *handle, uchar *buf)
{
	printf ("Start stage 1\n");

	stage_1_prep (handle, buf);

	install_fes_1_1 (handle, buf);

	install_fes_1_2 (handle, buf);

	send_crc_table (handle, buf);

	install_fes_2 (handle, buf);

	printf ("End of stage 1\n");

	return 0;
}


int	stage_2 (libusb_device_handle *handle, uchar *buf)
{
	printf ("Start stage 2\n");

	stage_2_prep (handle, buf);

	install_fed_nand (handle, buf);

	send_partitions_and_MBR (handle, buf);				// not done yet !!!

	install_uboot (handle, buf);

	install_boot0 (handle, buf);

	restore_system (handle, buf);

	printf ("End of stage 2\n");

	return 0;
}


int main (void)
{
	int x;
	uchar *buf = (uchar*) malloc (65536);

	rc = libusb_init (NULL);
	assert (rc == 0);

	handle = open_usb ();
	stage_1 (handle, buf);
	close_usb (handle);
	
	printf ("Waiting for 10 seconds");
  	fflush (stdout);
	for (x = 0; x < 10; x++) {
		usleep (1000000);
		printf (".");
		fflush (stdout);
	}
	printf ("\n");

	handle = open_usb ();
	stage_2 (handle, buf);
	close_usb (handle);

	printf ("All done\n");

	return 0;
}

