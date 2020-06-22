#define SCU_BASE 			0x1e6e2000
#define MPLL_REG 			(SCU_BASE + 0x220)
#define MPLL_EXT_REG 		(SCU_BASE + 0x224)
#define STRAP_REG			(SCU_BASE + 0x500)

#define MPLL_FREQ_400M 		0x0008405f
#define MPLL_EXT_400M 		0x00000031

#define MMC_BASE 			0x1e6e0000
#define PHY_BASE 			(MMC_BASE + 0x100)
#define PHY_STS_BASE		(MMC_BASE + 0x400)
#define MMC_UNLOCK_KEY 		0xfc600309

/* secure boot controller */
#define SBC_BASE			0x1e6f2000
#define OTP_QSR				0x40

#define SRAM_BASE			0x10000000	/* 64KB */
#define SRAM1_BASE			0x10010000	/* 24KB */
#define SPI_BASE			0x20000000
#define DRAM_BASE			CONFIG_SYS_SDRAM_BASE