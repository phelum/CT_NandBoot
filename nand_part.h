/*
 * This is a portion of Patrick's file.
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

#ifndef __nand_common_h__
#define __nand_common_h__

#include "types.h"

#define MAX_NAME 16

#endif

#ifndef    __MBR_H__
#define    __MBR_H__

#include "types.h"

#define v3_MBR_MAGIC "softw311"
#define v4_MBR_MAGIC "softw411"
#define v3_MBR_VERSION 0x100
#define v4_MBR_VERSION 0x200
//#define nand_part nand_part_a20
//#define checkmbrs checkmbrs_a20

#define v3_MAX_PART_COUNT       15                                      //max part count
#define v4_MAX_PART_COUNT       120                                     //max part count
#define MBR_COPY_NUM        4                                       //mbr backup count

#define MBR_START_ADDRESS   0x0                                     //mbr start address
#define v3_MBR_SIZE         1024                                    //mbr size
#define v4_MBR_SIZE         1024*16                                 //mbr size
#define v3_MBR_RESERVED        (v3_MBR_SIZE - 20 - (v3_MAX_PART_COUNT * 64))  //mbr reserved space
#define v4_MBR_RESERVED        (v4_MBR_SIZE - 32 - (v4_MAX_PART_COUNT * 128)) //mbr reserved space

#define DiskSize  (SECTOR_CNT_OF_SINGLE_PAGE * PAGE_CNT_OF_PHY_BLK * BLOCK_CNT_OF_DIE * \
            DIE_CNT_OF_CHIP * NandStorageInfo.ChipCnt  / 1024 * DATA_BLK_CNT_OF_ZONE)


struct nand_disk{
    unsigned long size;
    unsigned long offset;
    unsigned char type;
};


/* part info */

typedef struct v3_nand_tag_PARTITION {              // 64 bytes
    __u32                   addrhi;             //start address high 32 bit
    __u32                   addrlo;             //start address low 32 bit
    __u32                   lenhi;              //size high 32 bit
    __u32                   lenlo;              //size low 32 bit
    __u8                    classname[12];      //major device name
    __u8                    name[12];               //minor device name
    __u32                   user_type;
    __u32                   ro;
    __u8                    res[16];                //reserved
} v3_PARTITION;


typedef struct v4_nand_tag_PARTITION {              // 128 bytes
    __u32                   addrhi;
    __u32                   addrlo;
    __u32                   lenhi;
    __u32                   lenlo;
    __u8                    classname [16];
    __u8                    name [16];
    __u32                   user_type;
    __u32                   keydata;
    __u32                   ro;
    __u8                    res [68];
} __attribute__ ((packed)) v4_PARTITION;


/* mbr info */
typedef struct v3_nand_tag_MBR {
    __u32                   crc32;                      // crc 1k - 4
    __u32                   version;                    // 0x00000100
    __u8                    magic[8];                   //"softw311"
    __u8                    copy;                       // mbr backup count
    __u8                    index;                      // current part no
    __u16                   PartCount;                  // part counter
    v3_PARTITION            array [v3_MAX_PART_COUNT];  // part info
    __u8                    res [v3_MBR_RESERVED];      // reserved space
} v3_MBR;

typedef struct v4_nand_tag_MBR {
        unsigned  int       crc32;                      // crc 1k - 4
        unsigned  int       version;                    // 0x00000200
        unsigned  char      magic[8];                   //"softw411"
        unsigned  int       copy;
        unsigned  int       index;
        unsigned  int       PartCount;
        unsigned  int       stamp[1];
        v4_PARTITION        array[v4_MAX_PART_COUNT];  //
        unsigned  char      res[v4_MBR_RESERVED];
}__attribute__ ((packed)) v4_MBR;

//int mbr2disks(struct nand_disk* disk_array);
int IsA10               (int version);

#endif    //__MBR_H__

