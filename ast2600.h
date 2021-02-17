#ifndef _AST2600_H_
#define _AST2600_H_

#include "config.h"

#define SCU_BASE 						0x1e6e2000
#define MPLL_REG 						(SCU_BASE + 0x220)
#define MPLL_EXT_REG 					(SCU_BASE + 0x224)
#define STRAP_REG						(SCU_BASE + 0x500)

#define MPLL_FREQ_400M 					0x0008405f
#define MPLL_EXT_400M 					0x00000031

#define MMC_BASE 						0x1e6e0000
#define PHY_BASE 						(MMC_BASE + 0x100)
#define PHY_STS_BASE					(MMC_BASE + 0x400)
#define MMC_UNLOCK_KEY 					0xfc600309

#define UART_BASE						0x1e784000
/* DLAB = 0 */
#define UART_THR                        0x00
#define UART_RBR                        0x00
#define UART_IER                        0x04
/* DLAB = 1 */
#define UART_DLL                        0x00
#define UART_DLH                        0x04

#define UART_IIR                        0x08    /* for read */
#define UART_FCR                        0x08    /* for write */
#define UART_LCR                        0x0C
#define UART_MCR                        0x10
#define UART_LSR                        0x14
#define UART_MSR                        0x18
#define UART_SCR                        0x1C

/* bitfields for UART_LSR */
#define UART_LSR_THRE                   BIT(5)
#define UART_LSR_DR                     BIT(0)

/* secure boot controller */
#define SBC_BASE						0x1e6f2000
#define SBC_STS							0x14
#define OTP_QSR							0x40

#define SRAM_BASE						0x10000000	/* 64KB */
#define SRAM1_BASE						0x10010000	/* 24KB */
#define SPI_BASE						0x20000000
#define DRAM_BASE						CONFIG_SYS_SDRAM_BASE

#endif /* end of "ifndef _AST2600_H_" */
