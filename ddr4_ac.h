#include <stdint.h>
#include "config.h"
#if defined(CONFIG_FPGA_ASPEED) || defined(CONFIG_ASPEED_PALLADIUM)
static const uint32_t ddr4_ac_timing[4] = {0x030C0207, 0x04451133, 0x0E010200,
                                           0x00000140};

static const uint32_t ddr_max_grant_params[4] = {0x88888888, 0x88888888,
                                                 0x88888888, 0x88888888};
#else
static const uint32_t ddr4_ac_timing[4] = {0x040e0307, 0x0f4711f1, 0x0e060304,
                                           0x00001240};

static const uint32_t ddr_max_grant_params[4] = {0x44444444, 0x44444444,
                                                 0x44444444, 0x44444444};
#endif

#if defined(CONFIG_FPGA_ASPEED) || defined(CONFIG_ASPEED_PALLADIUM)
/* mode register settings for FPGA are fixed */
#define DDR4_MR01_MODE          0x03010100
#define DDR4_MR23_MODE          0x00000000
#define DDR4_MR45_MODE          0x04C00000
#define DDR4_MR6_MODE           0x00000050
#define DDR4_TRFC_FPGA          0x17263434

/* FPGA need for an additional initialization procedure: search read window */
#define SEARCH_RDWIN_ANCHOR_0   (CONFIG_SYS_SDRAM_BASE + 0x0000)
#define SEARCH_RDWIN_ANCHOR_1   (CONFIG_SYS_SDRAM_BASE + 0x0004)
#define SEARCH_RDWIN_PTRN_0     0x12345678
#define SEARCH_RDWIN_PTRN_1     0xaabbccdd
#define SEARCH_RDWIN_PTRN_SUM   0xbcf02355
#else
/* mode register setting for real chip are derived from the model GDDR4-1600 */
#define DDR4_MR01_MODE          0x03010510
#define DDR4_MR23_MODE          0x00000000
#define DDR4_MR45_MODE          0x04000000
#define DDR4_MR6_MODE           0x00000400
#define DDR4_TRFC_1600          0x467299f1
#define DDR4_TRFC_1333          0x3a5f80c9
#define DDR4_TRFC_800           0x23394c78
#define DDR4_TRFC_400           0x111c263c
#endif /* end of "#if defined(CONFIG_FPGA_ASPEED) ||                           \
          defined(CONFIG_ASPEED_PALLADIUM)" */

uint32_t ddr4_mr_setting[4] = {DDR4_MR01_MODE, DDR4_MR23_MODE, DDR4_MR45_MODE,
                               DDR4_MR6_MODE};

#if defined(CONFIG_FPGA_ASPEED) || defined(CONFIG_ASPEED_PALLADIUM)
#define DDR4_TRFC                       DDR4_TRFC_FPGA
#else
/* real chip setting */
#if defined(CONFIG_ASPEED_DDR4_1600)
#define DDR4_TRFC                       DDR4_TRFC_1600
#define DDR4_PHY_TRAIN_TRFC             0xc30
#elif defined(CONFIG_ASPEED_DDR4_1333)
#define DDR4_TRFC                       DDR4_TRFC_1333
#define DDR4_PHY_TRAIN_TRFC             0xa25
#elif defined(CONFIG_ASPEED_DDR4_800)
#define DDR4_TRFC                       DDR4_TRFC_800
#define DDR4_PHY_TRAIN_TRFC             0x618
#elif defined(CONFIG_ASPEED_DDR4_400)
#define DDR4_TRFC                       DDR4_TRFC_400
#define DDR4_PHY_TRAIN_TRFC             0x30c
#else
#error "undefined tRFC setting"
#endif  /* end of "#if (SCU_MPLL_FREQ_CFG == SCU_MPLL_FREQ_400M)" */
#endif  /* end of "#if defined(CONFIG_FPGA_ASPEED) ||                          \
           defined(CONFIG_ASPEED_PALLADIUM)" */
