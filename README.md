CTNandBoot
==========

Sets Cubietruck to allow Linux boot from NAND.

One problem with CTs is that one has to run PhoenixSuit and download an image (e.g. Linaro) just to change some code so the CT to boot from NAND.

This program (called bootfix) will run on a CT (also i386 and probably other systems) and will change a target CT so it will boot from NAND.  Only the 'hidden' NAND is changed.  This means the user will have to load the accessible NAND (MBR and partitions) later, probably by booting from an SD card.  If the NAND has already been setup, it won't be changed and the CT will boot automatically after the program has run.

The program requires some data files and all are included here.  The executable 'bootfix' included here will run on a CT.  Use 'make' to compile bootfix for other host machines.

To run this program, connect the target CT (must be in FEL mode), and then run ./bootfix and wait till finished (less than one minute).  Most of the output is "URB xxxxxx" lines which relate to records in the original USB monitor file.  There is a notes file that lists my notes about these records.  Connecting a serial monitor to the target CT will provide more details.

This product has NOT been tested on animals.  Ha.

