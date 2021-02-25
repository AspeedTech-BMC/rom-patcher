/**
 * AST2605 SSP(CM3) boot - Flash layout
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

void attach_ca7_jump_code(FILE *fp)
{
	uint32_t ca7_jmp_code[3] = 	{ 0xe3000000, 0xe3400001, 0xe1a0f000 };

	fwrite(&ca7_jmp_code, 1, sizeof(ca7_jmp_code), fp);
}

void parse_boot_image(void)
{
	FILE *fp;
	struct sb_header sbh;

	fp = fopen("boot.bin", "rb");
	fseek(fp, CONFIG_SECURE_BOOT_HDR_START, SEEK_CUR);

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

//int main()
int main(int argc, char *argv[])
{
	FILE *fp;
	fpos_t cm3_img_start;
	uint32_t size;
	int i, j;
	char *cm3_bin_name = CM3_BIN_NAME;

	struct sb_header sbh;

	/* parsing input args */
	if (argc > 1)
		cm3_bin_name = argv[1];
		        
	printf("cm3 bin name = %s\n", cm3_bin_name);


	fp = fopen("boot.bin", "wb+");
	if (!fp) {
	    printf("can not open dest file: %s\n", "boot.bin");
	    return -1;
	}
	fseek(fp, 0, SEEK_SET);

	/* ---------- attach CA7 jump code ---------- */
	attach_ca7_jump_code(fp);
	fseek(fp, CONFIG_SECURE_BOOT_HDR_START, SEEK_SET);

	/* ---------- attach secure boot header ---------- */
	memset(&sbh, 0, sizeof(sbh));
	sbh.patch_location = CONFIG_OFFSET_PATCH_START;
	sbh.img_size = 63 * 1024;
	fwrite(&sbh, 1, sizeof(sbh), fp);
	fseek(fp, CONFIG_OFFSET_PATCH_START, SEEK_SET);

	/* ---------- patch code header ---------- */ 
	start_code(fp);
	//uart_init(fp);
	uart_putc(fp, '\r');
	uart_putc(fp, '\n');
	uart_putc(fp, 'A');
	uart_putc(fp, 'S');
	uart_putc(fp, 'T');
	uart_putc(fp, '2');
	uart_putc(fp, '6');
	uart_putc(fp, '0');
	uart_putc(fp, '5');
	jmp_code(fp, "l_start");
	
	/* ---------- CM3 image ---------- */ 
	fgetpos(fp, &cm3_img_start);
	attach_cm3_binary(fp, cm3_bin_name);
	
	/* ---------- patch code start ---------- */ 
	declare_label(fp, "l_start");
	sdram_probe(fp);
	copy_cm3(fp, cm3_img_start);
	enable_cm3(fp);
	/* ---------- patch code end ---------- */ 
	quit_code(fp);

	print_labels();
	
	/* link label addresses and jump instructions */
	link_labels(fp);
	
	/* ---------- end ---------- */ 

	print_rom_patch(fp);

	fclose(fp);	

	/* test: parse commands */
	parse_boot_image();
	return 0;
}
