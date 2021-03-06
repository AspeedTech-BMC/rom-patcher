#ifndef _CONFIG_H_
#define _CONFIG_H_
#define CONFIG_SYS_SDRAM_BASE   0x80000000
//#define CONFIG_FPGA_ASPEED
//#define CONFIG_ASPEED_PALLADIUM
//#define CONFIG_ASPEED_DDR4_DUALX8
#define CONFIG_ASPEED_DDR4_1600
//#define CONFIG_ASPEED_DDR4_800

/**
 * secure boot header offset on SPI Flash
*/
#define CONFIG_SECURE_BOOT_HDR_START	0x20

/**
 * patch code offset on SPI Flash
*/
#define CONFIG_OFFSET_PATCH_START	0x50

/**
 * the destination address of the CM3 image
*/
#define CONFIG_CM3_DEST_ADDR		CONFIG_SYS_SDRAM_BASE

#ifdef CONFIG_FPGA_ASPEED
#define CM3_BIN_NAME		"ast2600_ssp_fpga.bin"
#else
#define CM3_BIN_NAME		"zephyr.bin"
#endif

/* file position value */
#ifdef __APPLE__		
#define vPOS(pos)	pos
#else
#define vPOS(pos)	pos.__pos
#endif
#endif /* end of "#ifndef _CONFIG_H_" */
