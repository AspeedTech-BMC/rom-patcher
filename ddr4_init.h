#include <stdint.h>
#include <stdio.h>

void sdrammc_common_init(FILE *fp);
void sdrammc_calc_size(FILE *fp);
void sdramphy_init(FILE *fp);
void sdramphy_kick_training(FILE *fp);
void sdramphy_check_status(FILE *fp);
void sdrammc_init_ddr4(FILE *fp);
void sdrammc_fpga_set_pll(FILE *fp);
void sdrammc_search_read_window(FILE *fp);
void sdrammc_calc_size(FILE *fp);
void sdram_probe(FILE *fp);