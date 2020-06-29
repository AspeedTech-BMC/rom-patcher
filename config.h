#ifndef _CONFIG_H_
#define _CONFIG_H_
#ifndef __APPLE__
#define LINUX_BUILD
#endif
#define CONFIG_SYS_SDRAM_BASE   0x80000000
#define CONFIG_FPGA_ASPEED
//#define CONFIG_ASPEED_PALLADIUM
//#define CONFIG_ASPEED_DDR4_DUALX8
#define CONFIG_ASPEED_DDR4_1600
//#define CONFIG_ASPEED_DDR4_800

/* memory offsets */
#define CONFIG_SECURE_BOOT_HDR_START	0x20
#define CONFIG_OFFSET_PATCH_START		0x50

#define OUTPUT_BIN_NAME		"rom_patch.bin"
#ifdef CONFIG_FPGA_ASPEED
#define CM3_BIN_NAME		"ast2600_ssp_fpga.bin"
#else
#define CM3_BIN_NAME		"ast2600_ssp.bin"
#endif
#endif /* end of "#ifndef _CONFIG_H_" */
