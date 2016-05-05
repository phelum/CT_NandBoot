/*
 * This is a portion of Patrick's file (crc routine and part of
 * writembrs).  The rest of this free software is my stuff for
 * my bootfix program.
 *
 * Steven Saunderson 2015-11-20
 */

/*
 * (C) Copyright 2013
 * Patrick H Wood, All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mount.h> /* BLKRRPART */
#include "nand_part.h"

typedef struct tag_CRC32_DATA
{
    __u32 CRC;
    __u32 CRC_32_Tbl[256];
}CRC32_DATA_t;

__u32 calc_crc32(void * buffer, __u32 length)
{
    __u32 i, j;
    CRC32_DATA_t crc32;
    __u32 CRC32 = 0xffffffff;
    crc32.CRC = 0;

    for( i = 0; i < 256; ++i)
    {
        crc32.CRC = i;
        for( j = 0; j < 8 ; ++j)
        {
            if(crc32.CRC & 1)
                crc32.CRC = (crc32.CRC >> 1) ^ 0xEDB88320;
            else
                crc32.CRC >>= 1;
        }
        crc32.CRC_32_Tbl[i] = crc32.CRC;
    }

    CRC32 = 0xffffffff;
    for( i = 0; i < length; ++i)
    {
        CRC32 = crc32.CRC_32_Tbl[(CRC32^((unsigned char*)buffer)[i]) & 0xff] ^ (CRC32>>8);
    }
    //return CRC32;
    return CRC32^0xffffffff;
}


static int v3_writembrs (int fd, char names[][MAX_NAME], __u32 *start, __u32 *lens,
                         unsigned int *user_types, int nparts, int partoffset)
{
    int     i;
    v3_MBR  *mbr = NULL;

    mbr = (v3_MBR*) malloc (sizeof (v3_MBR));
    if(mbr == NULL)
    {
        printf("%s : request memory fail\n",__FUNCTION__);
        return 0; // NULL;
    }

    memset (mbr, 0, sizeof (v3_MBR));
    mbr->version = 0x00000100;
    memcpy (mbr->magic, (char*) "softw311", 8);
    mbr->copy = 0;
    mbr->PartCount = nparts + partoffset;
//  if (partoffset)
//      start = mbr->array[0].addrlo + mbr->array[0].lenlo;
    for(i = 0; i < nparts; i++) {
        strncpy((char *)mbr->array[i+partoffset].name, names[i], 12);
        strncpy((char *)mbr->array[i+partoffset].classname, "DISK", 12);
        memset((void *) mbr->array[i+partoffset].res, 0, sizeof(mbr->array[i+partoffset].res));
        mbr->array[i+partoffset].user_type = user_types[i];
        mbr->array[i+partoffset].ro = 0;
        mbr->array[i+partoffset].addrhi = 0;
        mbr->array[i+partoffset].lenhi = 0;
        mbr->array[i+partoffset].addrlo = start [i];
        mbr->array[i+partoffset].lenlo = lens[i];
//      start += lens[i];
    }

    for (i = 0; i < MBR_COPY_NUM; i++) {
        mbr->index = i;
        // calculate new checksum
        *(__u32 *)mbr = calc_crc32((__u32 *)mbr + 1,v3_MBR_SIZE - 4);
        lseek(fd,MBR_START_ADDRESS + v3_MBR_SIZE*i,SEEK_SET);
        write(fd,mbr,v3_MBR_SIZE);
    }

    return 1;
}


static int v4_writembrs (int fd, char names[][MAX_NAME], __u32 *start, __u32 *lens,
                         unsigned int *user_types, int nparts, int partoffset)
{
    int     i;
    v4_MBR  *mbr = NULL;

    mbr = (v4_MBR*) malloc (sizeof (v4_MBR));
    if(mbr == NULL)
    {
        printf("%s : request memory fail\n",__FUNCTION__);
        return 0; // NULL;
    }

    memset (mbr, 0, sizeof (v4_MBR));
    mbr->version = 0x00000200;
    memcpy (mbr->magic, (char*) "softw411", 8);
    mbr->copy = 0;
    mbr->PartCount = nparts + partoffset;
//  if (partoffset)
//      start = mbr->array[0].addrlo + mbr->array[0].lenlo;
    for(i = 0; i < nparts; i++) {
        strncpy((char *)mbr->array[i+partoffset].name, names[i], 16);
        strncpy((char *)mbr->array[i+partoffset].classname, "DISK", 16);
        memset((void *) mbr->array[i+partoffset].res, 0, sizeof(mbr->array[i+partoffset].res));
        mbr->array[i+partoffset].user_type = user_types[i];
        mbr->array[i+partoffset].ro = 0;
        mbr->array[i+partoffset].addrhi = 0;
        mbr->array[i+partoffset].lenhi = 0;
        mbr->array[i+partoffset].addrlo = start [i];
        mbr->array[i+partoffset].lenlo = lens[i];
//      start += lens[i];
    }

    for (i = 0; i < MBR_COPY_NUM; i++) {
        mbr->index = i;
        // calculate new checksum
        *(__u32 *)mbr = calc_crc32((__u32 *)mbr + 1,v4_MBR_SIZE - 4);
        lseek(fd,MBR_START_ADDRESS + v4_MBR_SIZE*i,SEEK_SET);
        write(fd,mbr,v4_MBR_SIZE);
    }

    return 1;
}


int     GetAllNAND          (libusb_device_handle *handle, char *fid,
                            uint32_t sector, uint32_t sectors = 0)
{
        char    *buf = (char*) malloc (65536);
        FILE    *fin;
        uint    file_sectors;
        uint    nand_sec_size = 512;
        uint    usb_rec_size, usb_rec_secs;
        uint    read_bytes, read_secs, bytes_read;
        uint    sector_key, sector_limit;
        uint    usb_flags;
        int     erc;

    usb_rec_size = 65536;
    usb_rec_secs = usb_rec_size / nand_sec_size;
    usb_rec_size = usb_rec_secs * nand_sec_size;

    if (NULL == (fin = fopen64 (fid, "wb")))
      {
        perror ("Failed to open output file: ");
        exit (1);
      }

    aw_fes_send_file (handle, 0x40360000, AW_FEL2_DRAM, FN->magic_cr_start);

    file_sectors = NandInfo.SectorCount;
    printf ("Sectors to get = %d\n", file_sectors);

    sector_key = sector;
    if (sectors < file_sectors)
        sectors = file_sectors;
    sector_limit = sector_key + sectors;

    printf ("Creating %s...", fid);

    while (sector_key < sector_limit)
      {
        printf ("%d", sector_key);
        fflush (stdout);
        read_secs = sector_key - sector_limit;
        if (read_secs > usb_rec_secs)
            read_secs = usb_rec_secs;
        read_bytes = read_secs * nand_sec_size;

        usb_flags = AW_FEL2_NAND | AW_FEL2_RD;
        erc = aw_fes_read (handle, sector_key, buf, read_bytes, usb_flags);
        if (erc)
          {
            printf ("Error on read, sector = %d\n", sector_key);
            break;
          }

        bytes_read = fwrite (buf, 1, read_bytes, fin);
        if (bytes_read < read_bytes)
          {
            printf ("Short write, sector = %d\n", sector_key);
            break;
          }

        sector_key += read_secs;
        printf ("\rCreating %s...", fid);
      }

    printf ("done    \n");
    fclose (fin);

    aw_fes_send_file (handle, 0x40360000, AW_FEL2_DRAM, FN->magic_cr_end);

    free (buf);

    return 0;
}


int     PutSomeNAND         (libusb_device_handle *handle, char *fid, FILE *fin,
                            uint32_t sector, uint32_t sectors = 0, bool bCRC_reqd = 0)
{
        char    *buf = (char*) malloc (65536);
        off64_t file_size;
        uint    file_sectors;
        uint    nand_sec_size = 512;
        uint    usb_rec_size, usb_rec_secs;
        uint    read_bytes, read_secs, bytes_read;
        uint    sector_key, sector_limit;
        uint    usb_flags;
        int     erc, count;
        uint    temp;

    usb_rec_size = 65536;
    usb_rec_secs = usb_rec_size / nand_sec_size;
    usb_rec_size = usb_rec_secs * nand_sec_size;

    if (bCRC_reqd)
      {
        PutNulls (buf, 12);
        aw_fes_write (handle, 0x40023c00, buf, 0x0c, AW_FEL2_DRAM); // reset CRC
        aw_fes_send_file (handle, 0x40360000, AW_FEL2_DRAM, FN->magic_cr_start);
      }

    fseeko64 (fin, 0, SEEK_END);
    file_size = ftello64 (fin);
    file_sectors = (file_size + nand_sec_size - 1) / nand_sec_size;
    fseeko64 (fin, 0, SEEK_SET);
    printf ("File sectors = %d\n", file_sectors);
    if (sectors < file_sectors)
        sectors = file_sectors;
    printf ("Partition sectors = %d\n", sectors);

    printf ("Sending %s...", fid);

    sector_key = sector;
//  sector_limit = sector_key + sectors;            // = null fill after
    sector_limit = sector_key + file_sectors;       // = don't fill after

//      When writing to NAND we have to interrupt the process periodically
//      and let the writing catch up.  Currently this is done by sending the
//      CR end and start files after each 262144 sectors.
//      Looks like the problem occurs when sending too many records before
//      calling CRC calc.  So, not really a problem for us.

    count = 0;
    while (sector_key < sector_limit)
      {
        printf ("%d", sector_key - sector);
        fflush (stdout);

        if (((sector_key & 0x3FFFF) == 0) && (count > 0))       // new ???MB boundary ?
          {
            count = 0;
            if (bCRC_reqd)
              {
                printf ("\n");
                aw_fes_send_file (handle, 0x40360000, AW_FEL2_DRAM, FN->magic_cr_end);
                aw_fes_send_file (handle, 0x40360000, AW_FEL2_DRAM, FN->magic_cr_start);
              }
          }

        read_secs = sector_limit - sector_key;
        if (read_secs > usb_rec_secs)
            read_secs = usb_rec_secs;

//      Don't cross 128MB boundary.

        temp = (sector_key & 0xFFFF) + usb_rec_secs;
        if (temp > 0x10000)
            read_secs -= (temp - 0x10000);

        read_bytes = read_secs * nand_sec_size;

        bytes_read = fread (buf, 1, read_bytes, fin);
        if (bytes_read < read_bytes)
            PutNulls (buf + bytes_read, read_bytes - bytes_read);

        usb_flags = AW_FEL2_NAND | AW_FEL2_WR;
        if (sector_key == sector)
            usb_flags |= AW_FEL2_FIRST;
        if (sector_key + read_secs == sector_limit)
            usb_flags |= AW_FEL2_LAST;

        erc = aw_fes_write (handle, sector_key, buf, read_bytes, usb_flags);
        if (erc)
          {
            printf ("Write error, sector = %d\n", sector_key);
            break;
          }

        sector_key += read_secs;

        count += read_secs;

        printf ("\rSending %s...", fid);
      }

    printf ("done    \n");

    if (bCRC_reqd)
      {
        aw_fes_send_file (handle, 0x40360000, AW_FEL2_DRAM, FN->magic_cr_end);
        aw_fes_read  (handle, 0x40023c00, buf, 0x0c, AW_FEL2_DRAM); // read CRC
        printf ("CRC read complete 0x%08X\n", *(uint*) &(buf [8]));
      }

    free (buf);

    return 0;
}


int     PutAllNAND          (libusb_device_handle *handle, char *fid,
                            uint32_t sector, uint32_t sectors = 0)
{
        FILE    *fin;

    if (NULL == (fin = fopen64 (fid, "rb")))
      {
        perror("Failed to open file to send: ");
        exit(1);
      }

    PutSomeNAND (handle, fid, fin, sector, sectors);

    fclose (fin);

    aw_fes_0205  (handle, 0x02);                                    // 2 = NAND ?
    printf ("0205 sent\n");

    return 0;
}


int     BOJLoadNANDCheck    (int part_cnt, char ** part_list, uint part_start [], uint part_secs [])
{
        int     x, errs = 0;
        char*   fid;
        FILE    *fin;
        off64_t file_size;
        uint    file_sectors;
        uint    nand_sec_size = 512;

    for (x = 0; x < part_cnt; x++)
      {
        fid = part_name [x];
        if (sscanf (part_list [x], "%s %d %d", fid, &part_start [x], &part_secs [x]) < 3)
          {
            printf ("Expected file name and partition start and sector count : %s\n", part_list [x]);
            errs++;
          } 
        else
          {
            if (NULL == (fin = fopen64 (fid, "rb")))                // present ?
              {
                printf ("Failed to open file: %s\n", fid);
                errs++;
              }
            else                                                  // get size
              {
                fseeko64 (fin, 0, SEEK_END);
                file_size = ftello64 (fin);
                file_sectors = (file_size + nand_sec_size - 1) / nand_sec_size;
                fclose (fin);
                if (part_secs [x] < file_sectors)
                    part_secs [x] = file_sectors;
              }
          }
      }

    if (errs > 0)
      {
        printf ("Fatal error(s) in specs\n");
        exit (3);
      }

    return 0;
}


int     LoadNAND            (libusb_device_handle *handle,
                            int part_cnt, char part_name [16] [256],
                            uint part_start [], uint part_size [])
{
        int     x;
        FILE    *fin;

//      The FEL mode program reports that partitions and lengths are not aligned.
//      Not sure what this means and how to fix it; things are working.
//      I've tried different start addresses for the partitions and also
//      inceasing sizes so partitions start on large boundaries.
//      Probably "align" means align to NAND block boundaries.
//      4096 aligns to max 2MB boundaries - but doesn't stop warnings in boot1.
//      Currently using 256 so all start addrs (hex) end in 0x00.
//      8192 should cater for 4MiB blocks - user should adjust load script if reqd.

//      2016-05-05: Partitions must be super-block aligned.  This is page size in bytes
//      times page count per block times plane count of die.  Since we work in sectors
//      here we use page size in sectors.  We are assuming a maximum page size of 16
//      sectors, a block size of 256 pages, and 2 planes.  This gives us 16*256*2 = 8192.

        uint    parts_incr = 8192; // 4096; // 256; // 32768; // 256; // 128;
        uint    parts_base = parts_incr;
        uint    parts_end  = parts_base;
        uint    part_secs  [16];
        uint    part_type  [16];
        char    names      [16] [MAX_NAME];
        char    *in, *in1, *out, *fid;
        int     len;
        char    *mbr_fid = (char*) "nand_mbr";
        int     mbr_fd, mbr_part_cnt = part_cnt;

//      2016-05-05: Calc increment from NAND info.

    parts_incr = NandInfo.PlaneCountPerDie *
                 NandInfo.SectorCountPerPage *
                 NandInfo.PageCountPerPhyBlk;
    printf ("NAND planes = %d\n", NandInfo.PlaneCountPerDie);
    printf ("NAND sectors/page = %d\n", NandInfo.SectorCountPerPage);
    printf ("NAND pages/block = %d\n", NandInfo.PageCountPerPhyBlk);
    printf ("NAND partition key & size increment = %d\n", parts_incr);
    if (parts_incr < 256)
      {
        parts_incr = 256;
        printf ("Oops, increment calcs failed, using %d\n", parts_incr);
      }

    for (x = 0; x < part_cnt; x++)
      {
        if (part_start [x] < parts_end)
            part_start [x] = parts_end;                 // start sector
        parts_end = part_start [x] + part_size [x];
        parts_end += parts_incr - 1;                    // 4MiB multiples
        parts_end /= parts_incr;
        parts_end *= parts_incr;
        part_secs [x] = parts_end - part_start [x];     // secs = size + padding
        part_type [x] = 0;
        in = part_name [x];                             // file name
        if ((char*) NULL < (in1 = strrchr (in, '/')))   // skip all '/'
            in = in1 + 1;
        for (len = 0; len < MAX_NAME - 1; len++)        // up to 15 chars
            if (in [len] < ' ')
                break;
        out = names [x];
        memcpy (out, in, len);                          // copy
        out += len;
        while (len < MAX_NAME)                          // nulls as reqd
          {
            *out++ = '\0';
            len++;
          }
      }

    if (parts_end < NandInfo.SectorCount)               // dummy at end ?
      {
        part_start [x] = parts_end;                     // start sector
        part_secs  [x] = NandInfo.SectorCount - parts_end;
        parts_end      = NandInfo.SectorCount;
        part_type [x] = 0;
        in = (char*) "UDISK";
        len = 5;
        out = names [x];
        memcpy (out, in, len);                          // copy
        out += len;
        while (len < MAX_NAME)                          // nulls as reqd
          {
            *out++ = '\0';
            len++;
          }
        mbr_part_cnt++;
      }

    if (parts_end > NandInfo.SectorCount)
      {
        printf ("Error, total sectors reqd (%d) > NAND sectors (%d).\n",
                parts_end, NandInfo.SectorCount);
        exit(1);
      }

    if (0 > (mbr_fd = open (mbr_fid, O_RDWR | O_CREAT | O_TRUNC)))
      {
        perror("Failed to open mbr file: ");
        exit(1);
      }

    if (IsA10 (version))
        v3_writembrs (mbr_fd, names, part_start, part_secs, part_type, mbr_part_cnt, 0);
    else
        v4_writembrs (mbr_fd, names, part_start, part_secs, part_type, mbr_part_cnt, 0);
    close (mbr_fd);

    fin = fopen (mbr_fid, "rb");                        // write MBR to NAND
    PutSomeNAND (handle, mbr_fid, fin, 0, parts_incr, 1);
    fclose (fin);

    for (x = 0; x < part_cnt; x++)                      // write partitions to NAND
      {
        fid = part_name [x];
        fin = fopen64 (fid, "rb");
        PutSomeNAND (handle, fid, fin, part_start [x], part_secs [x], 0);
        fclose (fin);
      }

    aw_fes_0205  (handle, 0x02);                        // 2 = NAND ?
    printf ("0205 sent\n");

    return 0;
}



