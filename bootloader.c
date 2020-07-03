#include <stdint.h>
#include <stdio.h>
#include "config.h"
#include "bitops.h"
#include "ast2600.h"
#include "opcode.h"

/**
 * double-word (4-byte) aligned size
*/
#define DW_ALIGNED_DW_SIZE(byte_size)	(((byte_size) + 0x3) >> 2)

/**
 * double-word (4-byte) aligned size in byte
*/
#define DW_ALIGNED_BYTE_SIZE(byte_size)	(DW_ALIGNED_DW_SIZE(byte_size) << 2)

struct cm3_image_header {
	uint32_t magic;
	uint32_t src;				/* byte address */
	uint32_t dst;
	uint32_t size_dw;			/* size is in uint32_t */
};

// SPI Flash layout
// 2000_0000 - 2000_001F: reserved for ARM Cortex A7
// 2000_0020 - 2000_003F: secure boot header
// 2000_0040 ~          : ROM patch
uint32_t get_cm3_bin_size(void)
{
	FILE *fb;
	uint32_t size = 0;

	fb = fopen(CM3_BIN_NAME, "rb");
	if (!fb) {
	    printf("can not open cm3 bin file: %s\n", CM3_BIN_NAME);
		return 0;
	}

	fseek(fb, 0, SEEK_END);
	size = ftell(fb);
	fclose(fb);

	return size;
}

void attach_cm3_binary(FILE *fp)
{
	struct cm3_image_header hdr;
	FILE *fb;
	fpos_t fp_cur;
	uint8_t data;

	fgetpos(fp, &fp_cur);

	hdr.magic = 0x55667788;
	hdr.src = CONFIG_OFFSET_PATCH_START + vPOS(fp_cur) + sizeof(hdr);
	hdr.dst = DRAM_BASE;
	hdr.size_dw = DW_ALIGNED_DW_SIZE(get_cm3_bin_size());
	fwrite(&hdr, 1, sizeof(hdr), fp);

	fb = fopen(CM3_BIN_NAME, "rb");
	if (!fb) {
	    printf("can not open cm3 bin file: %s\n", CM3_BIN_NAME);
		return;
	}
	fseek(fb, 0, SEEK_SET);

	while (fread(&data, 1, sizeof(data), fb)) {
		fwrite(&data, 1, sizeof(data), fp);
	}
	fclose(fb);

	/* make pointer be 4-byte aligned */
	fgetpos(fp, &fp_cur);
	vPOS(fp_cur) = DW_ALIGNED_BYTE_SIZE(vPOS(fp_cur));
	fsetpos(fp, &fp_cur);

}

void copy_cm3(FILE *fp, fpos_t start)
{
	fpos_t fp_cur;
	struct cm3_image_header hdr;

	/* 
	1. backup current pointer
	2. set pointer to image start for reading the image header
	3. restore current pointer
	*/
	fgetpos(fp, &fp_cur);
	fsetpos(fp, &start);
	fread(&hdr, 1, sizeof(hdr), fp);
	fsetpos(fp, &fp_cur);

	/* copy CM3 bootcode */
	jeq_code(fp, SBC_BASE + OTP_QSR, BIT(26), BIT(26), "l_copy_from_sram");
	cp_code(fp, SPI_BASE + hdr.src, hdr.dst, hdr.size_dw);
	jmp_code(fp, "l_copy_done");
	declare_label(fp, "l_copy_from_sram");	
	cp_code(fp, SRAM_BASE + hdr.src, hdr.dst, hdr.size_dw);
	declare_label(fp, "l_copy_done");
	
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
#if 0	
	wr_single(fp, SCU_BASE + 0xa00, 1);
#endif	
}