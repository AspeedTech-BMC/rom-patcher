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

    log_label(fp, "l_calc_size");
    sdrammc_calc_size(fp);

    /* set handshake bits */
    setbit_code(fp, SCU_BASE + 0x100, BIT(7) | BIT(6));
    quit_code(fp);
    //fclose(fp);


#if 0

    ptr = sdrammc_common_init(ptr);
    log_label(ptr, "l_sdramphy_train");
	ptr = sdrammc_init_ddr4(ptr);
#if defined(CONFIG_FPGA_ASPEED) || defined(CONFIG_ASPEED_PALLADIUM)
	ptr = sdrammc_search_read_window(ptr);
#else
	ptr = waiteq_code(ptr, PHY_BASE + 0x300, BIT(1), BIT(1), 1);
	ptr = sdramphy_check_status(ptr);
#endif
    log_label(ptr, "l_calc_size");
	ptr = sdrammc_calc_size(ptr);

	/* set handshake bits */
	ptr = setbit_code(ptr, SCU_BASE + 0x100, BIT(7) | BIT(6));

	/* copy CM3 bootcode */
	cm3_bin_offset = SB_HDR_SIZE_BYTE + sizeof(rom_code);
	ptr = log_jeq(ptr, SBC_BASE + OTP_QSR, BIT(26), BIT(26), "l_copy_from_sram");
	ptr = cp_code(ptr, SPI_BASE + cm3_bin_offset, DRAM_BASE, CM3_BIN_SIZE_DW);
	ptr = log_jmp(ptr, "l_copy_done");
	log_label(ptr, "l_copy_from_sram");	
	ptr = cp_code(ptr, SRAM_BASE + cm3_bin_offset, DRAM_BASE, CM3_BIN_SIZE_DW);
	log_label(ptr, "l_copy_done");
	
	/* enable CM3 */
	ptr = wr_single(ptr, SCU_BASE + 0xa00, 0);
	ptr = wr_single(ptr, SCU_BASE + 0xa04, DRAM_BASE);
	ptr = wr_single(ptr, SCU_BASE + 0xa48, 3);
	ptr = wr_single(ptr, SCU_BASE + 0xa48, 1);
	ptr = wr_single(ptr, SCU_BASE + 0xa08, DRAM_BASE + 0x00100000);
	ptr = wr_single(ptr, SCU_BASE + 0xa0c, DRAM_BASE + 0x00200000);
	ptr = wr_single(ptr, SCU_BASE + 0xa00, 2);
	ptr = delay_code(ptr, 500);
	ptr = wr_single(ptr, SCU_BASE + 0xa00, 0);
	ptr = wr_single(ptr, SCU_BASE + 0xa00, 1);
    /* ---------- end ---------- */
    log_label(ptr, "l_end");
    ptr = quit_code(ptr);
#endif

    
	print_labels();
	link_labels(fp);
	print_rom_patch(fp);

	fclose(fp);	
}