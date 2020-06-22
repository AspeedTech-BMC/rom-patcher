#include <stdint.h>
#include <stdio.h>
#include "bitops.h"
#include "ast2600.h"
#include "opcode.h"

struct sb_header {
	uint32_t key_location;
	uint32_t enc_img_addr;
	uint32_t img_size;
	uint32_t sign_location;
	uint32_t header_rev[2];
	uint32_t patch_location;	/* address of the rom patch */
	uint32_t checksum;
};

struct cm3_image {
	uint32_t magic;
	uint32_t addr;				/* byte address */
	uint32_t size_dw;			/* size is in uint32_t */
};

// SPI Flash layout
// 2000_0000 - 2000_001F: reserved for ARM Cortex A7
// 2000_0020 - 2000_003F: secure boot header
// 2000_0040 ~          : ROM patch

void copy_cm3(FILE *fp)
{
	/* copy CM3 bootcode */
	///cm3_bin_offset = SB_HDR_SIZE_BYTE + sizeof(rom_code);
	log_jeq(fp, SBC_BASE + OTP_QSR, BIT(26), BIT(26), "l_copy_from_sram");
	//cp_code(fp, SPI_BASE + cm3_bin_offset, DRAM_BASE, CM3_BIN_SIZE_DW);
	log_jmp(fp, "l_copy_done");
	log_label(fp, "l_copy_from_sram");	
	//cp_code(fp, SRAM_BASE + cm3_bin_offset, DRAM_BASE, CM3_BIN_SIZE_DW);
	log_label(fp, "l_copy_done");
	
}

void enable_cm3(FILE *fp)
{
	wr_single(fp, SCU_BASE + 0xa00, 0);
	wr_single(fp, SCU_BASE + 0xa04, DRAM_BASE);
	wr_single(fp, SCU_BASE + 0xa48, 3);
	wr_single(fp, SCU_BASE + 0xa48, 1);
	wr_single(fp, SCU_BASE + 0xa08, DRAM_BASE + 0x00100000);
	wr_single(fp, SCU_BASE + 0xa0c, DRAM_BASE + 0x00200000);
	wr_single(fp, SCU_BASE + 0xa00, 2);
	delay_code(fp, 500);
	wr_single(fp, SCU_BASE + 0xa00, 0);
	wr_single(fp, SCU_BASE + 0xa00, 1);
}