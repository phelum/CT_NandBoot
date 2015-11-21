typedef unsigned char uchar;
typedef unsigned int  uint;

#define PutNulls(b,l) memset (b, 0, l)


typedef struct tag_NandInfo {
	uint				u10000200;
	uint				zeroes [6];
	uint				one_0;
	uint				one_1;					// offset 0x20
	uint				ChipCnt;				// 0x01
	uint				ChipConnectInfo;		// 0x01
	uint				RbCnt;					// 0x01
	uint				RbConnectMode;			// 0x01
	uint				BankCntPerChip;			// 0x01
	uint				DieCntPerChip;			// 0x01
	uint				PlaneCountPerDie;		// 0x02
	uint				SectorCountPerPage;		// 0x10
	uint				PageCountPerPhyBlk;		// 0x0100
	uint				BlockCountPerDie;		// 0x0800
	uint				OperationOpt;			// 0x1188
	uint				FrequencePar;			// 0x1E
	uint				EccMode;				// 0x03
	uchar				NandChipID [8];			// 0xDA94D7AD 0xFFFFFFFF
	uint				ValidBlockRatio;		// 0x03B0	
	uint				GoodBlockRatio;			// 0x03B0	
	uint				ReadRetryType;			// 0x010604
	uint				DDRType;				// 0x0
	uint				unk_c [24];
	uint				SectorCount;			// 0x760000	
	uint				unk_d [11];
}__attribute__ ((packed)) rNandInfo;


typedef struct tag_DramInfo {
	uint				zeroes [6];
	uint				unk_0;					// 0x007C4AC1
	uint				unk_1;					// 0
	uint				dram_baseaddr;			// 0x40000000
	uint				dram_clk;				// 0x01B0
	uint				dram_type;				// 0x03
	uint				dram_rank_num;			// 0x01
	uint				dram_chip_density;		// 0x1000
	uint				dram_io_width;			// 0x0010
	uint				dram_bus_width;			// 0x20
	uint				dram_cas;				// 0x09
	uint				dram_zq;				// 0x7F
	uint				dram_odt_en;			// 0x00
	uint				dram_size;				// 0x0400	MB
	uint				dram_tpr0;				// 0x42D899B7
	uint				dram_tpr1;				// 0xA090
	uint				dram_tpr2;				// 0x022A00
	uint				dram_tpr3;				// 0x00	
	uint				dram_tpr4;				// 0x01
	uint				dram_tpr5;				// 0x00
	uint				dram_emr1;				// 0x04
	uint				dram_emr2;				// 0x10	
	uint				dram_emr3;				// 0x00
	uint				spare [36];
}__attribute__ ((packed)) rDramInfo;



	bool				bShowURBs = true;
	int					rc;
	libusb_device_handle *handle = NULL;
	int 				detached_iface = -1;
	int					forceable = 0;				// allow continue when error ?
	int					readNAND = 0;				// -r = read NAND
	int					writeNAND = 0;				// -w = write NAND
	char				NAND_FID [256]		= { "NAND.DAT" };
	int					loadNAND = 0;				// -d = load NAND
	int					part_cnt = 0;				// partitions to write
	char				part_name [16] [256];		// list of partition files
	uint				part_secs [16];				// sectors for each partition
	int					errors = 0;					// forced past halt ?
	int					CB2_mode = 0;				// specials for CB2 ?
	int					version = 0;				// 0x1610 = flash mode
	rDramInfo			DramInfo;
	rNandInfo			NandInfo;

	char				*FN_DRAM_specs		= (char*) "pt1_000063";
	char				*FN_fes_1_1			= (char*) "fes_1-1.fex";
	char				*FN_fes_1_2			= (char*) "fes_1-2.fex";
	char				*FN_CRC_table		= (char*) "pt1_000147";
	char				*FN_fes				= (char*) "fes.fex";
	char				*FN_fes_2			= (char*) "fes_2.fex";
	char				*FN_magic_de_start	= (char*) "magic_de_start.fex";
	char				*FN_magic_de_end	= (char*) "magic_de_end.fex";
	char				*FN_magic_cr_start	= (char*) "magic_cr_start.fex";
	char				*FN_magic_cr_end	= (char*) "magic_cr_end.fex";
	char				*FN_fed_nand		= (char*) "fed_nand.axf";
	char				*FN_boot0_nand		= (char*) "boot0_nand.bin";
	char				*FN_boot1_nand		= (char*) "boot1_nand.fex";
	char				*FN_update_boot0	= (char*) "update_boot0.axf";
	char				*FN_update_boot1	= (char*) "update_boot1.axf";
	char				*FN_fet_restore		= (char*) "fet_restore.axf";

