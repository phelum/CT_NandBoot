/*
 *      Testing / development extras.
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


int     SaveMem                 (uint addr, uint len, uchar *buf, char *fid)
{
        uint            done, chunk;

    for (done = 0; done < len; done += chunk) {
        chunk = len - done;
        if (chunk > 256)
            chunk = 256;
        if (version == 0x1610)
            aw_fes_read (handle, addr + done, buf + done, chunk, AW_FEL2_DRAM);
        else
            aw_fel_read  (handle, addr + done, buf + done, chunk);
    }

    save_file (fid, buf, len);

    return 0;
}


int     SavePad                 (uchar *buf, char *fid)
{
//  return SaveMem (0x7000, 0x1000, buf, fid);
    return SaveMem (0x0000, 0x8000, buf, fid);
}


int     USBTests                (uchar *buf)
{
        int             x;

//  libmain ();

    handle = open_usb ();

    ShowUSBSpeed            (handle);                   // 12 Mbps here
    version = aw_fel_get_version (handle);

    SavePad (buf, (char *) "Dump0_000000");

    PutNulls (buf, 4);
    aw_fel_write            (handle, 0x7e00, buf, 4);
    aw_fel_get_version      (handle);

    read_log (buf, 0x200, (char*) "pt1_000063");        // DRAM access specs
    aw_fel_write            (handle, 0x7010, buf, 0x200);

    PutNulls (buf, 16);                                 // clear error log
    aw_fel_write            (handle, 0x7210, buf, 0x10);

    aw_fel_send_file        (handle, 0x7220, (char*) "fes_1-1.fex", 2784, 2784);
    aw_fel_execute          (handle, 0x7220);

    usleep (500000);    // need this to avoid error on next USB I/O

    aw_fel_read             (handle, 0x7210, buf, 16);  // expect 'DRAM' then nulls
    hexdump (buf, 0, 16);
    version = aw_fel_get_version (handle);

    SavePad (buf, (char *) "Dump0_000100");

    PutNulls (buf, 16);
    aw_fel_write            (handle, 0x7210, buf, 0x10);

    aw_fel_send_file        (handle, 0x2000, (char*) "fes_1-2.fex");
    aw_fel_execute          (handle, 0x2000);

    aw_fel_read (handle, 0x7210, buf, 16);              // expect 'DRAM', 0x01 then nulls
    hexdump (buf, 0, 16);

    aw_fel_read (handle, 0x7010, buf, 0x200);           // expect as per URB 138
    if ((buf [0x31] == 0x10) && (buf [0x49] == 0x04)) { // CB2 rather than CT ?
//      buf [0x31] = buf [0x231];
//      buf [0x49] = buf [0x249];
        CB2_mode = 1;
        printf ("Cubieboard2 detected\n");
    }

    version = aw_fel_get_version (handle);

    SavePad (buf, (char *) "Dump0_000200");

    read_log (buf, 0x2000, (char*) "pt1_000147");       // CRC table ?
    aw_fel_write            (handle, 0x40100000, buf, 0x2000);

    PutNulls (buf, 16);
    aw_fel_write            (handle, 0x7210, buf, 0x10);

    aw_fel_send_file        (handle, 0x40200000, (char*) "fes.fex");
    aw_fel_send_file        (handle, 0x7220,     (char*) "fes_2.fex");
    aw_fel_execute          (handle, 0x7220);           // this puts A20 in flash mode

    handle = close_usb      (handle);                   // have to close here.

    printf ("Re-opening");
    fflush (stdout);
    for (x = 0; !handle; x++) {
        usleep (1000000);                               // wait 1 sec
        printf (".");
        fflush (stdout);
        handle = open_usb (x < 10);                     // try to open
    }
    printf ("done\n");

    ShowUSBSpeed            (handle);                   // 480 Mbps now
    version = aw_fel_get_version (handle);

    aw_fes_read         (handle, 0x7e00, buf, 0x100, AW_FEL2_DRAM);
    aw_fes_write            (handle, 0x7e00, buf, 0x100, AW_FEL2_DRAM);

    aw_fes_write            (handle, 0x40a00000, buf, GetConfigRec (buf, 0), AW_FEL2_DRAM);

    aw_fes_send_file        (handle, 0x40360000, AW_FEL2_DRAM, (char*) "magic_de_start.fex");
    aw_fes_send_file        (handle, 0x40430000, AW_FEL2_DRAM, (char*) "fed_nand.axf");
    aw_fes_send_file        (handle, 0x40360000, AW_FEL2_DRAM, (char*) "magic_de_end.fex");
    printf ("\r..Wait");                                // the next exec takes a while
    fflush (stdout);
    aw_fes_exec         (handle, 0x40430000, 0x31);
    aw_fes_send_4uints      (handle, 0x40a00000, 0x40a01000, 0, 0);
    aw_fes_0203_until_ok    (handle);

    SaveMem (0x40a00000, 0x1000, buf, (char*) "Dump0_000300");  // SYS_PARA as written
    SaveMem (0x40a01000, 0x1000, buf, (char*) "Dump0_000301");  // also from ConfigRec above

    aw_fes_0204         (handle, 0x0400);           // must pad_read after 0204
    aw_pad_read             (handle, buf, 0x0400);
    memcpy (&NandInfo, buf, sizeof (NandInfo));         // will send buf+32 len 172 back

    aw_fes_send_file        (handle, 0x40600000, AW_FEL2_DRAM, (char*) "boot1_nand.fex");

    aw_fes_write            (handle, 0x40400000, buf, GetConfigRec (buf, 0), AW_FEL2_DRAM);

    aw_fes_write            (handle, 0x40410000, ((char*) &NandInfo) + 32, 0x00ac, AW_FEL2_DRAM);

    aw_fes_send_file        (handle, 0x40360000, AW_FEL2_DRAM, (char*) "magic_de_start.fex");
    aw_fes_send_file        (handle, 0x40430000, AW_FEL2_DRAM, (char*) "update_boot1.axf");
    aw_fes_send_file        (handle, 0x40360000, AW_FEL2_DRAM, (char*) "magic_de_end.fex");
    aw_fes_exec         (handle, 0x40430000, 0x11);

    aw_fes_send_4uints      (handle, 0x40600000, 0x40400000, 0x40410000, 0);
    aw_fes_0203_until_ok    (handle);

    SaveMem (0x40400000, 0x1000, buf, (char*) "Dump0_000310");
    SaveMem (0x40410000, 0x1000, buf, (char*) "Dump0_000311");

    aw_fes_0204         (handle, 0x0400);
    aw_pad_read             (handle, buf, 0x0400);
    printf ("%s\n", &buf [24]);

    aw_fes_send_file        (handle, 0x40360000, AW_FEL2_DRAM, (char*) "magic_de_start.fex");
    aw_fes_send_file        (handle, 0x40600000, AW_FEL2_DRAM, (char*) "boot0_nand.bin");
    aw_fes_send_file        (handle, 0x40360000, AW_FEL2_DRAM, (char*) "magic_de_end.fex");

    aw_fes_write            (handle, 0x40400000, buf, GetConfigRec (buf, 0), AW_FEL2_DRAM);

    aw_fes_write            (handle, 0x40410000, ((char*) &NandInfo) + 32, 0x00ac, AW_FEL2_DRAM);

    aw_fes_send_file        (handle, 0x40360000, AW_FEL2_DRAM, (char*) "magic_de_start.fex");
    aw_fes_send_file        (handle, 0x40430000, AW_FEL2_DRAM, (char*) "update_boot0.axf");
    aw_fes_send_file        (handle, 0x40360000, AW_FEL2_DRAM, (char*) "magic_de_end.fex");
    aw_fes_exec         (handle, 0x40430000, 0x11);

    aw_fes_send_4uints      (handle, 0x40600000, 0x40400000, 0x40410000, 0);
    aw_fes_0203_until_ok    (handle);

    aw_fes_0204         (handle, 0x0400);
    aw_pad_read             (handle, buf, 0x0400);
    printf ("%s\n", &buf [24]);

    aw_fes_write            (handle, 0x7e04, (char*) "\xcd\xa5\x34\x12", 0x04, AW_FEL2_DRAM);

    SavePad (buf, (char *) "Dump0_000400");

    aw_fes_send_file        (handle, 0x40360000, AW_FEL2_DRAM, (char*) "magic_de_start.fex");
    aw_fes_send_file        (handle, 0x40430000, AW_FEL2_DRAM, (char*) "fet_restore.axf");
    aw_fes_send_file        (handle, 0x40360000, AW_FEL2_DRAM, (char*) "magic_de_end.fex");
    aw_fes_exec         (handle, 0x40430000, 0x11);

    PutNulls (buf, 16);
    aw_pad_write            (handle, buf, 0x10);

    close_usb               (handle);

    return 0;
}


int     MakeModUBoot            (uchar *buf, char *fid)
{
        FILE    *tmp, *in;
        int     x;

    if (NULL == (in = fopen (fid, "rb"))) {
        printf ("%s open failed, fatal error\n", fid);
        exit (2);
    }
    tmp = fopen ((char*) "/tmp/uboot", "w+b");          // byte e0 is 00 in orig, 10 in ours
    fread (buf, 1, 256, in);
    buf [0xE0] = 0x10;                                  // 0x10 = fes mode
    fwrite (buf, 1, 256, tmp);
    while (0 < (x = fread (buf, 1, 65536, in)))
        fwrite (buf, 1, x, tmp);
    fclose (in);
    fclose (tmp);

    return 0;
}


int     H3_Tests                (uchar *buf)
{
        int             x;

    handle = open_usb ();

    version = aw_fel_get_version (handle);
    if ( ! IsH3 (version)) {
        printf ("This test is for an H3 SoC only.\n");
        return 1;
    }
    ShowUSBSpeed            (handle);                   // 12 Mbps here

    aw_fel_read             (handle, 0x7e00, buf, 256); // expect all 0xCC

    PutNulls (buf, 4);
    aw_fel_write            (handle, 0x7e00, buf, 256);
    aw_fel_get_version      (handle);

    install_fes_1_1 (handle, buf);

    PutNulls (buf, 16);
    aw_fel_write            (handle, 0x7210, buf, 0x10);

    ShowURB (62);
//  aw_fel_send_file        (handle, 0x2000, (char*) "H3_FES_1-0");
    aw_fel_send_file        (handle, 0x2000, (char*) "fes1_sun8iw7p1.bin+mod");
    ShowURB (78);
    aw_fel_execute          (handle, 0x2000);

    aw_fel_read (handle, 0x7210, buf, 136);             // expect 'DRAM', 0x01 then nulls
    read_log (buf+256, 136, (char*) "H3_pt1_000078");
    if (memcmp (buf, buf+256, 136)) {
        printf ("pt1_000078 error\n");
        hexdump (buf, 0, 136);
    }

//  MakeModUBoot (buf, (char*) "H3_UBOOT");
    MakeModUBoot (buf, (char*) "u-boot-sun8iw7p1.bin+mod");
//  MakeModUBoot (buf, (char*) "u-boot-sun7i.bin");
    ShowURB (84);
    aw_fel_send_file        (handle, 0x4A000000, (char*) "/tmp/uboot");
    aw_fel_execute          (handle, 0x4A000000);

    handle = close_usb      (handle);                   // have to close here.

//      line 61532  154661ms

reopen:
    printf ("Re-opening...");
    fflush (stdout);
    for (x = 0; !handle; x++) {
        usleep (1000000);                               // wait 1 sec
        printf (".");
        fflush (stdout);
        handle = open_usb (x < 10);                     // try to open
    }
    printf ("done\n");

    AW_USB_FEL_BULK_EP_OUT=0x02;        // have to swap endpoints here
    AW_USB_FEL_BULK_EP_IN=0x81;

    if (3 != ShowUSBSpeed (handle)) {                  // 480 Mbps now
        printf ("Oops, expected 480Mbps\n");
        handle = close_usb (handle);
        usleep (1000000);                               // wait 1 sec
        goto reopen;
    }

    version = aw_fel_get_version (handle);

    ShowURB (24);
    aw_fes_read         (handle, 0x7e00, buf, 0x100, AW_FEL2_DRAM);

    version = aw_fel_get_version (handle);

    ShowURB (45);
    aw_fes_write        (handle, 0x7e00, buf, 0x100, AW_FEL2_DRAM);

    ShowURB (50);
    PutNulls (buf, 16);
    buf [0] = 0x01;                                                 // 1 = erase
    aw_fex_send_buf     (handle, buf, 0x10, 0x00, 0x007F04, 1);     // load erase tag

    ShowURB (56);
    aw_fex_send_file    (handle, (char*) "H3_MBR", 0, 0x007F01, 1); // 7f01 = MBR

    ShowURB (62);
    aw_fex_0209         (handle, 0x00, 0x00, 0x00, buf);            // query storage
    hexdump (buf, 0, 4);                                            // expect 02 00 00 00 (2 = nand)

    ShowURB (65);
    aw_fex_020A         (handle);                                   // flash on
    printf ("Flash on\n");

    ShowURB (67);
    aw_fex_send_file    (handle, (char*) "H3_BOOTLOADER", 0x008000, 0x00, 1);

    ShowURB (445);
    aw_fex_send_file    (handle, (char*) "H3_ENV",        0x010000, 0x00, 1);

    ShowURB (454);
    aw_fex_send_file    (handle, (char*) "H3_BOOT",       0x018000, 0x00, 1);

    ShowURB (1006);
    aw_fex_send_file    (handle, (char*) "H3_SYSTEM",     0x020000, 0x00, 2);

#if 0
//
//      These download chunks are at the end of H3_SYSTEM but I can't see
//      how the output disk keys are specified.
//
    ShowURB (16660);
    read_log            (buf, 0x2000, (char*) "H3_pt2_016661");
    aw_fex_send_buf     (handle, buf, 0x2000,             0x0AAEC8, 0x00, 0);

    ShowURB (16663);
    read_log            (buf, 0x1000, (char*) "H3_pt2_016664");
    aw_fex_send_buf     (handle, buf, 0x1000,             0x0E0000, 0x00, 0);

    ShowURB (16666);
    read_log            (buf, 0x1000, (char*) "H3_pt2_016667");
    aw_fex_send_buf     (handle, buf, 0x1000,             0x0E0008, 0x00, 0);

    ShowURB (16669);
    read_log            (buf, 0x2000, (char*) "H3_pt2_016670");
    aw_fex_send_buf     (handle, buf, 0x2000,             0x0E0188, 0x00, 0);

    ShowURB (16672);
    read_log            (buf, 0x2000, (char*) "H3_pt2_016673");
    aw_fex_send_buf     (handle, buf, 0x2000,             0x120000, 0x00, 0);

    ShowURB (16675);
    read_log            (buf, 0x1000, (char*) "H3_pt2_016676");
    aw_fex_send_buf     (handle, buf, 0x1000,             0x160000, 0x00, 0);

    ShowURB (16678);
    read_log            (buf, 0x1000, (char*) "H3_pt2_016679");
    aw_fex_send_buf     (handle, buf, 0x1000,             0x160008, 0x00, 0);

    ShowURB (16681);
    read_log            (buf, 0x2000, (char*) "H3_pt2_016682");
    aw_fex_send_buf     (handle, buf, 0x2000,             0x160188, 0x00, 0);
#endif

    ShowURB (16684);
    aw_fex_send_file    (handle, (char*) "H3_RECOVERY",   0x1A8000, 0x00, 1);

    ShowURB (17278);
    aw_fex_send_file    (handle, (char*) "H3_SYSRECOVERY",0x1B8000, 0x00, 0);

    aw_fex_send_file    (handle, (char*) "H3_DISKFS",     0x4A8000, 0x00, 1);

    ShowURB (32017);
    aw_fex_020B         (handle);                       // flash off
    printf ("Flash off\n");

    ShowURB (32019);
    aw_fex_0230         (handle, 0x00, 0x00, 0x00, buf);        // query secure
    hexdump (buf, 0, 4);                                // expect 00 00 00 00

    ShowURB (32022);
    aw_fex_send_file    (handle, (char*) "H3_UBOOT",      0x000000, 0x007F02, 1);   // BOOT 1
//  aw_fex_send_file    (handle, (char*) "u-boot-sun8iw7p1.bin+mod", 0x000000, 0x007F02, 1);   // BOOT 1

    ShowURB (32067);
    aw_fex_0230         (handle, 0x00, 0x00, 0x00, buf);        // query secure
    hexdump (buf, 0, 4);                                // expect 00 00 00 00

    ShowURB (32070);
    aw_fex_0209         (handle, 0x00, 0x00, 0x00, buf);        // query storage
    hexdump (buf, 0, 4);                                // expect 02 00 00 00

    ShowURB (32073);
//  aw_fex_send_file    (handle, (char*) "H3_BOOT_0",     0x000000, 0x007F03, 1);   // BOOT 0   bad
//  aw_fex_send_file    (handle, (char*) "H3_12BOOT_0",   0x000000, 0x007F03, 1);   // BOOT 0   good
//  aw_fex_send_file    (handle, (char*) "boot0_nand_sun8iw7p1.bin+mod", 0x000000, 0x007F03, 1);   // BOOT 0
    aw_fex_send_file    (handle, (char*) "test_boot0",    0x000000, 0x007F03, 1);   // BOOT 0

    ShowURB (32079);
    aw_fex_020F         (handle, 0x08, 0, 0);

    close_usb           (handle);

    return 0;
}



int     A20_Tests                (uchar *buf)
{
        int             x, len;

    handle = open_usb ();

    ShowUSBSpeed            (handle);                   // 12 Mbps here
    version = aw_fel_get_version (handle);

    aw_fel_read             (handle, 0x7e00, buf, 256); // expect all 0xCC

//  version = aw_fel_get_version (handle);

    PutNulls (buf, 4);
    aw_fel_write            (handle, 0x7e00, buf, 256);
//  aw_fel_get_version      (handle);

    read_log (buf, 0x200, FN->DRAM_specs);               // DRAM access specs
    len = 0x200;
    aw_fel_write (handle, 0x7010, buf, len);
    PutNulls (buf, 16);
    aw_fel_write            (handle, 0x7210, buf, 0x10);


    aw_fel_send_file (handle, 0x2000, FN->fes_1_2);

    ShowURB (120);
    aw_fel_execute (handle, 0x2000);

    ShowURB (129);
    len = 16;
    aw_fel_read (handle, 0x7210, buf, len);             // expect 'DRAM', 0x01 then nulls

    if (memcmp (buf, "DRAM\x01\x00\x00\x00", 8)) {
        printf ("DRAM1 error\n");
        hexdump (buf, 0, len);
    }

    aw_fel_read (handle, 0x7010, buf, 0x200);           // expect as per URB 138
    memcpy (&DramInfo, buf, sizeof (DramInfo));         // save DRAM details.

    ShowURB (84);
//  MakeModUBoot (buf, (char*) "H3_UBOOT");
    MakeModUBoot (buf, (char*) "relic/u-boot-sun7i.bin");
    aw_fel_send_file        (handle, 0x4A000000, (char*) "/tmp/uboot");
    aw_fel_execute          (handle, 0x4A000000);           // this fails !!!!!!!

//  usleep (500000);    // help ?

    handle = close_usb      (handle);                   // have to close here.

//      line 61532  154661ms

    printf ("Re-opening...");
    fflush (stdout);
    for (x = 0; !handle; x++) {
        usleep (1000000);                               // wait 1 sec
        printf (".");
        fflush (stdout);
        handle = open_usb (x < 10);                     // try to open
    }
    printf ("done\n");

    if (3 != ShowUSBSpeed(handle)) {                    // 480 Mbps now
        printf ("Oops, expected 480Mbps\n");
        goto close;
    }

    AW_USB_FEL_BULK_EP_OUT=0x02;        // have to swap endpoints here
    AW_USB_FEL_BULK_EP_IN=0x81;

    version = aw_fel_get_version (handle);

    ShowURB (24);
    aw_fes_read     (handle, 0x7e00, buf, 0x100, AW_FEL2_DRAM);

    version = aw_fel_get_version (handle);

    ShowURB (45);
    aw_fes_write        (handle, 0x7e00, buf, 0x100, AW_FEL2_DRAM);

        char            cKeyChar;

    while (1)
    {
        printf ("Waiting key ...");
        fflush (stdout);
        cKeyChar = GetKey ();
        printf ("%c\n", cKeyChar);
        if (cKeyChar == 0x1B)
            break;
        if  (cKeyChar == 'a') {  
            aw_fex_0209         (handle, 0x00, 0x00, 0x00, buf);        // query storage
            hexdump (buf, 0, 4);                                // expect 02 00 00 00 (2 = nand)
            continue;
        }
        if  (cKeyChar == 'b') {  
            aw_fex_0230         (handle, 0x00, 0x00, 0x00, buf);        // query secure
            hexdump (buf, 0, 4);                                // expect 00 00 00 00
            continue;
        }
        if  (cKeyChar == 'c') {  
            aw_fex_020A         (handle);                       // flash on
            printf ("Flash on\n");
            continue;
        }
        if  (cKeyChar == 'd') {  
            aw_fex_020B         (handle);                       // flash off
            printf ("Flash off\n");
            continue;
        }

    }

#if 0

    ShowURB (50);
    PutNulls (buf, 16);
    buf [0] = 0x01;
    aw_fex_send_buf     (handle, buf, 0x10, 0x00, 0x007F04, 1);

    ShowURB (56);
    aw_fex_send_file    (handle, (char*) "H3_MBR", 0, 0x007F01, 1); // 7f01 = MBR

    ShowURB (62);
    aw_fex_0209         (handle, 0x00, 0x00, 0x00, buf);        // query storage
    hexdump (buf, 0, 4);                                // expect 02 00 00 00 (2 = nand)

    ShowURB (65);
    aw_fex_020A         (handle);                       // flash on
    printf ("Flash on\n");

    ShowURB (67);
    aw_fex_send_file    (handle, (char*) "H3_BOOTLOADER", 0x008000, 0x00, 1);

    ShowURB (445);
    aw_fex_send_file    (handle, (char*) "H3_ENV",        0x010000, 0x00, 1);

    ShowURB (454);
    aw_fex_send_file    (handle, (char*) "H3_BOOT",       0x018000, 0x00, 1);

    ShowURB (1006);
    aw_fex_send_file    (handle, (char*) "H3_SYSTEM",     0x020000, 0x00, 2);

    ShowURB (16684);
    aw_fex_send_file    (handle, (char*) "H3_RECOVERY",   0x1A8000, 0x00, 1);

    ShowURB (17278);
    aw_fex_send_file    (handle, (char*) "H3_SYSRECOVERY",0x1B8000, 0x00, 0);

    aw_fex_send_file    (handle, (char*) "H3_DISKFS",     0x4A8000, 0x00, 1);

    ShowURB (32017);
    aw_fex_020B         (handle);                       // flash off
    printf ("Flash off\n");

    ShowURB (32019);
    aw_fex_0230         (handle, 0x00, 0x00, 0x00, buf);        // query secure
    hexdump (buf, 0, 4);                                // expect 00 00 00 00

    ShowURB (32022);
    aw_fex_send_file    (handle, (char*) "H3_UBOOT",      0x000000, 0x007F02, 1);   // BOOT 1

    ShowURB (32067);
    aw_fex_0230         (handle, 0x00, 0x00, 0x00, buf);        // query secure
    hexdump (buf, 0, 4);                                // expect 00 00 00 00

    ShowURB (32070);
    aw_fex_0209         (handle, 0x00, 0x00, 0x00, buf);        // query storage
    hexdump (buf, 0, 4);                                // expect 02 00 00 00

    ShowURB (32073);
//  aw_fex_send_file    (handle, (char*) "H3_BOOT_0",     0x000000, 0x007F03, 1);   // BOOT 0
    aw_fex_send_file    (handle, (char*) "H3_12BOOT_0",   0x000000, 0x007F03, 1);   // BOOT 0

    ShowURB (32079);
    aw_fex_020F         (handle, 0x08, 0, 0);
#endif

close:
    close_usb               (handle);

    return 0;
}


int     Lime_Tests              (uchar *buf)
{
        int             x;

    handle = open_usb ();

    ShowUSBSpeed            (handle);                   // 12 Mbps here
    version = aw_fel_get_version (handle);
    version = aw_fel_get_version (handle);
    version = aw_fel_get_version (handle);

    aw_fel_read             (handle, 0x7e00, buf, 256); // expect all 0xCC

    version = aw_fel_get_version (handle);

    PutNulls (buf, 4);
    aw_fel_write            (handle, 0x7e00, buf, 256);
    aw_fel_get_version      (handle);

    PutNulls (buf, 16);
    aw_fel_write            (handle, 0x7210, buf, 0x10);

    install_fes_1_1 (handle, buf);

    install_fes_1_2 (handle, buf);


        FILE    *tmp, *in;

    in = fopen  ((char*) "H3_UBOOT", "rb");
    tmp = fopen ((char*) "/tmp/uboot", "w+b");          // byte e0 is 00 in orig, 10 in ours
    fread (buf, 1, 256, in);
    buf [0xE0] = 0x10;                                  // 0x10 = fes mode
    fwrite (buf, 1, 256, tmp);
    while (0 < (x = fread (buf, 1, 65536, in)))
        fwrite (buf, 1, x, tmp);
    fclose (in);
    fclose (tmp);


    ShowURB (84);
    aw_fel_send_file        (handle, 0x4A000000, (char*) "/tmp/uboot");
    aw_fel_execute          (handle, 0x4A000000);

    handle = close_usb      (handle);                   // have to close here.

    printf ("Re-opening...");
    fflush (stdout);
    for (x = 0; !handle; x++) {
        usleep (1000000);                               // wait 1 sec
        printf (".");
        fflush (stdout);
        handle = open_usb (x < 10);                     // try to open
    }
    printf ("done\n");

    AW_USB_FEL_BULK_EP_OUT=0x02;        // have to swap endpoints here
    AW_USB_FEL_BULK_EP_IN=0x81;

    ShowUSBSpeed        (handle);                       // 480 Mbps now

    version = aw_fel_get_version (handle);

    version = aw_fel_get_version (handle);

    ShowURB (24);
    aw_fes_read     (handle, 0x7e00, buf, 0x100, AW_FEL2_DRAM);

    version = aw_fel_get_version (handle);

    ShowURB (45);
    aw_fes_write        (handle, 0x7e00, buf, 0x100, AW_FEL2_DRAM);

    ShowURB (50);
    PutNulls (buf, 16);
    buf [0] = 0x01;
    aw_fex_send_buf     (handle, buf, 0x10, 0x00, 0x007F04, 1);

    ShowURB (56);
    aw_fex_send_file    (handle, (char*) "/mnt/sda3/Nand/lime/nand_mbr", 0, 0x007F01, 1);   // 7f01 = MBR

    ShowURB (62);
    aw_fex_0209         (handle, 0x00, 0x00, 0x00, buf);        // query storage
    hexdump (buf, 0, 4);                                // expect 02 00 00 00

    ShowURB (65);
    aw_fex_020A         (handle);                       // flash on
    printf ("Flash on\n");

    ShowURB (67);
    aw_fex_send_file    (handle, (char*) "/mnt/sda3/Nand/lime/bootloader", 0x000100, 0x00, 1);
    aw_fex_send_file    (handle, (char*) "/mnt/sda3/Nand/lime/boot",       0x000900, 0x00, 1);
    aw_fex_send_file    (handle, (char*) "/mnt/sda3/Nand/lime/CloudFS",    0x000B00, 0x00, 1);

    ShowURB (32017);
    aw_fex_020B         (handle);                       // flash off
    printf ("Flash off\n");

    ShowURB (32019);
    aw_fex_0230         (handle, 0x00, 0x00, 0x00, buf);        // query secure
    hexdump (buf, 0, 4);                                // expect 00 00 00 00

    ShowURB (32022);
    aw_fex_send_file    (handle, (char*) "A20_boot1_nand.fex",      0x000000, 0x007F02, 1); // BOOT 1

    ShowURB (32067);
    aw_fex_0230         (handle, 0x00, 0x00, 0x00, buf);        // query secure
    hexdump (buf, 0, 4);                                // expect 00 00 00 00

    ShowURB (32070);
    aw_fex_0209         (handle, 0x00, 0x00, 0x00, buf);        // query storage
    hexdump (buf, 0, 4);                                // expect 02 00 00 00

    ShowURB (32073);
    aw_fex_send_file    (handle, (char*) "A20_boot0_nand.bin",     0x000000, 0x007F03, 1);  // BOOT 0

    ShowURB (32079);
    aw_fex_020F         (handle, 0x08, 0, 0);

    close_usb               (handle);

    return 0;
}



