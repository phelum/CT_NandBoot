typedef unsigned char uchar;
typedef unsigned int  uint;

#define PutNulls(b,l) memset (b, 0, l)


typedef struct tag_NandInfo                     // 256 bytes
{
    __u32               u10000200;              // 0000 0x10000200
    __u32               zeroes [6];             // 0004
    __u32               one_0;                  // 001C 0x01
    __u16               ChipCnt;                // 0020 0x01
    __u16               ChipConnectInfo;        // 0022 0x01
    __u8                RbCnt;                  // 0024 0x01
    __u8                RbConnectInfo;          // 0025 0x01
    __u8                RbConnectMode;          // 0026 0x01
    __u8                BankCntPerChip;         // 0027 0x01
    __u8                DieCntPerChip;          // 0028 0x01
    __u8                PlaneCountPerDie;       // 0029 0x02
    __u16               SectorCountPerPage;     // 002A 0x10
    __u16               PageCountPerPhyBlk;     // 002C 0x0100
    __u16               BlockCountPerDie;       // 002E 0x0800
    __u16               OperationOpt;           // 0030 0x1188
    __u8                FrequencePar;           // 0032 0x1E
    __u8                EccMode;                // 0033 0x03
    __u8                NandChipID [8];         // 0034 0xDA94D7AD 0xFFFFFFFF
    __u32               ValidBlockRatio;        // 003C 0x03B0
    __u32               GoodBlockRatio;         // 0040 0x03B0
    __u32               ReadRetryType;          // 0044 0x010604
    __u32               DDRType;                // 0048 0x0
    __u8                unk_c [132];            // 004C
    __u32               SectorCount;            // 00D0 0x760000
    __u32               unk_d [11];             // 00D4
}
__attribute__ ((packed)) rNandInfo;


typedef struct tag_AWSysPar                     // 256 bytes    AWSystemParameters
{
    __u32               unk_0;                  // 0000 0 if A20, 0x02000000 if A10     chip
    __u32               unk_1;                  // 0004 0 if A20, 0x02000000 if A10     pid
    __u32               unk_2;                  // 0008 0 if A20, 0x02000100 if A10     sid
    __u32               unk_3;                  // 000C 0 if A20, 0x00000080 if A10     bid
    __u32               unk_4;                  // 0010 0
    __u32               unk_5;                  // 0014 0
    __u32               unk_6;                  // 0018 0x007C4AC1  uart_debug_tx_pin
    __u32               unk_7;                  // 001C 0           uart_debug_port
    __u32               dram_baseaddr;          // 0020 0x40000000
    __u32               dram_clk;               // 0024 0x01B0
    __u32               dram_type;              // 0028 0x03
    __u32               dram_rank_num;          // 002C 0x01
    __u32               dram_chip_density;      // 0030 0x1000
    __u32               dram_io_width;          // 0034 0x0010
    __u32               dram_bus_width;         // 0038 0x20
    __u32               dram_cas;               // 003C 0x09
    __u32               dram_zq;                // 0040 0x7F
    __u32               dram_odt_en;            // 0044 0x00
    __u32               dram_size;              // 0048 0x0400  MB
    __u32               dram_tpr0;              // 004C 0x42D899B7
    __u32               dram_tpr1;              // 0050 0xA090
    __u32               dram_tpr2;              // 0054 0x022A00
    __u32               dram_tpr3;              // 0058 0x00
    __u32               dram_tpr4;              // 005C 0x01
    __u32               dram_tpr5;              // 0060 0x00
    __u32               dram_emr1;              // 0064 0x04
    __u32               dram_emr2;              // 0068 0x10
    __u32               dram_emr3;              // 006C 0x00
    __u32               dram_spare [36];        // 0070
}
__attribute__ ((packed)) rAWSysPar;


typedef struct tag_AWDRAMData           // < BinData::Record # size 136?
{
    __u8                magic [4];              // 0000 "DRAM"
    __u32               is_loaded;              //
    __u32               dram_clk;               //
    __u32               dram_type;              //
    __u32               dram_zq;                //
    __u32               dram_odt_en;            //
    __u32               dram_para1;             //
    __u32               dram_para2;             //
    __u32               dram_mr0;               //
    __u32               dram_mr1;               //
    __u32               dram_mr2;               //
    __u32               dram_mr3;               //
    __u32               dram_tpr0;              //
    __u32               dram_tpr1;              //
    __u32               dram_tpr2;              //
    __u32               dram_tpr3;              //
    __u32               dram_tpr4;              //
    __u32               dram_tpr5;              //
    __u32               dram_tpr6;              //
    __u32               dram_tpr7;              //
    __u32               dram_tpr8;              //
    __u32               dram_tpr9;              //
    __u32               dram_tpr10;             //
    __u32               dram_tpr11;             //
    __u32               dram_tpr12;             //
    __u32               dram_tpr13;             //
    __u32               dram_unknown [8];       //
} 
__attribute__ ((packed)) rAWDRAMData;


//# size 180
//# Used on old CPUs which contains sys_config1.fex & sys_config.fex
typedef struct tag_AWLegacySystemParameters     // < BinData::Record
{
    __u32               chip;                   // 0x00 [platform]
    __u32               pid;                    // 0x04 [platform]
    __u32               sid;                    // 0x08 [platform]
    __u32               bid;                    // 0x0C [platform]
    __u32               unk5;                   // 0x10
    __u32               unk6;                   // 0x14
    __u32               uart_debug_tx;          // 0x18 [uart_para]
    __u32               uart_debug_port;        // 0x0C [uart_para]
    __u32               unk7 [15];              // 0x20
    __u32               dram_baseaddr;          // 0x5C
    __u32               dram_clk;               // 0x60
    __u32               dram_type;              // 0x64
    __u32               dram_rank_num;          // 0x68
    __u32               dram_chip_density;      // 0x6C
    __u32               dram_io_width;          // 0x70
    __u32               dram_bus_width;         // 0x74
    __u32               dram_cas;               // 0x78
    __u32               dram_zq;                // 0x7C
    __u32               dram_odt_en;            // 0x80
    __u32               dram_size;              // 0x84
    __u32               dram_tpr0;              // 0x88
    __u32               dram_tpr1;              // 0x8C
    __u32               dram_tpr2;              // 0x90
    __u32               dram_tpr3;              // 0x94
    __u32               dram_tpr4;              // 0x98
    __u32               dram_tpr5;              // 0x9C
    __u32               dram_emr1;              // 0xA0
    __u32               dram_emr2;              // 0xA4
    __u32               dram_emr3;              // 0xA8
    __u32               unk8 [2];               // 0xAC
} 
__attribute__ ((packed)) rAWLegSysPar;


typedef struct tag_SysParaHdr                   // 20 bytes
{
    __u8                Magic [8];              // "SYS_PARA"
    __u32               version;                // = 0x0100 (version ???)
    __u32               EraseChipFlag;          // 0 = don't, 1 = do.
    __u32               jtag;                   // not sure
}
rSysParaHdr;


typedef struct tag_A10_config_part_rec          // 104 bytes
{
    __u32               sizehi;
    __u32               sizelo;
    __u8                major_name [32];
    __u8                minor_name [32];
    __u8                unknown    [32];
} 
rA10_cpart;


typedef struct tag_A10_config_name_rec          // 97 bytes
{
    __u8                part_name   [32];
    __u8                file_name   [32];
    __u8                verify_name [32];
    __u8                empty_flag  [1];        // 0 = loaded, 1 = ignore
} 
rA10_cname;


typedef struct tag_A10_config_rec               // 5496 bytes
{
    rSysParaHdr         SysParaHdr;             // 0x0000
    rAWLegSysPar        DramInfo;               // 0x0014
    __u32               unk5;                   // 0c00C8
    __u32               unk6;                   // 0c00CC
    __u8                filler [1024];          // 0x00D0
    __u32               MBR_size;               // 0x04D0
    __u32               Part_count;             // 0x04D4
    rA10_cpart          Part [14];              // 0x04D8
    __u32               Name_count;             // 0x0A88
    rA10_cname          Name [14];              // 0x0A8C
    __u8                unknown [1438];         // 0x0FDA
}
__attribute__ ((packed)) rA10_ConfigRec;


typedef struct tag_A20_config_part_rec          // 104 bytes
{
    __u32               sizehi;
    __u32               sizelo;
    __u8                major_name [32];
    __u8                minor_name [32];
    __u8                unknown    [32];
} 
rA20_cpart;


typedef struct tag_A20_config_name_rec          // 97 bytes
{
    __u8                part_name   [32];
    __u8                file_name   [32];
    __u8                verify_name [32];
    __u8                empty_flag  [1];        // 0 = loaded, 1 = ignore
} 
rA20_cname;


typedef struct tag_A20_config_rec               // 10080 bytes
{
    rSysParaHdr         SysParaHdr;             // 0x0000
    rAWLegSysPar        DramInfo;               // 0x0014
    __u8                unk_5 [336];            // 0x00C8
    __u32               unk_6;                  // 0x0218   sometimes 0x08
    __u8                unk_7 [2048];           // 0x021C
    __u32               MBR_size;               // 0x0A1C
    __u32               Part_count;             // 0x0A20
    rA20_cpart          Part [32];              // 0x0A24
    __u8                fill [1024];            // 0x1724
    __u32               Name_count;             // 0x1B24
    rA20_cname          Name [32];              // 0x1B28
    __u8                unknown [24];           // 0x2748
}
__attribute__ ((packed)) rA20_ConfigRec;


/*

# U-boot mode (uboot_spare_head.boot_data.work_mode,0xE0 offset)
AWUBootWorkMode = {
  :boot                            => 0x0,  # normal start
  :usb_tool_product                => 0x4,  # if :action => :none, then reboots device
  :usb_tool_update                 => 0x8,
  :usb_product                     => 0x10, # FES mode
  :card_product                    => 0x11, # SD-card flash
  :usb_debug                       => 0x12, # FES mode with debug
  :usb_update                      => 0x20, # USB upgrade (automatically inits nand!)
  :outer_update                    => 0x21  # external disk upgrade
}


#FES STORAGE TYPE
FESIndex = {
  :dram                            => 0x0,
  :physical                        => 0x1,
  :log                             => 0x2,
# these below are usable on boot 1.0
  :nand                            => 0x2,
  :card                            => 0x3,
  :nand2                           => 0x20 # encrypted data write?
}


#FES run types (AWFELMessage->len)
AWRunContext = {
  :none                           => 0x0,
  :has_param                      => 0x1,
  :fet                            => 0x10,
  :gen_code                       => 0x20,
  :fed                            => 0x30
}




*/



typedef struct _boot_dram_para_t                // for H3, from /brandy/u-boot-2011.09/arch/arm/include/arm/archsub81w7/dram.h
{
    //normal configuration
    unsigned int        dram_clk;
    unsigned int        dram_type;      //dram_type         DDR2: 2             DDR3: 3             LPDDR2: 6   DDR3L: 31
    unsigned int        dram_zq;
    unsigned int        dram_odt_en;

    //control configuration
    unsigned int        dram_para1;
    unsigned int        dram_para2;

    //timing configuration
    unsigned int        dram_mr0;
    unsigned int        dram_mr1;
    unsigned int        dram_mr2;
    unsigned int        dram_mr3;
    unsigned int        dram_tpr0;
    unsigned int        dram_tpr1;
    unsigned int        dram_tpr2;
    unsigned int        dram_tpr3;
    unsigned int        dram_tpr4;
    unsigned int        dram_tpr5;
    unsigned int        dram_tpr6;

    //reserved for future use
    unsigned int        dram_tpr7;
    unsigned int        dram_tpr8;
    unsigned int        dram_tpr9;
    unsigned int        dram_tpr10;
    unsigned int        dram_tpr11;
    unsigned int        dram_tpr12;
    unsigned int        dram_tpr13;

}
__dram_para_t;


#define DRAM_MDFS_TABLE_PARA0   (2)
#define DRAM_MDFS_TABLE_PARA1   (10)

typedef struct __DRAM_PARA          // from linux-3.4/drivers/devfreq/dramfreq/sunxi-dramfreq.h
{
    //normal configuration
    unsigned int dram_clk;
    unsigned int dram_type;    //dram_type  DDR2: 2  DDR3: 3 LPDDR2: 6 DDR3L: 31
    unsigned int dram_zq;
    unsigned int dram_odt_en;

    //control configuration
    unsigned int dram_para1;
    unsigned int dram_para2;

    //timing configuration
    unsigned int dram_mr0;
    unsigned int dram_mr1;
    unsigned int dram_mr2;
    unsigned int dram_mr3;
    unsigned int dram_tpr0;
    unsigned int dram_tpr1;
    unsigned int dram_tpr2;
    unsigned int dram_tpr3;
    unsigned int dram_tpr4;
    unsigned int dram_tpr5;
    unsigned int dram_tpr6;

    //reserved for future use
    unsigned int dram_tpr7;
    unsigned int dram_tpr8;
    unsigned int dram_tpr9;
    unsigned int dram_tpr10;
    unsigned int dram_tpr11;
    unsigned int dram_tpr12;
    unsigned int dram_tpr13;

    unsigned int high_freq;

    unsigned int table[DRAM_MDFS_TABLE_PARA0][DRAM_MDFS_TABLE_PARA1];
} 
__dram_para_t_old;


class   FullName
  {
        char        *szName;
        char        *szFullName;
    public:
        /*void*/    FullName (char *name);
        /*void*/    ~FullName (void);
        char*       get (void);
  };


typedef struct tag_filenames
  {
    char                *ConfigRec;
    char                *DRAM_specs;
    char                *boot0_nand;
    char                *boot1_nand;
    char                *fed_nand;
    char                *fes;
    char                *fes_1_1;
    char                *fes_1_2;
    char                *fes_2;
    char                *fet_restore;
    char                *magic_cr_start;
    char                *magic_cr_end;
    char                *magic_de_start;
    char                *magic_de_end;
    char                *update_boot0;
    char                *update_boot1;
  }
rFileNames;


    char                szBasePath [512];

    bool                bShowURBs = true;
    bool                bEraseReqd = false;
    int                 rc;
    libusb_device_handle *handle = NULL;
    int                 detached_iface = -1;
    int                 forceable = 0;              // allow continue when error ?
    int                 readNAND = 0;               // -r = read NAND
    int                 writeNAND = 0;              // -w = write NAND
    char                NAND_FID [256]      = { "NAND.DAT" };
    int                 loadNAND = 0;               // -d = load NAND
    int                 part_cnt = 0;               // partitions to write
    char                part_name [16] [256];       // list of partition files
    uint                part_start [16];            // start sector for each partition
    uint                part_secs [16];             // sectors for each partition
    int                 errors = 0;                 // forced past halt ?
    int                 CB2_mode = 0;               // specials for CB2 ?
    int                 version = 0;                // 0x1610 = flash mode
    rAWSysPar           DramInfo;
    rNandInfo           NandInfo;
    unsigned int        MagicAddr   = 0x40360000;
    unsigned int        ConfigAddr  = 0x40A00000;
    rFileNames          *FN;
    rFileNames          FN_sun4i =
      {
                        .ConfigRec          = (char*) "sun4i/ConfigRec",
                        .DRAM_specs         = (char*) "sun4i/DRAM_specs",
                        .boot0_nand         = (char*) "sun4i/boot0_nand.bin",
                        .boot1_nand         = (char*) "sun4i/boot1_nand.fex",
                        .fed_nand           = (char*) "sun4i/fed_nand.axf",
                        .fes                = (char*) "sun4i/fes.fex",
                        .fes_1_1            = (char*) "sun4i/fes_1-1.fex",
                        .fes_1_2            = (char*) "sun4i/fes_1-2.fex",
                        .fes_2              = (char*) "sun4i/fes_2.fex",
                        .fet_restore        = (char*) "sun4i/fet_restore.axf",
                        .magic_cr_start     = (char*) "sun4i/magic_cr_start.fex",
                        .magic_cr_end       = (char*) "sun4i/magic_cr_end.fex",
                        .magic_de_start     = (char*) "sun4i/magic_de_start.fex",
                        .magic_de_end       = (char*) "sun4i/magic_de_end.fex",
                        .update_boot0       = (char*) "sun4i/update_boot0.axf",
                        .update_boot1       = (char*) "sun4i/update_boot1.axf"
      };
    rFileNames          FN_sun7i =
      {
                        .ConfigRec          = (char*) "sun7i/ConfigRec",
                        .DRAM_specs         = (char*) "sun7i/DRAM_specs",
                        .boot0_nand         = (char*) "sun7i/boot0_nand.bin",
                        .boot1_nand         = (char*) "sun7i/boot1_nand.fex",
                        .fed_nand           = (char*) "sun7i/fed_nand.axf",
                        .fes                = (char*) "sun7i/fes.fex",
                        .fes_1_1            = (char*) "sun7i/fes_1-1.fex",
                        .fes_1_2            = (char*) "sun7i/fes_1-2.fex",
                        .fes_2              = (char*) "sun7i/fes_2.fex",
                        .fet_restore        = (char*) "sun7i/fet_restore.axf",
                        .magic_cr_start     = (char*) "sun7i/magic_cr_start.fex",
                        .magic_cr_end       = (char*) "sun7i/magic_cr_end.fex",
                        .magic_de_start     = (char*) "sun7i/magic_de_start.fex",
                        .magic_de_end       = (char*) "sun7i/magic_de_end.fex",
                        .update_boot0       = (char*) "sun7i/update_boot0.axf",
                        .update_boot1       = (char*) "sun7i/update_boot1.axf"
      };


