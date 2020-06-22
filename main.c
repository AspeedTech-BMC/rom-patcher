#include <stdio.h>
#include <string.h>
#include "opcode.h"
#include "bitops.h"
#include "config.h"
#include "ast2600.h"
#include "ddr4_init.h"

int main()
{
    int i, j, size;
	uint32_t cm3_bin_offset, data;
	FILE *fp;

	fp = fopen("rom_patch.bin", "wb+");
	if (!fp) {
	    printf("can not open dest file: rom_patch.bin\n");
	    return -1;
	}
	fseek(fp, 0, SEEK_SET);

	log_label(fp, "l_start");
	start_code(fp);
    /* ---------- start ---------- */ 
	/* goto check_size if DRAM is already initialized */
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

    /* set handshake bits */
    setbit_code(fp, SCU_BASE + 0x100, BIT(7) | BIT(6));
    quit_code(fp);

#if 0
	/* copy CM3 bootcode */
	cm3_bin_offset = SB_HDR_SIZE_BYTE + sizeof(rom_code);
	log_jeq(ptr, SBC_BASE + OTP_QSR, BIT(26), BIT(26), "l_copy_from_sram");
	cp_code(ptr, SPI_BASE + cm3_bin_offset, DRAM_BASE, CM3_BIN_SIZE_DW);
	log_jmp(ptr, "l_copy_done");
	log_label(ptr, "l_copy_from_sram");	
	cp_code(ptr, SRAM_BASE + cm3_bin_offset, DRAM_BASE, CM3_BIN_SIZE_DW);
	log_label(ptr, "l_copy_done");
	
	/* enable CM3 */
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
    /* ---------- end ---------- */
    log_label(fp, "l_end");
#endif

	print_labels();
	link_labels(fp);
	print_rom_patch(fp);

	fclose(fp);	
}