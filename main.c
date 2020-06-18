#include <stdio.h>
#include <string.h>
#include "opcode.h"
#include "bitops.h"
#include "config.h"
#include "ddr4_ac.h"
#include "ddr4_phy.h"

#define SCU_BASE 		0x1e6e2000
#define MPLL_REG 		(SCU_BASE + 0x220)
#define MPLL_EXT_REG 	(SCU_BASE + 0x224)
#define STRAP_REG		(SCU_BASE + 0x500)

#define MPLL_FREQ_400M 	0x0008405f
#define MPLL_EXT_400M 	0x00000031

#define MMC_BASE 		0x1e6e0000
#define PHY_BASE 		(MMC_BASE + 0x100)
#define PHY_STS_BASE	(MMC_BASE + 0x400)
#define MMC_UNLOCK_KEY 	0xfc600309

typedef struct label_s {
    char name[32];
    int offset;
} label_t;

typedef struct rom_lables_s {
    label_t labels[32];
    int count;
} rom_labels_t;

/* log "jump" instructions */
typedef struct log_jmp_s {
    rom_op_jmp_t *log[32];
    char label[32][32];
    int count;
} log_jmp_t;

/**
 * global data
*/
uint32_t rom_code[512];
rom_labels_t rom_labels = { .count = 0 };

log_jmp_t jmp_list = { .count = 0 };

void log_label(uint32_t *curr_ptr, char *name)
{
    label_t *lab = &rom_labels.labels[rom_labels.count++];

    strcpy(&lab->name[0], name);
    lab->offset = (int)curr_ptr - (int)rom_code;
}

uint32_t *log_jeq(uint32_t *ptr, uint32_t addr, uint32_t mask, uint32_t target,
		  char *label_name)
{
    jmp_list.log[jmp_list.count] = (rom_op_jmp_t *)ptr;
    strcpy(jmp_list.label[jmp_list.count++], label_name);

    ptr = jeq_code(ptr, addr, mask, target, 0);
    return ptr;
}

uint32_t *log_jne(uint32_t *ptr, uint32_t addr, uint32_t mask, uint32_t target,
		  char *label_name)
{
    jmp_list.log[jmp_list.count] = (rom_op_jmp_t *)ptr;
    strcpy(jmp_list.label[jmp_list.count++], label_name);

    ptr = jne_code(ptr, addr, mask, target, 0);
    return ptr;
}

uint32_t *log_jmp(uint32_t *ptr, char *label_name)
{
    jmp_list.log[jmp_list.count] = (rom_op_jmp_t *)ptr;
    strcpy(jmp_list.label[jmp_list.count++], label_name);

    ptr = jne_code(ptr, SCU_BASE + 0x4, 0, 0xffffffff, 0);
    return ptr;
}

uint32_t *sdrammc_common_init(uint32_t *ptr)
{
    uint32_t buff[2] = { 0, 0 };

    ptr = wr_single(ptr, MMC_BASE + 0x34, 0xc0);
    ptr = wr_single(ptr, MMC_BASE + 0x08, 0x00440003);
    ptr = wr_single(ptr, MMC_BASE + 0x38, 0x10 << 16);
    ptr = wr_single(ptr, MMC_BASE + 0x3c, 0xffbbfff4);
    ptr = wr_code(ptr, MMC_BASE + 0x40, 4, (uint32_t *)ddr_max_grant_params);

    ptr = wr_single(ptr, MMC_BASE + 0x50, BIT(31));
    ptr = wr_single(ptr, MMC_BASE + 0x54, 0x07ffffff);
    ptr = wr_single(ptr, MMC_BASE + 0x70, 0);
    ptr = wr_single(ptr, MMC_BASE + 0x74, 0x80000001);
    ptr = wr_code(ptr, MMC_BASE + 0x78, 2, buff);
    ptr = wr_single(ptr, MMC_BASE + 0x80, 0xffffffff);
    ptr = wr_single(ptr, MMC_BASE + 0x84, 0);

    ptr = delay_code(ptr, 600);

#ifdef CONFIG_ASPEED_DDR4_DUALX8
    ptr = wr_single(ptr, MMC_BASE + 0x04, 0x37);
#else
    ptr = wr_single(ptr, MMC_BASE + 0x04, 0x17);
#endif

    ptr = wr_code(ptr, MMC_BASE + 0x10, ARRAY_SIZE(ddr4_ac_timing),
		  (uint32_t *)ddr4_ac_timing);
    ptr = wr_code(ptr, MMC_BASE + 0x20, ARRAY_SIZE(ddr4_mr_setting),
		  (uint32_t *)ddr4_mr_setting);

    return ptr;
}

uint32_t *sdramphy_init(uint32_t *ptr)
{
#if !defined(CONFIG_FPGA_ASPEED) && !defined(CONFIG_ASPEED_PALLADIUM)	
	uint32_t *start, *curr, n;
	int i;

	ptr = wr_single(ptr, MMC_BASE + 0x60, 0x0);
	ptr = delay_code(ptr, 10);

	curr = start = sdramphy_config; 
	while (*curr != DDR_PHY_TBL_END) {
		if (*curr == DDR_PHY_TBL_CHG_ADDR) {
			/* start and curr are excluded */
			n = curr - start - 1;
			ptr = wr_code(ptr, *start, n, start + 1);
			start = ++curr;
		}
		curr++;
	}

	ptr = rmw_code(ptr, PHY_BASE + 0x84, ~GENMASK(16, 0), DDR4_PHY_TRAIN_TRFC);
#endif
	return ptr;
}

uint32_t *sdramphy_kick_training(uint32_t *ptr)
{
#if !defined(CONFIG_FPGA_ASPEED) && !defined(CONFIG_ASPEED_PALLADIUM)
	ptr = wr_single(ptr, MMC_BASE + 0x60, BIT(2));
	ptr = delay_code(ptr, 5);
	ptr = wr_single(ptr, MMC_BASE + 0x60, BIT(2) | BIT(0));
	ptr = delay_code(ptr, 1000);
	ptr = waiteq_code(ptr, MMC_BASE + 0x60, BIT(0), BIT(0), 1);
#endif	
	return ptr;
}

uint32_t *sdramphy_check_status(uint32_t *ptr)
{
#if !defined(CONFIG_FPGA_ASPEED) && !defined(CONFIG_ASPEED_PALLADIUM)
	ptr = log_jeq(ptr, PHY_STS_BASE + 0x00, BIT(3), BIT(3), "l_sdramphy_train");
	ptr = log_jeq(ptr, PHY_STS_BASE + 0x00, BIT(5), BIT(5), "l_sdramphy_train");

	/* retrain if PHY_STS068[7:0] == 0 */
	ptr = log_jeq(ptr, PHY_STS_BASE + 0x68, GENMASK(7, 0), 0, "l_sdramphy_train");

	/* retrain if PHY_STS07c[7:0] == 0 */
	ptr = log_jeq(ptr, PHY_STS_BASE + 0x7c, GENMASK(7, 0), 0, "l_sdramphy_train");

	/* retrain if PHY_STS050[15:0] == 0 */
	ptr = log_jeq(ptr, PHY_STS_BASE + 0x50, GENMASK(15, 0), 0, "l_sdramphy_train");

	/* retrain if PHY_STS050[31:16] == 0 */
	ptr = log_jeq(ptr, PHY_STS_BASE + 0x50, GENMASK(31, 16), 0, "l_sdramphy_train");
#endif	
	return ptr;
}

uint32_t *sdrammc_init_ddr4(uint32_t *ptr)
{
	ptr = sdramphy_init(ptr);

	/* enable CKE, RESTETN_DIS */
	ptr = wr_single(ptr, MMC_BASE + 0x34, 0xc1);
	ptr = delay_code(ptr, 5);
	ptr = sdramphy_kick_training(ptr);
	ptr = delay_code(ptr, 500);
	
	/* DLL/ZQCL enable */
	ptr = wr_single(ptr, MMC_BASE + 0x0c, BIT(6));

	/* set MR */
	ptr = wr_single(ptr, MMC_BASE + 0x30, 0x00000007);
	ptr = wr_single(ptr, MMC_BASE + 0x30, 0x0000000d);
	ptr = wr_single(ptr, MMC_BASE + 0x30, 0x0000000b);
	ptr = wr_single(ptr, MMC_BASE + 0x30, 0x00000009);
	ptr = wr_single(ptr, MMC_BASE + 0x30, 0x00000005);
	ptr = wr_single(ptr, MMC_BASE + 0x30, 0x00000003);
	ptr = wr_single(ptr, MMC_BASE + 0x30, 0x00000011);

	/* enable refresh */
#if defined(CONFIG_FPGA_ASPEED) || defined(CONFIG_ASPEED_PALLADIUM)
	ptr = wr_single(ptr, MMC_BASE + 0x0c, BIT(6) | BIT(0) | (0x5d << 8));
#else
	ptr = wr_single(ptr, MMC_BASE + 0x0c, BIT(6) | BIT(0) | (0x5f << 8));
#endif

	ptr = waiteq_code(ptr, MMC_BASE + 0x34, GENMASK(30, 28), 0, 1);

#if defined(CONFIG_FPGA_ASPEED) || defined(CONFIG_ASPEED_PALLADIUM)
	ptr = wr_single(ptr, MMC_BASE + 0x0c, BIT(7) | BIT(5) | BIT(0) | (0x5d << 8) | (0x4000 << 16));
#else
	ptr = wr_single(ptr, MMC_BASE + 0x0c, BIT(7) | BIT(5) | BIT(0) | (0x5f << 8) | (0x42aa << 16));
#endif

	ptr = wr_single(ptr, MMC_BASE + 0x34, 0x7a3);
	ptr = delay_code(ptr, 500);

#if defined(CONFIG_FPGA_ASPEED)
	/* toggle Vref training */
	ptr = setbit_code(ptr, MMC_BASE + 0x2c, BIT(7));
	ptr = wr_single(ptr, MMC_BASE + 0x30, BIT(4) | 0xd);
	ptr = clrbit_code(ptr, MMC_BASE + 0x2c, BIT(7));
	ptr = wr_single(ptr, MMC_BASE + 0x30, BIT(4) | 0xd);
#endif

	return ptr;
}

uint32_t *sdrammc_fpga_set_pll(uint32_t *ptr)
{
#define AST_SCU_FPGA_STS        0x004
#define AST_SCU_FPGA_PLL        0x400

	ptr = wr_single(ptr, SCU_BASE + AST_SCU_FPGA_PLL, 0x00000303);
	ptr = waiteq_code(ptr, SCU_BASE + AST_SCU_FPGA_STS, 0x100, 0x100, 1);
	ptr = wr_single(ptr, SCU_BASE + AST_SCU_FPGA_PLL, 0x00000103);

	return ptr;
}

uint32_t *sdrammc_search_read_window(uint32_t *ptr)
{
#define SRAM_BASE				0x10000000
#define var_win					(SRAM_BASE + 0)

#ifdef CONFIG_ASPEED_PALLADIUM
	ptr = wr_single(ptr, PHY_BASE + 0x00, 0x0000000c);
	return ptr;
#endif

	ptr = wr_single(ptr, SEARCH_RDWIN_ANCHOR_0, SEARCH_RDWIN_PTRN_0);
    ptr = wr_single(ptr, SEARCH_RDWIN_ANCHOR_1, SEARCH_RDWIN_PTRN_1);
	ptr = wr_single(ptr, PHY_BASE + 0x00, 0x0000000c);
	log_label(ptr, "l_check_value_start");
	ptr = sdrammc_fpga_set_pll(ptr);
	ptr = log_jeq(ptr, SEARCH_RDWIN_ANCHOR_0, GENMASK(31, 0), SEARCH_RDWIN_PTRN_0, "l_check_value_start");
	ptr = log_jeq(ptr, SEARCH_RDWIN_ANCHOR_1, GENMASK(31, 0), SEARCH_RDWIN_PTRN_1, "l_check_value_start");

	ptr = wr_single(ptr, var_win, 0);
	log_label(ptr, "l_cali_rd_win_start");
	ptr = sdrammc_fpga_set_pll(ptr);
	ptr = add_code(ptr, var_win, 1);
	ptr = log_jne(ptr, var_win, GENMASK(31, 0), 256, "l_cali_rd_win_start");

	return ptr;
}

uint32_t *sdrammc_calc_size(uint32_t *ptr)
{
	ptr = wr_single(ptr, 0xc0100000, 0xdeadbeef);
	ptr = wr_single(ptr, 0xa0100000, 0xfdeadbee);
	ptr = wr_single(ptr, 0x90100000, 0xefdeadbe);
	ptr = wr_single(ptr, 0x80100000, 0xeefdeadb);

	ptr = log_jeq(ptr, 0xc0100000, GENMASK(31, 0), 0xdeadbeef, "l_size_2g");
	ptr = log_jeq(ptr, 0xa0100000, GENMASK(31, 0), 0xfdeadbee, "l_size_1g");
	ptr = log_jeq(ptr, 0x90100000, GENMASK(31, 0), 0xefdeadbe, "l_size_512m");
	
	//log_label(ptr, "l_size_256m");
	ptr = rmw_code(ptr, MMC_BASE + 0x04, ~GENMASK(1, 0), 0x0);
	ptr = log_jmp(ptr, "l_size_done_0");

	log_label(ptr, "l_size_2g");
	ptr = rmw_code(ptr, MMC_BASE + 0x04, ~GENMASK(1, 0), 0x3);
	ptr = log_jmp(ptr, "l_size_done_0");

	log_label(ptr, "l_size_1g");
	ptr = rmw_code(ptr, MMC_BASE + 0x04, ~GENMASK(1, 0), 0x2);
	ptr = log_jmp(ptr, "l_size_done_0");

	log_label(ptr, "l_size_512m");
	ptr = rmw_code(ptr, MMC_BASE + 0x04, ~GENMASK(1, 0), 0x1);
	//ptr = jmp_code(ptr, "l_size_done");

/* l_size_done_0 */
	log_label(ptr, "l_size_done_0");

	/* check VGA size: read SCU500[14:13] and write MMC04[3:2] */
	ptr = log_jeq(ptr, STRAP_REG, GENMASK(14, 13), 0x3 << 13, "l_vga_size_64m");
	ptr = log_jeq(ptr, STRAP_REG, GENMASK(14, 13), 0x2 << 13, "l_vga_size_32m");
	ptr = log_jeq(ptr, STRAP_REG, GENMASK(14, 13), 0x1 << 13, "l_vga_size_16m");

	// VGA 8MB -> set to 16MB
	ptr = setbit_code(ptr, STRAP_REG, 0x1 << 13);
	log_label(ptr, "l_vga_size_16m");
	ptr = rmw_code(ptr, MMC_BASE + 0x04, ~GENMASK(3, 2), 0x1 << 2);
	ptr = log_jmp(ptr, "l_size_done_1");

	log_label(ptr, "l_vga_size_32m");
	ptr = rmw_code(ptr, MMC_BASE + 0x04, ~GENMASK(3, 2), 0x2 << 2);
	ptr = log_jmp(ptr, "l_size_done_1");

	log_label(ptr, "l_vga_size_64m");
	ptr = rmw_code(ptr, MMC_BASE + 0x04, ~GENMASK(3, 2), 0x3 << 2);
	ptr = log_jmp(ptr, "l_size_done_1");
/* l_size_done_1 */
	log_label(ptr, "l_size_done_1");
	return ptr;
}

int main()
{
    int i, size;
    uint32_t *ptr = rom_code;
	FILE *fp;

    log_label(ptr, "l_start");
    ptr = start_code(ptr);
    /* ---------- start ---------- */ 
	/* goto check_size if DRAM is already initialized */
    ptr = log_jeq(ptr, SCU_BASE + 0x100, BIT(6), BIT(6), "l_calc_size");

    /* set MPLL */
    log_label(ptr, "l_set_pll_start");
    ptr = rmw_code(ptr, MPLL_REG, ~(BIT(24) | GENMASK(22, 0)),
		   BIT(25) | BIT(23) | MPLL_FREQ_400M);
    ptr = wr_single(ptr, MPLL_EXT_REG, MPLL_EXT_400M);
    ptr = delay_code(ptr, 100);
    ptr = rmw_code(ptr, MPLL_REG, GENMASK(31, 0), ~(BIT(25) | BIT(23)));
    ptr = waiteq_code(ptr, MPLL_EXT_REG, BIT(31), BIT(31), 1);

    /* ast2600_sdrammc_unlock */
    ptr = wr_single(ptr, MMC_BASE, MMC_UNLOCK_KEY);
    ptr = waiteq_code(ptr, MMC_BASE, BIT(0), BIT(0), 1);

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
    /* ---------- end ---------- */
    log_label(ptr, "l_end");
    ptr = quit_code(ptr);


    /* ---------- debug print ---------- */
	printf("---------------------------------------------------------------\n");
	printf("list of labels\n");
	printf("---------------------------------------------------------------\n");
	for (i = 0; i < rom_labels.count; i++) {
		label_t *lab = &rom_labels.labels[i];
		printf("[%04x](%d) %s\n", lab->offset, lab->offset, lab->name);
	}

    /* ---------- organize jump label ---------- */
    // replace the label offset
	for (i = 0; i < jmp_list.count; i++) {
		int j;
		for (j = 0; j < rom_labels.count; j++) {
		    if (0 ==
			strcmp(jmp_list.label[i], rom_labels.labels[j].name)) {
			rom_op_jmp_t *code = jmp_list.log[i];
			// +------------------+
			// | start            | -> rom_code
			// +------------------+
			// | ...              |
			// +------------------+
			// | jump inst. start | -> jmp_list.log[i]
			// +------------------+
			// | ...              |
			// +------------------+
			// | jump inst. end   |
			// +------------------+
			// | next inst.       | -> jmp_list.log[i] + sizeof(rom_op_jmp_t)
			// +------------------+
			// | ...              |
			// +------------------+
			// | label to jump    | -> label
			// +------------------+
			code->cmd.b.num = rom_labels.labels[j].offset -
					  ((int)code + sizeof(rom_op_jmp_t) -
					   (int)rom_code);
		    }
		}
    }
    printf("---------------------------------------------------------------\n");
    printf("rom code patch\n");
    printf("---------------------------------------------------------------\n");
    size = ptr - rom_code;
    for (i = 0; i < size; i++) {
		printf("[%04x] %08x\n", i * 4, rom_code[i]);
	}
	
	fp = fopen("rom_patch.bin", "wb");
	fwrite(rom_code, size, sizeof(uint32_t), fp);
	fclose(fp);
}