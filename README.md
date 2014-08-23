CTNandBoot
==========

Sets Cubietruck to allow Linux boot from NAND.

One problem I've found with CTs is that I have to run PhoenixSuit and download an image (e.g. Linaro) just to change the boot code so I can get the CT to boot from NAND.

This program (called bootfix) will run on a CT (also i386 and probably other systems) and will change the boot code in another CT so it will boot from NAND.  Only the 'hidden' NAND is changed.  This means the user will have to load the accessible NAND (MBR and partitions) later, probably by booting from an SD card.  If the NAND has already been setup, it won't be changed and the CT will boot automatically after the program has run.

The program requires some data files and all are included here.

To run this program, connect the target CT (must be in FEL mode), and then run ./bootfix and wait till finished (less than one minute).  Most of the output is "URB xxxxxx" lines which relate to records in the original USB monitor file.  There is a notes file that lists my notes about these records.  Connecting a serial monitor to the target CT will provide more details.

