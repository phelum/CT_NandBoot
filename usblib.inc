/*
 *		Testing / development extras.
 */

#include <iostream>
#include <libusb.h>
using namespace std;

void printdev(libusb_device *dev); //prototype of the function

int libmain() {
	libusb_device **devs; //pointer to pointer of device, used to retrieve a list of devices
	libusb_context *ctx = NULL; //a libusb session
	int r; //for return values
	ssize_t cnt; //holding number of devices in list
	r = libusb_init(&ctx); //initialize a library session
	if(r < 0) {
		cout<<"Init Error "<<r<<endl; //there was an error
				return 1;
	}
	libusb_set_debug(ctx, 3); //set verbosity level to 3, as suggested in the documentation
	cnt = libusb_get_device_list(ctx, &devs); //get the list of devices
	if(cnt < 0) {
		cout<<"Get Device Error"<<endl; //there was an error
	}
	cout<<cnt<<" Devices in list."<<endl; //print total number of usb devices
		ssize_t i; //for iterating through the list
	for(i = 0; i < cnt; i++) {
				printdev(devs[i]); //print specs of this device
		}
		libusb_free_device_list(devs, 1); //free the list, unref the devices in it
		libusb_exit(ctx); //close the session
		return 0;
}

void printdev(libusb_device *dev) {
	libusb_device_descriptor desc;
	int r = libusb_get_device_descriptor(dev, &desc);
	if (r < 0) {
		cout<<"failed to get device descriptor"<<endl;
		return;
	}
	cout<<"Number of possible configurations: "<<(int)desc.bNumConfigurations<<"  ";
	cout<<"Device Class: "<<(int)desc.bDeviceClass<<"  ";
	cout<<"VendorID: "<<desc.idVendor<<"  ";
	cout<<"ProductID: "<<desc.idProduct<<endl;
	libusb_config_descriptor *config;
	libusb_get_config_descriptor(dev, 0, &config);
	cout<<"Interfaces: "<<(int)config->bNumInterfaces<<" ||| ";
	const libusb_interface *inter;
	const libusb_interface_descriptor *interdesc;
	const libusb_endpoint_descriptor *epdesc;
	for(int i=0; i<(int)config->bNumInterfaces; i++) {
		inter = &config->interface[i];
		cout<<"Number of alternate settings: "<<inter->num_altsetting<<" | ";
		for(int j=0; j<inter->num_altsetting; j++) {
			interdesc = &inter->altsetting[j];
			cout<<"Interface Number: "<<(int)interdesc->bInterfaceNumber<<" | ";
			cout<<"Number of endpoints: "<<(int)interdesc->bNumEndpoints<<" | ";
			for(int k=0; k<(int)interdesc->bNumEndpoints; k++) {
				epdesc = &interdesc->endpoint[k];
				cout<<"Descriptor Type: "<<(int)epdesc->bDescriptorType<<" | ";
				cout<<"EP Address: "<<(int)epdesc->bEndpointAddress<<" | ";
			}
		}
	}
	cout<<endl<<endl<<endl;
	libusb_free_config_descriptor(config);
}


int		SaveMem					(uint addr, uint len, uchar *buf, char *fid)
{
		uint			done, chunk;

	for (done = 0; done < len; done += chunk) {
		chunk = len - done;
		if (chunk > 256)
			chunk = 256;
		if (version == 0x1610)
			aw_fel2_read (handle, addr + done, buf + done, chunk, AW_FEL2_DRAM);
		else
			aw_fel_read  (handle, addr + done, buf + done, chunk);
	}

	save_file (fid, buf, len);

	return 0;
}


int		SavePad					(uchar *buf, char *fid)
{
//	return SaveMem (0x7000, 0x1000, buf, fid);
	return SaveMem (0x0000, 0x8000, buf, fid);
}


int		USBTests				(uchar *buf)
{
		int				x;

//	libmain ();

	handle = open_usb ();

	ShowUSBSpeed			(handle);					// 12 Mbps here
	version = aw_fel_get_version (handle);					

	SavePad (buf, (char *) "Dump0_000000");

	PutNulls (buf, 4);
	aw_fel_write			(handle, 0x7e00, buf, 4);		
	aw_fel_get_version		(handle);					

	read_log (buf, 0x200, (char*) "pt1_000063");		// DRAM access specs
	aw_fel_write			(handle, 0x7010, buf, 0x200);

	PutNulls (buf, 16);									// clear error log
  	aw_fel_write			(handle, 0x7210, buf, 0x10);

	aw_fel_send_file		(handle, 0x7220, (char*) "fes_1-1.fex", 2784, 2784);
	aw_fel_execute			(handle, 0x7220);				

	usleep (500000);	// need this to avoid error on next USB I/O

	aw_fel_read				(handle, 0x7210, buf, 16);	// expect 'DRAM' then nulls
	hexdump (buf, 0, 16);
	version = aw_fel_get_version (handle);					

	SavePad (buf, (char *) "Dump0_000100");

	PutNulls (buf, 16);
  	aw_fel_write			(handle, 0x7210, buf, 0x10);

	aw_fel_send_file		(handle, 0x2000, (char*) "fes_1-2.fex");
	aw_fel_execute			(handle, 0x2000);				

	aw_fel_read (handle, 0x7210, buf, 16);				// expect 'DRAM', 0x01 then nulls
	hexdump (buf, 0, 16);

	aw_fel_read (handle, 0x7010, buf, 0x200);			// expect as per URB 138
	if ((buf [0x31] == 0x10) && (buf [0x49] == 0x04)) {	// CB2 rather than CT ?
//		buf [0x31] = buf [0x231];
//		buf [0x49] = buf [0x249];
		CB2_mode = 1;
		printf ("Cubieboard2 detected\n");
	}


	version = aw_fel_get_version (handle);					

	SavePad (buf, (char *) "Dump0_000200");

	read_log (buf, 0x2000, (char*) "pt1_000147");		// CRC table ?
  	aw_fel_write			(handle, 0x40100000, buf, 0x2000);			

	PutNulls (buf, 16);
  	aw_fel_write			(handle, 0x7210, buf, 0x10);

	aw_fel_send_file		(handle, 0x40200000, (char*) "fes.fex");
	aw_fel_send_file		(handle, 0x7220,     (char*) "fes_2.fex");
	aw_fel_execute			(handle, 0x7220);			// this puts A20 in flash mode						

	handle = close_usb		(handle);					// have to close here.

	printf ("Re-opening");
  	fflush (stdout);
	for (x = 0; !handle; x++) {
		usleep (1000000);								// wait 1 sec
		printf (".");
	  	fflush (stdout);
		handle = open_usb (x < 10);						// try to open
	}
	printf ("done\n");

	ShowUSBSpeed			(handle);					// 480 Mbps now
	version = aw_fel_get_version (handle);

 	aw_fel2_read			(handle, 0x7e00, buf, 0x100, AW_FEL2_DRAM);
	aw_fel2_write			(handle, 0x7e00, buf, 0x100, AW_FEL2_DRAM);

	aw_fel2_write			(handle, 0x40a00000, buf, GetConfigRec (buf), AW_FEL2_DRAM);

	aw_fel2_send_file		(handle, 0x40360000, AW_FEL2_DRAM, (char*) "magic_de_start.fex");
	aw_fel2_send_file		(handle, 0x40430000, AW_FEL2_DRAM, (char*) "fed_nand.axf");
	aw_fel2_send_file		(handle, 0x40360000, AW_FEL2_DRAM, (char*) "magic_de_end.fex");
	printf ("\r..Wait");								// the next exec takes a while
	fflush (stdout);
	aw_fel2_exec			(handle, 0x40430000, 0x31);
	aw_fel2_send_4uints		(handle, 0x40a00000, 0x40a01000, 0, 0);
	aw_fel2_0203_until_ok	(handle);

	SaveMem (0x40a00000, 0x1000, buf, (char*) "Dump0_000300");	// SYS_PARA as written
	SaveMem (0x40a01000, 0x1000, buf, (char*) "Dump0_000301");	// also from ConfigRec above

	aw_fel2_0204			(handle, 0x0400);			// must pad_read after 0204
	aw_pad_read				(handle, buf, 0x0400);
	memcpy (DRAM_config, buf + 32, 172);            	// will send buf+32 len 172 back

	aw_fel2_send_file		(handle, 0x40600000, AW_FEL2_DRAM, (char*) "boot1_nand.fex");

	aw_fel2_write			(handle, 0x40400000, buf, GetConfigRec (buf), AW_FEL2_DRAM);

	aw_fel2_write			(handle, 0x40410000, DRAM_config, 0x00ac, AW_FEL2_DRAM);

	aw_fel2_send_file		(handle, 0x40360000, AW_FEL2_DRAM, (char*) "magic_de_start.fex");
	aw_fel2_send_file		(handle, 0x40430000, AW_FEL2_DRAM, (char*) "update_boot1.axf");
	aw_fel2_send_file		(handle, 0x40360000, AW_FEL2_DRAM, (char*) "magic_de_end.fex");
	aw_fel2_exec			(handle, 0x40430000, 0x11);

	aw_fel2_send_4uints		(handle, 0x40600000, 0x40400000, 0x40410000, 0);
	aw_fel2_0203_until_ok	(handle);

	SaveMem (0x40400000, 0x1000, buf, (char*) "Dump0_000310");
	SaveMem (0x40410000, 0x1000, buf, (char*) "Dump0_000311");

	aw_fel2_0204			(handle, 0x0400);
	aw_pad_read				(handle, buf, 0x0400);
	printf ("%s\n", &buf [24]);

	aw_fel2_send_file		(handle, 0x40360000, AW_FEL2_DRAM, (char*) "magic_de_start.fex");
	aw_fel2_send_file		(handle, 0x40600000, AW_FEL2_DRAM, (char*) "boot0_nand.bin");
	aw_fel2_send_file		(handle, 0x40360000, AW_FEL2_DRAM, (char*) "magic_de_end.fex");

	aw_fel2_write			(handle, 0x40400000, buf, GetConfigRec (buf), AW_FEL2_DRAM);

	aw_fel2_write			(handle, 0x40410000, DRAM_config, 0x00ac, AW_FEL2_DRAM);

	aw_fel2_send_file		(handle, 0x40360000, AW_FEL2_DRAM, (char*) "magic_de_start.fex");
	aw_fel2_send_file		(handle, 0x40430000, AW_FEL2_DRAM, (char*) "update_boot0.axf");
	aw_fel2_send_file		(handle, 0x40360000, AW_FEL2_DRAM, (char*) "magic_de_end.fex");
	aw_fel2_exec			(handle, 0x40430000, 0x11);

	aw_fel2_send_4uints		(handle, 0x40600000, 0x40400000, 0x40410000, 0);
	aw_fel2_0203_until_ok	(handle);

	aw_fel2_0204			(handle, 0x0400);
	aw_pad_read				(handle, buf, 0x0400);
	printf ("%s\n", &buf [24]);

	aw_fel2_write			(handle, 0x7e04, (char*) "\xcd\xa5\x34\x12", 0x04, AW_FEL2_DRAM);

	SavePad (buf, (char *) "Dump0_000400");

	aw_fel2_send_file		(handle, 0x40360000, AW_FEL2_DRAM, (char*) "magic_de_start.fex");
	aw_fel2_send_file		(handle, 0x40430000, AW_FEL2_DRAM, (char*) "fet_restore.axf");
	aw_fel2_send_file		(handle, 0x40360000, AW_FEL2_DRAM, (char*) "magic_de_end.fex");
	aw_fel2_exec			(handle, 0x40430000, 0x11);

	PutNulls (buf, 16);
	aw_pad_write			(handle, buf, 0x10);

	close_usb				(handle);

	return 0;
}


