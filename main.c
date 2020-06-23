#include <stdio.h>
#include <string.h>
#include "opcode.h"
#include "bitops.h"
#include "config.h"
#include "ast2600.h"
#include "ddr4_init.h"
#include "bootloader.h"
/**
 *                 0000_0000 +--------------------+
 *                           | ARM CA7 code
 *                 0000_0020 +--------------------+
 *                           | secure boot header
 *                           | ...
 *                 0000_0038 | patch code location (fixed value: CONFIG_OFFSET_PATCH_START)
 *                           | ...
 *                 0000_0040 +--------------------+
 *                           | reserved
 * CONFIG_OFFSET_PATCH_START +--------------------+
 *                           | start code
 *                           | jump to l_start
 *                           +--------------------+
 *                           | CM3 image header
 *                           | CM3 binary
 *                  l_start  +--------------------+
 *                           | init DRAM
 *                           | download CM3 binary to DRAM
 *                           | enable CM3
 *                           | end code (0x0000_000F)
 *                           +--------------------+
*/
int main()
{
	FILE *fp;
	fpos_t cm3_img_start;
	uint32_t size;
    int i, j;

	fp = fopen("rom_patch.bin", "wb+");
	if (!fp) {
	    printf("can not open dest file: rom_patch.bin\n");
	    return -1;
	}
	fseek(fp, 0, SEEK_SET);

    /* ---------- start ---------- */ 
	start_code(fp);
	log_jmp(fp, "l_start");
	fgetpos(fp, &cm3_img_start);
	attach_cm3_code(fp);
	
	log_label(fp, "l_start");
	/* goto l_calc_size if DRAM is already initialized */
    log_jeq(fp, SCU_BASE + 0x100, BIT(6), BIT(6), "l_calc_size");

	/* set MPLL */
    rmw_code(fp, MPLL_REG, ~(BIT(24) | GENMASK(22, 0)),
	     BIT(25) | BIT(23) | MPLL_FREQ_400M);
    wr_single(fp, MPLL_EXT_REG, MPLL_EXT_400M);
    delay_code(fp, 100);
    rmw_code(fp, MPLL_REG, GENMASK(31, 0), ~(BIT(25) | BIT(23)));
    waiteq_code(fp, MPLL_EXT_REG, BIT(31), BIT(31), 1);

	/* ast2600_sdrammc_unlock */
    wr_single(fp, MMC_BASE, MMC_UNLOCK_KEY);
    waiteq_code(fp, MMC_BASE, BIT(0), BIT(0), 1);

	/* DDR4 init start */
	sdrammc_common_init(fp);
    log_label(fp, "l_sdramphy_train");
	sdrammc_init_ddr4(fp);

#if defined(CONFIG_FPGA_ASPEED) || defined(CONFIG_ASPEED_PALLADIUM)
	sdrammc_search_read_window(fp);
#else
	waiteq_code(fp, PHY_BASE + 0x300, BIT(1), BIT(1), 1);
	sdramphy_check_status(fp);
#endif

    log_label(fp, "l_calc_size");
	sdrammc_calc_size(fp);

    /* DDR4 init end: set handshake bits */
    setbit_code(fp, SCU_BASE + 0x100, BIT(7) | BIT(6));

	/* download and enable CM3 */
	copy_cm3(fp, cm3_img_start);
	enable_cm3(fp);
    quit_code(fp);

	print_labels();
	
	/* link label addresses and jump instructions */
	link_labels(fp);
	
	/* ---------- end ---------- */ 

	print_rom_patch(fp);

	fclose(fp);	
}