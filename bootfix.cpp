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
 * Booting form NAND requires more files in the boot partition than when
 * booting from SD card.  For consistency, these extra files can be put
 * in the SD card partition and will be ignored.
 *
 * The required files are:
 *   /boot.axf
 *   /boot.ini
 *   /uEnv.txt
 *   /script.bin
 *   /uImage
 *   /linux/linux.ini
 *   /linux/u-boot.bin
 *
 * uEnv.txt, script.bin, and uImage vary to suit the board and kernel.
 *
 * /linux/u-boot.bin is specific for the board type.
 * CubieTruck needs u-boot.bin size 309340
 * CubieBoard2 needs u-boot.bin size 302280
 *
 * With SD card boot, the bootloader is stored at sector 16 onwards.  This
 * bootloader uses uEnv.txt, script.bin, and uImage only.
 *
 * With NAND boot, the bootloader runs boot.axf which uses boot.ini then
 * /linux/linux.ini then runs /linux/u-boot.bin.  This u-boot.bin uses
 * uEnv.txt, script.bin, and uImage.
 *
 * A skeleton boot partition is included.  The relevant u-boot.* file
 * must be renamed to u-boot.bin.  The script.bin and uImage files must
 * be added.  The uEnv.txt file will probably require customising.
 *
 * u-boot.bin reads uEnv.txt which contains environment updating commands.
 * Some versions of u-boot.bin are modified to read uEnv.cb2 if the DRAM
 * size is < 2GB, otherwise they look for uEnv.ct.
 *
 * By default, u-boot.bin then reads script.bin to addr 43000000, then
 * uImage to addr 48000000, then passes control to uImage (the kernel).
 *
 * uEnv.txt can also contain U-boot commands and these will be executed
 * when the file is being processed.  This makes it possible to override
 * the normal boot process at this stage.
 *
 *
 *          A10 notes
 *
 * The A10 boot1 (U-boot) requires the boot partition to be nand1 and vfat,
 * the second partition must contain the boot1 configuration, and that the
 * boot partition size matches the value specified in the _SYS_CONFIG file.
 *
 * When running in A10 mode many of the files are different.  The "A10"
 * version of the files have names starting with "A10" instead of "A20".
 *
 * - Steven Saunderson (check <http://phelum.net/> for contact details).
 */

//#define OLD_EXTRAS

#include <types.h>
#include <unistd.h>
#include "usbfel.inc"
#include <sys/termios.h>

#include <limits.h>
#include <stdlib.h>

#include "bootfix.h"
#include "nand_part.inc"


/*void*/FullName::FullName (char *name)
  {
    szName = name;
    szFullName = NULL;

    return;
  }


/*void*/FullName::~FullName (void)
  {
    if (szFullName)
        free (szFullName);
    szFullName = NULL;

    return;
  }


char*   FullName::get   (void)
  {
    if (szFullName == NULL)
        szFullName = (char*) malloc (1024);
    strcpy (szFullName, szBasePath);
    strcat (szFullName, szName);
    
    return szFullName;
  }


char    GetKey      (void)
{
    char    cKeycode = 0;
    struct termios  termios, termios_save;

    tcgetattr (STDIN_FILENO, &termios);
    termios_save = termios;
    termios.c_lflag &= ~(ICANON | ECHO);                // special mode
    tcsetattr (STDIN_FILENO, TCSANOW, &termios);

    cKeycode = tolower (getchar ());

//  printf ("%c\n", cKeycode);

    tcsetattr (STDIN_FILENO, TCSANOW, &termios_save);   // normal mode

    return cKeycode;
}


char    GetYesNo    (char *msg)
{
    char    cKeycode = 0;
    struct termios  termios, termios_save;

    tcgetattr (STDIN_FILENO, &termios);
    termios_save = termios;
    termios.c_lflag &= ~(ICANON | ECHO);                // special mode
    tcsetattr (STDIN_FILENO, TCSANOW, &termios);

    printf ("%s [y/n] ? ", msg);

    while (cKeycode != 'n' && cKeycode != 'y')
        cKeycode = tolower (getchar ());

    printf ("%c\n", cKeycode);

    tcsetattr (STDIN_FILENO, TCSANOW, &termios_save);   // normal mode

    return cKeycode;
}


int     PerhapsQuit     (void)
{
    if (!forceable)
        exit (1);

    if (GetYesNo ((char *) "Serious error, quit program") != 'n')
        exit (1);

    errors++;

    return 0;
}


int     DebugHalt       (void)
{
    if (GetYesNo ((char *) "Halt; y = continue, n = quit program") == 'n')
        exit (1);

    errors++;

    return 0;
}


int     GetUSBSpeed             (libusb_device_handle *handle)
{
    libusb_device       *pUSB_dev;

    pUSB_dev = libusb_get_device (handle);

    return libusb_get_device_speed (pUSB_dev);
}


int     ShowUSBSpeed            (libusb_device_handle *handle)
{
    int     speed = GetUSBSpeed (handle);
    char    *speeds [] = {(char*) "unknown",        // 0
                          (char*) "1.5 Mbps",       // 1
                          (char*) "12 Mbps",        // 2
                          (char*) "480 Mbps",       // 3
                          (char*) "5000 Mbps"};     // 4

    printf ("Speed = %s\n", speeds [speed]);

    return speed;
}


libusb_device_handle*   open_usb    (int bFailAllowed = 0)
{
    while (0 == (handle = libusb_open_device_with_vid_pid (NULL, 0x1f3a, 0xefe8)))
      {
        if (bFailAllowed)
            return 0;

        switch (errno)
          {
            case EACCES:
                fprintf (stderr, "ERROR: Allwinner USB FEL device access denied\n");
                break;
            default:
                fprintf (stderr, "ERROR: Allwinner USB FEL device not found\n");
                break;
          }
        if (GetYesNo ((char *) "Retry USB device access") != 'y')
            exit (1);
      }

    rc = libusb_claim_interface (handle, 0);            // claim interface

#if defined(__linux__)
    if (rc != LIBUSB_SUCCESS)                           // if claim rejected,
      {
        libusb_detach_kernel_driver (handle, 0);        // tell kernel to release
        detached_iface = 0;                             // > -1 = restore later
        rc = libusb_claim_interface (handle, 0);        // try to claim again
      }
#endif

    assert (rc == 0);                                   // die if error

    return handle;
}


libusb_device_handle*   close_usb   (libusb_device_handle* handle)
{
    libusb_close (handle);

#if defined(__linux__)
    if (detached_iface >= 0)                            // detach requested ?
      {
        libusb_attach_kernel_driver (handle, 0);        // restore status quo
        detached_iface = -1;
      }
#endif

    return NULL;
}


void    ShowURB         (int urb)
{
    if (bShowURBs)
        printf ("URB %d\n", urb);

    return;
}


int read_log (void *dest, int bytes, char *name)
{
    FILE  *fin;
    uchar *out = (uchar*) dest;
    char  line [1024];
    char  *in = line;
    int   nbyte;

    if (NULL == (fin = fopen (name, "rb")))
      {
        perror ("Failed to open input file: ");
        exit (1);
      }

    for (*in = 0x00; bytes > 0; in++)
      {
        if (*in == 0x00)
          {
            if (NULL == fgets (line, 1024, fin))
              {
                perror ("Early EOF on file: ");
                exit (1);
              }

            in = strchr (line, ':');    // might have offset at start
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


int IsA10               (int version)
{
    return (version == 0x1623);
}


int IsA20               (int version)
{
    return (version == 0x1651);
}


int IsH3                (int version)
{
    return (version == 0x1680);
}


int stage_1_prep (libusb_device_handle *handle, uchar *buf)
{
    int  x;

    ShowURB (5);
    version = aw_fel_get_version (handle);

    if (IsA10 (version))                            // 0x1623 = A10
      {
        printf ("Detected A10 (ID 0x%04X)\n", version);
        FN = &FN_sun4i;
      }
    else if (IsA20 (version))                       // 0x1651 = A20
      {
        printf ("Detected A20 (ID 0x%04X)\n", version);
        FN = &FN_sun7i;
      }
    else if (IsH3 (version))                        // 0x1680 = H3
        printf ("Detected H3 (ID 0x%04X)\n", version);
    else
      {
        printf ("Unknown processor ID 0x%04X\n", version);
        PerhapsQuit ();
      }

    ShowURB (14);
    version = aw_fel_get_version (handle);

            // URB 23 - 27

    ShowURB (23);
    aw_fel_read (handle, 0x7e00, buf, 256);         // get 256 0xCC

    for (x = 0; x < 256; x++)
      {
        if (buf [x] != 0xCC)
          {
            printf ("Non 0xCC at entry %d\n", x);
            hexdump (buf, 0, 256);
            save_file ((char *) "Dump1_000023", buf, 256);
            PerhapsQuit ();
            break;
          }
      }

    ShowURB (32);
    version = aw_fel_get_version (handle);

    ShowURB (41);
    PutNulls (buf, 4);
    aw_fel_write (handle, 0x7e00, buf, 256);

    ShowURB (50);
    version = aw_fel_get_version (handle);

    return 0;
}


int     install_fes_1_1 (libusb_device_handle *handle, uchar *buf)
{
    rAWSysPar       *pDI;
    int             len = 0x200;

    ShowURB (63);

    PutNulls (buf, 512);
    pDI = (rAWSysPar*) buf;
    pDI->unk_6 = 0x007C4AC1;                            // UART 0 TX pin specs
    if (IsA10 (version))                                // A10 defaults
      {
        pDI->unk_0           = 0x02000000;                  // chip
        pDI->unk_1           = 0x02000000;                  // pid
        pDI->unk_2           = 0x02000100;                  // sid
        pDI->unk_3           = 0x80;                        // bid
//      pDI->dram_baseaddr   = 0x40000000;                  // from Ronald T.
//      pDI->dram_clk        = 480;
//      pDI->dram_type       = 3;
//      pDI->dram_rank_num   = 1;
//      pDI->dram_chip_density = 4096;
//      pDI->dram_io_width   = 16;
//      pDI->dram_bus_width  = 32;
//      pDI->dram_cas        = 6;
//      pDI->dram_zq         = 0x7b;
//      pDI->dram_odt_en     = 0;
//      pDI->dram_size       = 1024;
//      pDI->dram_tpr0       = 0x30926692;
//      pDI->dram_tpr1       = 0x1090;
//      pDI->dram_tpr2       = 0x1a0c8;
//      pDI->dram_tpr3       = 0x0;
        pDI->dram_tpr4       = 0x40000000;                  // from USB trace.
        pDI->dram_tpr5       = 480;                         // all values here
        pDI->dram_emr1       = 3;                           // are 60 bytes after
        pDI->dram_emr2       = 1;                           // expected location.
        pDI->dram_emr3       = 4096;
        pDI->dram_spare [0]  = 16;
        pDI->dram_spare [1]  = 32;
        pDI->dram_spare [2]  = 6;
        pDI->dram_spare [3]  = 0x7B;
        pDI->dram_spare [4]  = 0;
        pDI->dram_spare [5]  = 1024;
        pDI->dram_spare [6]  = 0x30926692;
        pDI->dram_spare [7]  = 0x1090;
        pDI->dram_spare [8]  = 0x01A0C8;
        pDI->dram_spare [9]  = 0;
//      read_log (buf, 0xB4, FN->DRAM_specs);              // DRAM access specs
        len = 0xB4;
      }

    if (IsA20 (version))                                // A20 defaults
      {
        pDI->dram_baseaddr   = 0x40000000;                  // from CT script.fex
        pDI->dram_clk        = 432;
        pDI->dram_type       = 3;
        pDI->dram_rank_num   = 0xFFFFFFFF;
        pDI->dram_chip_density = 0xFFFFFFFF;
        pDI->dram_io_width   = 0xFFFFFFFF;
        pDI->dram_bus_width  = 0xFFFFFFFF;
        pDI->dram_cas        = 9;
        pDI->dram_zq         = 0x7f;
        pDI->dram_odt_en     = 0;
        pDI->dram_size       = 0xFFFFFFFF;
        pDI->dram_tpr0       = 0x42d899b7;
        pDI->dram_tpr1       = 0xa090;
        pDI->dram_tpr2       = 0x22a00;
        pDI->dram_tpr3       = 0x0;
        pDI->dram_tpr4       = 0x1;
        pDI->dram_tpr5       = 0x0;
        pDI->dram_emr1       = 0x4;
        pDI->dram_emr2       = 0x10;
        pDI->dram_emr3       = 0x0;
//      read_log (buf, 0x200, FN->DRAM_specs);               // DRAM access specs
        len = 0x200;
      }      

    if (IsH3 (version))                                 // H3 defaults
      {
        pDI->dram_baseaddr   = 0x40000000;                  // from H3 script.fex
//      pDI->dram_clk        = 672;
        pDI->dram_clk        = 480;
        pDI->dram_type       = 3;
        pDI->dram_zq         = 0x3b3bfb;
        pDI->dram_odt_en     = 0x1;
//      pDI->dram_para1      = 0x10E40000;
//      pDI->dram_para2      = 0x0000;
//      pDI->dram_mr0        = 0x1840;
        pDI->dram_emr1       = 0x40;
        pDI->dram_emr2       = 0x18;
        pDI->dram_emr3       = 0x2;
        pDI->dram_tpr0       = 0x0048A192;
        pDI->dram_tpr1       = 0x01C2418D;
        pDI->dram_tpr2       = 0x00076051;
        pDI->dram_tpr3       = 0;
        pDI->dram_tpr4       = 0;
        pDI->dram_tpr5       = 0;
//      pDI->dram_tpr6       = 100;
//      pDI->dram_tpr7       = 0;
//      pDI->dram_tpr8       = 0;
//      pDI->dram_tpr9       = 0;
//      pDI->dram_tpr10      = 0;
//      pDI->dram_tpr11      = 0x6aaa0000;
//      pDI->dram_tpr12      = 0x7979;
//      pDI->dram_tpr13      = 0x800800;
      }

    aw_fel_write (handle, 0x7010, buf, len);

    ShowURB (72);
    PutNulls (buf, 16);                                 // clear error log
    aw_fel_write (handle, 0x7210, buf, 0x10);

    ShowURB (77);

#ifdef OLD_EXTRAS
//          Load buffer as per URB 81 (0xae0 = 2784) FES_1-1 with nulls after.
//          We do lots of sanity checks here (relic from testing).
    PutNulls (buf, 65536);
    read_log (buf, 0x0ae0, (char*) "pt1_000081");       // data from log

    FILE *fin;

    if (NULL == (fin = fopen (FN->fes_1_1, "rb")))
      {
        perror ("Failed to open file to send: ");
        exit (1);
      }

    fread (buf + 2784, 1, 2784, fin);                   // data from file
    fclose (fin);

    if (memcmp (buf, buf + 2784, 2784))                 // shouldn't happen
      {
        printf ("Dump / fes_1-1 file mismatch\n");
        save_file ((char *) "Dump1_000077", buf, 5568);
        PerhapsQuit ();
      }
#endif

    if (IsH3 (version))
        return 0;

    aw_fel_send_file (handle, 0x7220, FN->fes_1_1, 2784, 2784);

#ifdef OLD_EXTRAS
    aw_fel_read (handle, 0x7220, buf + 2784, 2784);     // sanity test
    if (memcmp (buf, buf + 2784, 2784))
      {
        printf ("Readback mismatch of fes_1-1\n");
        save_file ((char *) "Dump1_000081", buf + 2784, 2784);
        PerhapsQuit ();
      }
#endif

    ShowURB (87);
    aw_fel_execute (handle, 0x7220);

    usleep (500000);    // need this to avoid error on next USB I/O

    ShowURB (96);
    aw_fel_read (handle, 0x7210, buf, 256);             // expect 'DRAM' then nulls

    if (memcmp (buf, "DRAM\x00\x00\x00\x00", 8))
      {
        perror ("Compare to DRAM lit failed");
        hexdump (buf, 0, 256);
        save_file ((char *) "Dump1_000096", buf, 256);
        PerhapsQuit ();
      }

    return 0;
}


int     install_fes_1_2 (libusb_device_handle *handle, uchar *buf)
{
    int     len; 
    ShowURB (105);
    PutNulls (buf, 16);
    aw_fel_write (handle, 0x7210, buf, 0x10);

    ShowURB (114);
    aw_fel_send_file (handle, 0x2000, FN->fes_1_2);

    ShowURB (120);
    aw_fel_execute (handle, 0x2000);

    ShowURB (129);
    len = 16;
    if (IsH3 (version))
        len = 256;
    aw_fel_read (handle, 0x7210, buf, len);             // expect 'DRAM', 0x01 then nulls

    if (memcmp (buf, "DRAM\x01\x00\x00\x00", 8))
      {
        perror ("Compare to DRAM1 lit failed");
        hexdump (buf, 0, len);
        save_file ((char *) "Dump1_000129", buf, 16);
        PerhapsQuit ();
      }

    ShowURB (138);
    aw_fel_read (handle, 0x7010, buf, 0x200);           // expect as per URB 138
    memcpy (&DramInfo, buf, sizeof (DramInfo));         // save DRAM details.

    if (IsA20 (version))
        printf ("%dMB RAM detected\n", DramInfo.dram_size);
    if (forceable)
        save_file ((char *) "Dump1_000138", buf, 0x200);

    return 0;
}


int     do_dram_test    (libusb_device_handle *handle, uchar *buf)
{
        int         x;


    ShowURB (147);

    for (x = 0; x < 0x2000; x++)
        buf [x] = random ();

    aw_fel_write (handle, 0x40100000, buf, 0x2000);

    ShowURB (153);
    //          read it back to make sure it's correct
    aw_fel_read (handle, 0x40100000, buf + 0x2000, 0x2000);
    if (memcmp (buf, buf + 0x2000, 0x2000))
      {
        perror ("Compare to DRAM test data failed");
        save_file ((char *) "Dump1_DRAM_test", buf + 0x2000, 0x2000);
        PerhapsQuit ();
      }

    return 0;
}


int     install_fes_2   (libusb_device_handle *handle, uchar *buf)
{

    ShowURB (165);
    PutNulls (buf, 16);
    aw_fel_write (handle, 0x7210, buf, 0x10);

    ShowURB (174);
    aw_fel_send_file (handle, 0x40200000, FN->fes);

    ShowURB (192);
    aw_fel_send_file (handle, 0x7220, FN->fes_2);

    ShowURB (198);
    aw_fel_execute (handle, 0x7220);

    return 0;
}


int     stage_2_prep    (libusb_device_handle *handle, uchar *buf)
{
    int  x, version2;

    if (errors)
      {
        printf ("Due to previous errors it might not be safe to continue.\n");
        PerhapsQuit ();
      }

    if (IsA10 (version))
        MagicAddr = 0x40330000;

    ShowURB (5);
    version2 = aw_fel_get_version (handle);

    if (version2 != 0x1610)                         // 0x1610 = flash mode
      {
        printf ("Expected ID 0x1610, got %04X\n", version2);
        PerhapsQuit ();
      }

    ShowURB (14);
    aw_fel_get_version (handle);

    ShowURB (24);
    aw_fes_read (handle, 0x7e00, buf, 0x100, AW_FEL2_DRAM);

    for (x = 0; x < 256; x++)
      {
        if (buf [x] != (x < 4) ? 0x00 : 0xCC)
          {
            printf ("Scratchpad incorrect\n");
            hexdump (buf, 0, 256);
            save_file ((char *) "Dump2_000024", buf, 256);
            PerhapsQuit ();
            break;
          }
      }

    ShowURB (32);
    aw_fel_get_version (handle);

    ShowURB (42);
    aw_fes_write (handle, 0x7e00, buf, 0x100, AW_FEL2_DRAM);

    return 0;
}



int     GetConfigRec        (uchar *buf, bool bEraseReqd)
{
    rSysParaHdr *pHdr;
    rAWSysPar   *pInfo;
    int         len = 0x2760;

    pHdr  = (rSysParaHdr*) buf;
    pInfo = (rAWSysPar*) &(buf [20]);

    if (IsA10 (version))
        len = 0x1578;

    read_log (buf, len, FN->ConfigRec);

    *pInfo = DramInfo;

//      save_file ((char *) "Dump1_A10_config", buf, 0x1578);

    pHdr->EraseChipFlag = bEraseReqd;
    if (IsA10 (version))
        return len;

    pInfo->dram_rank_num     = 0xFFFFFFFF;
    pInfo->dram_chip_density = 0xFFFFFFFF;
    pInfo->dram_io_width     = 0xFFFFFFFF;
    pInfo->dram_bus_width    = 0xFFFFFFFF;
    pInfo->dram_size         = 0xFFFFFFFF;

//  if (CB2_mode)
//      buf [0x218] = 0x04;

//  buf [0x218] = RAM_MB_count / 256;   // why did I do this ?

    return len;
}


int     install_fed_nand    (libusb_device_handle *handle, uchar *buf, bool bEraseReqd)
{

    ShowURB (51);
    aw_fes_write (handle, 0x40a00000, buf, GetConfigRec (buf, bEraseReqd), AW_FEL2_DRAM);

    ShowURB (60);
    aw_fes_send_file (handle, MagicAddr, AW_FEL2_DRAM, FN->magic_de_start);

    ShowURB (69);
    aw_fes_send_file (handle, 0x40430000, AW_FEL2_DRAM, FN->fed_nand, 0x8000);

    ShowURB (123);
    aw_fes_send_file (handle, MagicAddr, AW_FEL2_DRAM, FN->magic_de_end);

    ShowURB (132);

    aw_fes_exec (handle, 0x40430000, 0x31);             // 0x31 = fed | has_param

    ShowURB (135);
    aw_fes_send_4uints (handle, 0x40a00000, 0x40a01000, 0, 0);

    ShowURB (140);
    aw_fes_0203_until_ok (handle);

    ShowURB (150);
    aw_fes_0204 (handle, 0x0400);

    ShowURB (153);
    aw_pad_read (handle, buf, 0x0400);                  // NAND config
    memcpy (&NandInfo, buf, sizeof (NandInfo));
    printf ("Max NAND key = %d\n", NandInfo.SectorCount);
#ifdef OLD_EXTRAS
    hexdump (buf, 0, 256);
#endif
    if (forceable)
        save_file ((char *) "Dump2_000153", buf, 0x0400);

    return 0;
}


int     DownloadPartition   (libusb_device_handle *handle, char *fid,
                            uint32_t sector, uint32_t sectors = 0)
{
    char    *buf = (char*) malloc (65536);
    FILE    *fin;
    off64_t file_size;
    uint    file_sectors;
    uint    nand_sec_size = 512;
    uint    usb_rec_size, usb_rec_secs;
    uint    read_bytes, read_secs, bytes_read;
    uint    sector_key, sector_limit;
    uint    usb_flags;

    usb_rec_size = 65536;
    usb_rec_secs = usb_rec_size / nand_sec_size;
    usb_rec_size = usb_rec_secs * nand_sec_size;

    if (NULL == (fin = fopen64 (fid, "rb"))) {
        perror("Failed to open file to send: ");
        exit(1);
    }

//  aw_fes_send_file (handle, MagicAddr, AW_FEL2_DRAM, FN->magic_cr_start);      // not when A10

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

    while (sector_key < sector_limit)
      {
        read_secs = sector_limit - sector_key;
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
        aw_fes_write (handle, sector_key, buf, read_bytes, usb_flags);
//printf ("sector = %d, sector_key = %d, read_secs = %d, sector_limit = %d, flags = %x\n", sector, sector_key, read_secs, sector_limit, usb_flags);

        sector_key += read_secs;
      }

    printf ("done\n");
    fclose (fin);

//  aw_fes_send_file (handle, MagicAddr, AW_FEL2_DRAM, FN->magic_cr_end);        // not when A10

    free (buf);

    return 0;
}


int     send_partitions_and_MBR (libusb_device_handle *handle, uchar *buf)
{
//  FILE    *fMBR = NULL;

    ShowURB (180);
    PutNulls (buf, 12);
    aw_fes_write (handle, 0x40023c00, buf, 0x0c, AW_FEL2_DRAM); // reset CRC
#if 0
    DownloadPartition (handle, (char*) "bootloader.fex", 0x8000);
    aw_fes_read  (handle, 0x40023c00, buf, 0x0c, AW_FEL2_DRAM); // read CRC

    PutNulls (buf, 12);
    aw_fes_write (handle, 0x40023c00, buf, 0x0c, AW_FEL2_DRAM); // reset CRC
    DownloadPartition (handle, (char*) "rootfs.fex", 0x028000);
//  aw_fes_read  (handle, 0x40023c00, buf, 0x0c, AW_FEL2_DRAM); // read CRC

//  PutNulls (buf, 12);
//  aw_fes_write (handle, 0x40023c00, buf, 0x0c, AW_FEL2_DRAM); // reset CRC
    DownloadPartition (handle, (char*) "sunxi_mbr.fex", 0x00);
#endif

//  if ( ! IsA10 (version))     // A10 testing only.
        return 0;

//      A10 test stuff

    ShowURB (150);
//  DownloadPartition (handle, (char*) "/mnt/sda3/lubuntu/RFSFAT16_BOOTLOADER_00000", 0x0800);
    DownloadPartition (handle, (char*) "/mnt/sda3/Nand/A10/bootloader", 0x0800);

    ShowURB (2265);
    PutNulls (buf, 12);
    aw_fes_write (handle, 0x40023c00, buf, 0x0c, AW_FEL2_DRAM); // reset CRC

    ShowURB (2274);
//  DownloadPartition (handle, (char*) "/mnt/sda3/lubuntu/RFSFAT16_ENVIROMENT_00000", 0x020800);
    DownloadPartition (handle, (char*) "/mnt/sda3/Nand/A10/env", 0x020800);

    ShowURB (2292);
    PutNulls (buf, 12);
    aw_fes_write (handle, 0x40023c00, buf, 0x0c, AW_FEL2_DRAM); // reset CRC

    ShowURB (2301);
//  DownloadPartition (handle, (char*) "/mnt/sda3/lubuntu/RFSFAT16_ROOTFS_000000000", 0x021800);
    DownloadPartition (handle, (char*) "/mnt/sda3/Nand/A10/rootfs", 0x021800);

    ShowURB (41901);
    PutNulls (buf, 12);
    aw_fes_write (handle, 0x40023c00, buf, 0x0c, AW_FEL2_DRAM); // reset CRC

    ShowURB (41910);
//  DownloadPartition (handle, (char*) "/mnt/sda3/lubuntu/RFSFAT16_ULIB_00000000000", 0x221800);
    DownloadPartition (handle, (char*) "/mnt/sda3/Nand/A10/ulib", 0x221800);

    ShowURB (80070);
    PutNulls (buf, 12);
    aw_fes_write (handle, 0x40023c00, buf, 0x0c, AW_FEL2_DRAM); // reset CRC

    ShowURB (80079);
//  DownloadPartition (handle, (char*) "/mnt/sda3/lubuntu/RFSFAT16_USHARE_000000000", 0x421800);
    DownloadPartition (handle, (char*) "/mnt/sda3/Nand/A10/ushare", 0x421800);

    ShowURB (133935);
    PutNulls (buf, 12);
    aw_fes_write (handle, 0x40023c00, buf, 0x0c, AW_FEL2_DRAM); // reset CRC

    ShowURB (133944);
//  DownloadPartition (handle, (char*) "/mnt/sda3/lubuntu/RFSFAT16_LIBS_00000000000", 0x621800);
    DownloadPartition (handle, (char*) "/mnt/sda3/Nand/A10/libs", 0x621800);

    ShowURB (188088);
    PutNulls (buf, 12);
    aw_fes_write (handle, 0x40023c00, buf, 0x0c, AW_FEL2_DRAM); // reset CRC

    ShowURB (188100);
    aw_fes_send_file (handle, MagicAddr, AW_FEL2_DRAM, FN->magic_cr_start);

    ShowURB (188109);
//  read_log (buf, 0x1000, (char*) "A10_MBR.dump");
//  fMBR = fopen ("A10_MBR", "w+b");
//  fwrite (buf, 1, 4096, fMBR);
//  fclose (fMBR);
    DownloadPartition (handle, (char*) "/mnt/sda3/Nand/A10/MBR", 0x000000);

    ShowURB (188118);
    aw_fes_send_file (handle, MagicAddr, AW_FEL2_DRAM, FN->magic_cr_end);

    ShowURB (188124);
    aw_fes_read  (handle, 0x40023c00, buf, 0x0c, AW_FEL2_DRAM); // read CRC
    hexdump (buf, 0, 12);

    aw_fes_0205  (handle, 0x02);                                    // 2 partitions ?  no, always 02

    return 0;
}


int     install_boot1   (libusb_device_handle *handle, uchar *buf)
{
    ShowURB (113241);

    if (IsA10 (version))
        aw_fes_send_file (handle, MagicAddr, AW_FEL2_DRAM, FN->magic_de_start);

    aw_fes_send_file (handle, 0x40600000, AW_FEL2_DRAM, FN->boot1_nand);

    if (IsA10 (version))
        aw_fes_send_file (handle, MagicAddr, AW_FEL2_DRAM, FN->magic_de_end);

    ShowURB (113303);
    aw_fes_write (handle, 0x40400000, buf, GetConfigRec (buf, 0), AW_FEL2_DRAM);

    ShowURB (113316);
    aw_fes_write (handle, 0x40410000, ((char*) &NandInfo) + 32, 0x00ac, AW_FEL2_DRAM);

    ShowURB (113322);
    aw_fes_send_file (handle, MagicAddr, AW_FEL2_DRAM, FN->magic_de_start);

    ShowURB (113331);
    aw_fes_send_file (handle, 0x40430000, AW_FEL2_DRAM, FN->update_boot1, 32768);

    ShowURB (113384);
    aw_fes_send_file (handle, MagicAddr, AW_FEL2_DRAM, FN->magic_de_end);

    ShowURB (113394);
    aw_fes_exec (handle, 0x40430000, 0x11);             // 0x11 = fet | has_param

    ShowURB (113397);
    aw_fes_send_4uints (handle, 0x40600000, 0x40400000, 0x40410000, 0);

    ShowURB (113402);
    aw_fes_0203_until_ok (handle);

    ShowURB (113502);
    aw_fes_0204 (handle, 0x0400);

    ShowURB (113505);
    aw_pad_read (handle, buf, 0x0400);
    printf ("%s\n", &buf [24]);
    if (strcmp ((char*) &buf [24], "updateBootxOk000"))
      {
        hexdump (buf, 0, 1024);
        save_file ((char *) "Dump2_113505", buf, 1024);
      }

    return 0;
}


int     install_boot0   (libusb_device_handle *handle, uchar *buf)
{
    ShowURB (113514);
    aw_fes_send_file (handle, MagicAddr, AW_FEL2_DRAM, FN->magic_de_start);

    ShowURB (113523);

    aw_fes_send_file (handle, 0x40600000, AW_FEL2_DRAM, FN->boot0_nand);

    ShowURB (113532);
    aw_fes_send_file (handle, MagicAddr, AW_FEL2_DRAM, FN->magic_de_end);

    ShowURB (113541);
    aw_fes_write (handle, 0x40400000, buf, GetConfigRec (buf, 0), AW_FEL2_DRAM);

    ShowURB (113547);
    aw_fes_write (handle, 0x40410000, ((char*) &NandInfo) + 32, 0x00ac, AW_FEL2_DRAM);

    ShowURB (113559);
    aw_fes_send_file (handle, MagicAddr, AW_FEL2_DRAM, FN->magic_de_start);

    ShowURB (113565);
    aw_fes_send_file (handle, 0x40430000, AW_FEL2_DRAM, FN->update_boot0, 32768);

    ShowURB (113610);
    aw_fes_send_file (handle, MagicAddr, AW_FEL2_DRAM, FN->magic_de_end);

    ShowURB (113619);
    aw_fes_exec (handle, 0x40430000, 0x11);             // 0x11 = fet | has_param

    ShowURB (113622);
    aw_fes_send_4uints (handle, 0x40600000, 0x40400000, 0x40410000, 0);

    ShowURB (113628);
    aw_fes_0203_until_ok (handle);

    ShowURB (113655);
    aw_fes_0204 (handle, 0x0400);

    ShowURB (113658);
    aw_pad_read (handle, buf, 0x0400);
    printf ("%s\n", &buf [24]);
    if (strcmp ((char*) &buf [24], "updateBootxOk000"))
      {
        hexdump (buf, 0, 1024);
        save_file ((char *) "Dump2_113658", buf, 1024);
      }

    return 0;
}


int     restore_system  (libusb_device_handle *handle, uchar *buf)
{
    ShowURB (113664);
    aw_fel_get_version (handle);

    ShowURB (113673);
    aw_fes_write (handle, 0x7e04, (char*) "\xcd\xa5\x34\x12", 0x04, AW_FEL2_DRAM);

    ShowURB (113682);
    aw_fes_send_file (handle, MagicAddr, AW_FEL2_DRAM, FN->magic_de_start);

    ShowURB (113691);
    aw_fes_send_file (handle, 0x40430000, AW_FEL2_DRAM, FN->fet_restore, 0x8000);

    ShowURB (113703);
    aw_fes_send_file (handle, MagicAddr, AW_FEL2_DRAM, FN->magic_de_end);

    ShowURB (113709);
    aw_fes_exec (handle, 0x40430000, 0x11);

    ShowURB (113712);
    PutNulls (buf, 16);
    aw_pad_write (handle, buf, 0x10);

    return 0;
}


int     stage_1         (libusb_device_handle *handle, uchar *buf)
{
    if (2 != ShowUSBSpeed (handle))                 // 12 Mbps here
        printf ("Expected 12Mbps, continuing anyway.\n");

    printf ("Start stage 1\n");

    stage_1_prep    (handle, buf);

    install_fes_1_1 (handle, buf);

    install_fes_1_2 (handle, buf);

    do_dram_test    (handle, buf);

    install_fes_2   (handle, buf);

    printf ("End of stage 1\n");

    return 0;
}


int     stage_2         (libusb_device_handle *handle, uchar *buf)
{
    ShowUSBSpeed            (handle);                   // 480 Mbps now

    printf ("Start stage 2\n");

    stage_2_prep (handle, buf);

    install_fed_nand (handle, buf, bEraseReqd);

    if (readNAND == 1)                                      // read test
        GetAllNAND (handle, NAND_FID, 0, 0);
    else if (writeNAND == 1)                                // write test
        PutAllNAND (handle, NAND_FID, 0, 0);
    else if (loadNAND == 1)                                 // MBR and partitions
        LoadNAND (handle, part_cnt, part_name, part_start, part_secs);
    else 
        send_partitions_and_MBR (handle, buf);              // not done yet !!!

    install_boot1 (handle, buf);
    install_boot0 (handle, buf);

    restore_system (handle, buf);

    printf ("End of stage 2\n");

    return 0;
}


#include "usblib.inc"


int     main            (int argc, char * argv [])
{
    int     x;
    uchar   *buf = (uchar*) malloc (65536);
    char    *pc;

    assert (buf != 0);

    rc = libusb_init (NULL);
    assert (rc == 0);

    if (NULL == (realpath (argv [0], szBasePath)))
      {
        printf ("Can't get realpath\n");
        return (1);
      }
    for (pc = strchr (szBasePath, '\0'); *pc != '/'; pc--) *pc = '\0';

    FN = &FN_sun7i;                             // A20 by default.

    while (argc > 1)
      {
        if (strcmp (argv [1], (char *) "-t") == 0)
          {
            USBTests (buf);
            goto bye;
          }
        if (strcmp (argv [1], (char *) "-H") == 0)
          {
            H3_Tests (buf);
            goto bye;
          }
        if (strcmp (argv [1], (char *) "-l") == 0)
          {
            Lime_Tests (buf);
            goto bye;
          }
        if (strcmp (argv [1], (char *) "-a") == 0)
          {
            A20_Tests (buf);
            goto bye;
          }
        if (strcmp (argv [1], (char *) "-h") == 0)
          {
            printf ("no flags = update boot0 and boot1.\n");
            printf ("-x = allow force continue when error.\n");
            printf ("-e = erase chip before boot update.\n");
            printf ("-r = read all NAND, save to NAND.DAT.\n");
            printf ("-w = write NAND.DAT to device NAND.\n");
            printf ("-t = USB tests.\n");
            printf ("-i <file list> = create NAND MBR, write MBR and files to NAND,\n");
            printf ("  each file is a partition to write, the file name is used as\n");
            printf ("  the partition name in the MBR.\n");
            printf ("  Each file entry consists of a name and start and size enclosed in quotes.\n");
            printf ("  The start is the sector key (0 = use next available sector).\n");
            printf ("  The size is the partition size in sectors (0 = use file size).\n");
            printf ("  e.g. bootfix -i \"/data/boot 2048 0\" \"/data/rootfs 0 0\" \"/data/extra 0 0\"\n");
            goto bye;
          }
        if (strcmp (argv [1], (char *) "-x") == 0)
            forceable = 1;
        else if (strcmp (argv [1], (char *) "-e") == 0)
            bEraseReqd = 1;
        else if (strcmp (argv [1], (char *) "-r") == 0)
          {
            readNAND = 1;
            if (argc > 2)
                strcpy (NAND_FID, argv [2]);
            break;
          }  
        else if (strcmp (argv [1], (char *) "-w") == 0)
          {
            writeNAND = 1;
            if (argc > 2)
                strcpy (NAND_FID, argv [2]);
            break;
          }
        else if (strcmp (argv [1], (char *) "-i") == 0)
          {
            loadNAND = 1;
            part_cnt = argc - 2;
            if (part_cnt > 16)
                part_cnt = 16;
            BOJLoadNANDCheck (part_cnt, &argv [2], part_start, part_secs);
            bEraseReqd = true;
            break;
          }
        argc--;
        argv++;
      }

    handle = open_usb ();                               // stage 1
    stage_1 (handle, buf);
    handle = close_usb (handle);

    printf ("Re-opening");                              // stage 2
    fflush (stdout);
    for (x = 0; !handle; x++)
      {
        usleep (1000000);                               // wait 1 sec
        printf (".");
        fflush (stdout);
        handle = open_usb (x < 10);                     // try to open
      }
    printf ("done\n");
    stage_2 (handle, buf);
    handle = close_usb (handle);

    printf ("All done\n");

bye:
    libusb_exit (NULL);

    return 0;
}


