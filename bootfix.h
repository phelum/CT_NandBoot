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
	rNandInfo			NandInfo;
//	uchar				DRAM_config [224];
	int					RAM_256MB_count = 8;		// 8 = 2GB = CubieTruck
	int					NAND_256MB_count = 32;		// 32 = 8GB = CubieTruck
	uint				MaxNANDKey = 0x760000;		// 4GB CubieBoard2

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

