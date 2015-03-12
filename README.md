	CTNandBoot

Sets Cubietruck (and now CubieBoard2 also) to allow Linux boot from NAND.

One problem with CTs is that one has to run PhoenixSuit and download an image (e.g. Linaro)
just to change some code so the CT to boot from NAND.

This program (called bootfix) will run on a CT (also i386 and probably other systems)
and will change a target CT so it will boot from NAND.  Only the 'hidden' NAND is changed.
This means the user will have to load the accessible NAND (MBR and partitions) later,
probably by booting from an SD card.  If the NAND has already been setup,
it won't be changed and the CT will boot automatically after the program has run.

The program requires some data files and all are included here.  The executable
'bootfix' included here will run on a CT.  Use 'make' to compile bootfix for other host machines.

To run this program, connect the target CT (must be in FEL mode), and then run ./bootfix
and wait till finished (less than one minute).  Most of the output is "URB xxxxxx"
lines which relate to records in the original USB monitor file.  There is a notes file
that lists my notes about these records.  Connecting a serial monitor to the target CT
will provide more details.

Booting from NAND requires more files in the boot partition than when booting from SD card.
For consistency, these extra files can be put in the SD card partition and will be ignored.

<pre>
The required files are:
	/boot.axf
	/boot.ini
	/uEnv.txt
	/script.bin
	/uImage
	/linux/linux.ini
	/linux/u-boot.bin
</pre>

uEnv.txt, script.bin, and uImage vary to suit the board and kernel.

With SD card boot, the bootloader is stored at sector 16 onwards.  This bootloader
uses uEnv.txt, script.bin, and uImage only.

With NAND boot, the bootloader runs boot.axf which uses boot.ini then /linux/linux.ini
then runs /linux/u-boot.bin.  This u-boot.bin uses uEnv.txt, script.bin, and uImage.

A skeleton boot partition is included.  The script.bin and uImage files must be added.
The uEnv.txt file will probably require customising.

