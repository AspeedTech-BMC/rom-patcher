/**
 * AST2600A2 CM3 boot - Flash layout
 * 
 *                 0000_0000 +--------------------+
 *                           | CA7 code:
 *                           |   movw r0, #0  ; r0[15:0] = 0x0000
 *                           |   movt r0, #1  ; r0[31:16] = 0x0001
 *                           |   mov  pc, r0  ; pc is 0x00010000 (64KB)
 *                           |
 *                 0000_0020 +--------------------+
 *                           | secure boot header:
 *                           | ...
 *                 0000_0038 |   patch code location (fixed value: CONFIG_OFFSET_PATCH_START)
 *                           | ...
 *                 0000_0040 +--------------------+
 *                           | reserved
 * CONFIG_OFFSET_PATCH_START +--------------------+
 *                           | patch code part1:
 *                           |   start code
 *                           |   uart5 init
 *                           |   jump to l_start
 *                           +--------------------+
 *                           | CM3 image header
 *                           | CM3 binary
 *                  l_start  +--------------------+
 *                           | patch code part2:
 *                           |   init DRAM
 *                           |   download CM3 binary to DRAM
 *                           |   reset CM3
 *                           |   end code (0x0000_000F)
 *                           +--------------------+
 *                           | ...
 *                0001_0000  +--------------------+
 *                           | CA7 u-boot binary
 *                           +--------------------+
*/
#include <stdio.h>
#include <string.h>
#include "opcode.h"
#include "bitops.h"
#include "config.h"
#include "ast2600.h"
#include "ddr4_init.h"
#include "bootloader.h"

struct sb_header {
	uint32_t key_location;
	uint32_t enc_img_addr;
	uint32_t img_size;
	uint32_t sign_location;
	uint32_t header_rev[2];
	uint32_t patch_location;	/* address of the rom patch */
	uint32_t checksum;
};

void uart_init(FILE *fp)
{
	wr_single(fp, UART_BASE + UART_LCR, 0x3);
	wr_single(fp, UART_BASE + UART_FCR, 0xc7);
	wr_single(fp, UART_BASE + UART_LCR, 0x83);

	wr_single(fp, UART_BASE + UART_DLL, 0x1);
	wr_single(fp, UART_BASE + UART_DLH, 0x0);

	wr_single(fp, UART_BASE + UART_LCR, 0x3);

	wr_single(fp, UART_BASE + UART_THR, 0xd);
	wr_single(fp, UART_BASE + UART_THR, 0xa);
}

void uart_putc(FILE *fp, uint8_t c)
{
	wr_single(fp, UART_BASE + UART_THR, c);
}

/**
 * @brief generate SPI Flash binary
 * 
 * including:
 *   - CA7 code (pc jump to 0x0001_0000)
 *   - secure boot header
 *   - patch code
 *   - CM3 binary
 * 
 * NOT including:
 *   - CA7 u-boot image: please use gen_cm3_boot_ca7.sh
*/
void gen_test_bin(void)
{
	FILE *fp, *fb;
	struct sb_header sbh;
	uint8_t data;
	uint32_t ca7_jmp_code[3] = 	{ 0xe3000000, 0xe3400001, 0xe1a0f000 };

	fp = fopen("test.bin", "wb+");
	if (!fp) {
	    printf("can not open test file: %s\n", "test.bin");
	    return;
	}
	fseek(fp, 0, SEEK_SET);
	fwrite(&ca7_jmp_code, 1, sizeof(ca7_jmp_code), fp);
	fseek(fp, CONFIG_SECURE_BOOT_HDR_START, SEEK_SET);

	fb = fopen(OUTPUT_BIN_NAME, "rb");
	if (!fb) {
	    printf("can not open dest file: %s\n", OUTPUT_BIN_NAME);
	    return;
	}
	fseek(fb, 0, SEEK_SET);
	
	memset(&sbh, 0, sizeof(sbh));
	sbh.patch_location = CONFIG_OFFSET_PATCH_START;
	sbh.img_size = 63 * 1024;
	fwrite(&sbh, 1, sizeof(sbh), fp);
	fseek(fp, CONFIG_OFFSET_PATCH_START, SEEK_SET);

	while (fread(&data, 1, sizeof(data), fb)) {
		fwrite(&data, 1, sizeof(data), fp);
	}

	fclose(fp);
	fclose(fb);
}

void parse_test_bin(void)
{
	FILE *fp;
	struct sb_header sbh;

	fp = fopen("test.bin", "rb");
	fseek(fp, 0x20, SEEK_CUR);

	fread(&sbh, 1, sizeof(sbh), fp);
	if (sbh.patch_location == 0) {
		printf("%s: no rom patch\n", __func__);
		fclose(fp);
		return;
	}

	printf("%s: secure boot header-> patch address %08x, size %08x\n", __func__, sbh.patch_location, sbh.img_size);
	fseek(fp, sbh.patch_location, SEEK_SET);
	parse_opcode(fp);
	fclose(fp);
}

int main()
{
	FILE *fp;
	fpos_t cm3_img_start;
	uint32_t size;
    int i, j;

	fp = fopen(OUTPUT_BIN_NAME, "wb+");
	if (!fp) {
	    printf("can not open dest file: %s\n", OUTPUT_BIN_NAME);
	    return -1;
	}
	fseek(fp, 0, SEEK_SET);

    /* ---------- start ---------- */ 
	start_code(fp);
	uart_init(fp);
	uart_putc(fp, '1');
	jmp_code(fp, "l_start");
	fgetpos(fp, &cm3_img_start);
	attach_cm3_binary(fp);
	
	declare_label(fp, "l_start");
	sdram_probe(fp);
	/* download and enable CM3 */
	copy_cm3(fp, cm3_img_start);
	enable_cm3(fp);
	uart_putc(fp, '6');
    quit_code(fp);

	print_labels();
	
	/* link label addresses and jump instructions */
	link_labels(fp);
	
	/* ---------- end ---------- */ 

	print_rom_patch(fp);

	fclose(fp);	

	/* test: generate whole SPI boot image */
	gen_test_bin();
	parse_test_bin();
	return 0;
}