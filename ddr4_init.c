#include "bitops.h"
#include "opcode.h"
#include "ddr4_ac.h"
#include "ddr4_phy.h"
#include "ast2600.h"
extern void uart_putc(FILE *fp, uint8_t c);
void sdrammc_common_init(FILE *fp)
{
    uint32_t buff[2] = { 0, 0 };

    wr_single(fp, MMC_BASE + 0x34, 0xc0);
    wr_single(fp, MMC_BASE + 0x08, 0x00440003);
    wr_single(fp, MMC_BASE + 0x38, 0x10 << 16);
    wr_single(fp, MMC_BASE + 0x3c, 0xffbbfff4);
    wr_code(fp, MMC_BASE + 0x40, 4, (uint32_t *)ddr_max_grant_params);

    wr_single(fp, MMC_BASE + 0x50, BIT(31));
    wr_single(fp, MMC_BASE + 0x54, 0x07ffffff);
    wr_single(fp, MMC_BASE + 0x70, 0);
    wr_single(fp, MMC_BASE + 0x74, 0x80000001);
    wr_code(fp, MMC_BASE + 0x78, 2, buff);
    wr_single(fp, MMC_BASE + 0x80, 0xffffffff);
    wr_single(fp, MMC_BASE + 0x84, 0);

    delay_code(fp, 600);

#ifdef CONFIG_ASPEED_DDR4_DUALX8
    wr_single(fp, MMC_BASE + 0x04, 0x37);
#else
    wr_single(fp, MMC_BASE + 0x04, 0x17);
#endif

	wr_code(fp, MMC_BASE + 0x10, ARRAY_SIZE(ddr4_ac_timing),
	    (uint32_t *)ddr4_ac_timing);
    wr_code(fp, MMC_BASE + 0x20, ARRAY_SIZE(ddr4_mr_setting),
	    (uint32_t *)ddr4_mr_setting);

    
}

void sdramphy_init(FILE *fp)
{
#if !defined(CONFIG_FPGA_ASPEED) && !defined(CONFIG_ASPEED_PALLADIUM)	
	uint32_t *start, *curr, n;
	int i;

	wr_single(fp, MMC_BASE + 0x60, 0x0);
	delay_code(fp, 10);

	curr = start = sdramphy_config; 
	while (*curr != DDR_PHY_TBL_END) {
		if (*curr == DDR_PHY_TBL_CHG_ADDR) {
			/* start and curr are excluded */
			n = curr - start - 1;
			wr_code(fp, *start, n, start + 1);
			start = ++curr;
		}
		curr++;
	}

	rmw_code(fp, PHY_BASE + 0x84, ~GENMASK(16, 0), DDR4_PHY_TRAIN_TRFC);
#endif
	
}

void sdramphy_kick_training(FILE *fp)
{
#if !defined(CONFIG_FPGA_ASPEED) && !defined(CONFIG_ASPEED_PALLADIUM)
	wr_single(fp, MMC_BASE + 0x60, BIT(2));
	delay_code(fp, 5);
	wr_single(fp, MMC_BASE + 0x60, BIT(2) | BIT(0));
	delay_code(fp, 1000);
	waiteq_code(fp, MMC_BASE + 0x60, BIT(0), BIT(0), 1);
#endif	
	
}

void sdramphy_check_status(FILE *fp)
{
#if !defined(CONFIG_FPGA_ASPEED) && !defined(CONFIG_ASPEED_PALLADIUM)
	jeq_code(fp, PHY_STS_BASE + 0x00, BIT(3), BIT(3), "l_sdramphy_train");
	jeq_code(fp, PHY_STS_BASE + 0x00, BIT(5), BIT(5), "l_sdramphy_train");

	/* the delay time is MAGIC.  shorter delay would somehow cause DRAM init fail */
	delay_code(fp, 3000);
	/* retrain if PHY_STS068[7:0] == 0 */
	jeq_code(fp, PHY_STS_BASE + 0x68, GENMASK(7, 0), 0, "l_sdramphy_train");

	/* retrain if PHY_STS07c[7:0] == 0 */
	jeq_code(fp, PHY_STS_BASE + 0x7c, GENMASK(7, 0), 0, "l_sdramphy_train");

	/* retrain if PHY_STS050[15:0] == 0 */
	jeq_code(fp, PHY_STS_BASE + 0x50, GENMASK(15, 0), 0, "l_sdramphy_train");

	/* retrain if PHY_STS050[31:16] == 0 */
	jeq_code(fp, PHY_STS_BASE + 0x50, GENMASK(31, 16), 0, "l_sdramphy_train");
#endif	
	
}

void sdrammc_init_ddr4(FILE *fp)
{
	sdramphy_init(fp);

	/* enable CKE, RESTETN_DIS */
	wr_single(fp, MMC_BASE + 0x34, 0xc1);
	delay_code(fp, 5);
	sdramphy_kick_training(fp);
	delay_code(fp, 500);
	
	/* DLL/ZQCL enable */
	wr_single(fp, MMC_BASE + 0x0c, BIT(6));

	/* set MR */
	wr_single(fp, MMC_BASE + 0x30, 0x00000007);
	wr_single(fp, MMC_BASE + 0x30, 0x0000000d);
	wr_single(fp, MMC_BASE + 0x30, 0x0000000b);
	wr_single(fp, MMC_BASE + 0x30, 0x00000009);
	wr_single(fp, MMC_BASE + 0x30, 0x00000005);
	wr_single(fp, MMC_BASE + 0x30, 0x00000003);
	wr_single(fp, MMC_BASE + 0x30, 0x00000011);

	/* enable refresh */
#if defined(CONFIG_FPGA_ASPEED) || defined(CONFIG_ASPEED_PALLADIUM)
	wr_single(fp, MMC_BASE + 0x0c, BIT(6) | BIT(0) | (0x5d << 8));
#else
	wr_single(fp, MMC_BASE + 0x0c, BIT(6) | BIT(0) | (0x5f << 8));
#endif

	waiteq_code(fp, MMC_BASE + 0x34, GENMASK(30, 28), 0, 1);

#if defined(CONFIG_FPGA_ASPEED) || defined(CONFIG_ASPEED_PALLADIUM)
	wr_single(fp, MMC_BASE + 0x0c, BIT(7) | BIT(5) | BIT(0) | (0x5d << 8) | (0x4000 << 16));
#else
	wr_single(fp, MMC_BASE + 0x0c, BIT(7) | BIT(5) | BIT(0) | (0x5f << 8) | (0x42aa << 16));
#endif

	wr_single(fp, MMC_BASE + 0x34, 0x7a3);
	delay_code(fp, 500);

#if defined(CONFIG_FPGA_ASPEED)
	/* toggle Vref training */
	setbit_code(fp, MMC_BASE + 0x2c, BIT(7));
	wr_single(fp, MMC_BASE + 0x30, BIT(4) | 0xd);
	clrbit_code(fp, MMC_BASE + 0x2c, BIT(7));
	wr_single(fp, MMC_BASE + 0x30, BIT(4) | 0xd);
#endif

	
}

void sdrammc_fpga_set_pll(FILE *fp)
{
#define AST_SCU_FPGA_STS        0x004
#define AST_SCU_FPGA_PLL        0x400

    wr_single(fp, SCU_BASE + AST_SCU_FPGA_PLL, 0x00000303);
	waiteq_code(fp, SCU_BASE + AST_SCU_FPGA_STS, 0x100, 0x100, 1);
	wr_single(fp, SCU_BASE + AST_SCU_FPGA_PLL, 0x00000103);

    
}

void sdrammc_search_read_window(FILE *fp)
{
#define var_win					(SRAM1_BASE + 0)

#ifdef CONFIG_ASPEED_PALLADIUM
	wr_single(fp, PHY_BASE + 0x00, 0x0000000c);
	
#endif

#ifdef CONFIG_FPGA_ASPEED
	wr_single(fp, SEARCH_RDWIN_ANCHOR_0, SEARCH_RDWIN_PTRN_0);
    wr_single(fp, SEARCH_RDWIN_ANCHOR_1, SEARCH_RDWIN_PTRN_1);
	wr_single(fp, PHY_BASE + 0x00, 0x0000000c);
	declare_label(fp, "l_check_value_start");
	sdrammc_fpga_set_pll(fp);
	jeq_code(fp, SEARCH_RDWIN_ANCHOR_0, GENMASK(31, 0), SEARCH_RDWIN_PTRN_0, "l_check_value_start");
	jeq_code(fp, SEARCH_RDWIN_ANCHOR_1, GENMASK(31, 0), SEARCH_RDWIN_PTRN_1, "l_check_value_start");

	wr_single(fp, var_win, 0);
	declare_label(fp, "l_cali_rd_win_start");
	sdrammc_fpga_set_pll(fp);
	add_code(fp, var_win, 1);
	jne_code(fp, var_win, GENMASK(31, 0), 256, "l_cali_rd_win_start");
#endif
	
}

void sdrammc_calc_size(FILE *fp)
{
	wr_single(fp, 0xc0100000, 0xdeadbeef);
	wr_single(fp, 0xa0100000, 0xfdeadbee);
	wr_single(fp, 0x90100000, 0xefdeadbe);
	wr_single(fp, 0x80100000, 0xeefdeadb);

	jeq_code(fp, 0xc0100000, GENMASK(31, 0), 0xdeadbeef, "l_size_2g");
	jeq_code(fp, 0xa0100000, GENMASK(31, 0), 0xfdeadbee, "l_size_1g");
	jeq_code(fp, 0x90100000, GENMASK(31, 0), 0xefdeadbe, "l_size_512m");
	
	//declare_label(fp, "l_size_256m");
	rmw_code(fp, MMC_BASE + 0x04, ~GENMASK(1, 0), 0x0);
	jmp_code(fp, "l_size_done_0");

	declare_label(fp, "l_size_2g");
	rmw_code(fp, MMC_BASE + 0x04, ~GENMASK(1, 0), 0x3);
	jmp_code(fp, "l_size_done_0");

	declare_label(fp, "l_size_1g");
	rmw_code(fp, MMC_BASE + 0x04, ~GENMASK(1, 0), 0x2);
	jmp_code(fp, "l_size_done_0");

	declare_label(fp, "l_size_512m");
	rmw_code(fp, MMC_BASE + 0x04, ~GENMASK(1, 0), 0x1);
	//jmp_code(fp, "l_size_done");

/* l_size_done_0 */
	declare_label(fp, "l_size_done_0");

	/* check VGA size: read SCU500[14:13] and write MMC04[3:2] */
	jeq_code(fp, STRAP_REG, GENMASK(14, 13), 0x3 << 13, "l_vga_size_64m");
	jeq_code(fp, STRAP_REG, GENMASK(14, 13), 0x2 << 13, "l_vga_size_32m");
	jeq_code(fp, STRAP_REG, GENMASK(14, 13), 0x1 << 13, "l_vga_size_16m");

	// VGA 8MB -> set to 16MB
	setbit_code(fp, STRAP_REG, 0x1 << 13);
	declare_label(fp, "l_vga_size_16m");
	rmw_code(fp, MMC_BASE + 0x04, ~GENMASK(3, 2), 0x1 << 2);
	jmp_code(fp, "l_size_done_1");

	declare_label(fp, "l_vga_size_32m");
	rmw_code(fp, MMC_BASE + 0x04, ~GENMASK(3, 2), 0x2 << 2);
	jmp_code(fp, "l_size_done_1");

	declare_label(fp, "l_vga_size_64m");
	rmw_code(fp, MMC_BASE + 0x04, ~GENMASK(3, 2), 0x3 << 2);
	jmp_code(fp, "l_size_done_1");
/* l_size_done_1 */
	declare_label(fp, "l_size_done_1");
	
}

void sdram_probe(FILE *fp)
{
	/* goto l_calc_size if DRAM is already initialized */
    jeq_code(fp, SCU_BASE + 0x100, BIT(6), BIT(6), "l_calc_size");
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
    declare_label(fp, "l_sdramphy_train");
	sdrammc_init_ddr4(fp);

#if defined(CONFIG_FPGA_ASPEED) || defined(CONFIG_ASPEED_PALLADIUM)
	sdrammc_search_read_window(fp);
#else
	waiteq_code(fp, PHY_BASE + 0x300, BIT(1), BIT(1), 1);
	sdramphy_check_status(fp);
#endif

	/* the delay time is MAGIC.  shorter delay would somehow cause DRAM init fail */
	delay_code(fp, 5000);

    declare_label(fp, "l_calc_size");
	sdrammc_calc_size(fp);

    /* DDR4 init end: set handshake bits */
    setbit_code(fp, SCU_BASE + 0x100, BIT(7) | BIT(6));
}