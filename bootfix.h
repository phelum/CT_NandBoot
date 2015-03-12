typedef unsigned char uchar;
typedef unsigned int  uint;

#define PutNulls(b,l) memset (b, 0, l)


	bool				bShowURBs = true;
	int					rc;
	libusb_device_handle *handle = NULL;
	int 				detached_iface = -1;
	int					forceable = 0;				// allow continue when error ?
	int					errors = 0;					// forced past halt ?
	int					CB2_mode = 0;				// specials for CB2 ?
	int					version = 0;				// 0x1610 = flash mode
	uchar				DRAM_config [172];

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

