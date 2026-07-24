#ifndef HAL_QSPI_H
#define HAL_QSPI_H

#include <stdint.h>
#include "config/board.h"

/* =========================================================================
   Base Addresses — Secção 10, Figure 10-1, Página 43
   ========================================================================= */
#define QSPI_BASE 0x4007C000UL     /* Página 43 */
#define QSPI_MEM_BASE 0x80000000UL /* QSPI memory-mapped region */

/* =========================================================================
   Peripheral ID — Secção 14, Table 14-1, Página 58
   ========================================================================= */
#define ID_QSPI 43U /* Página 58 */

/* =========================================================================
   QSPI Register Offsets — Secção 42, Table 42-6, Página 1040
   ========================================================================= */
#define QSPI_CR_OFFSET 0x00U  /* Control Register            */
#define QSPI_MR_OFFSET 0x04U  /* Mode Register               */
#define QSPI_RDR_OFFSET 0x08U /* Receive Data Register       */
#define QSPI_TDR_OFFSET 0x0CU /* Transmit Data Register      */
#define QSPI_SR_OFFSET 0x10U  /* Status Register             */
#define QSPI_IER_OFFSET 0x14U /* Interrupt Enable Register   */
#define QSPI_IDR_OFFSET 0x18U /* Interrupt Disable Register  */
#define QSPI_IMR_OFFSET 0x1CU /* Interrupt Mask Register     */
#define QSPI_SCR_OFFSET 0x20U /* Serial Clock Register       */
#define QSPI_IAR_OFFSET 0x30U /* Instruction Address Register*/
#define QSPI_ICR_OFFSET 0x34U /* Instruction Code Register   */
#define QSPI_IFR_OFFSET 0x38U /* Instruction Frame Register  */
#define QSPI_SMR_OFFSET 0x40U /* Scrambling Mode Register    */
#define QSPI_SKR_OFFSET 0x44U /* Scrambling Key Register     */

/* =========================================================================
   QSPI_CR bits — Secção 42, Página 1041
   ========================================================================= */
#define QSPI_CR_QSPIEN (1UL << 0)    /* QSPI Enable              */
#define QSPI_CR_QSPIDIS (1UL << 1)   /* QSPI Disable             */
#define QSPI_CR_SWRST (1UL << 7)     /* Software Reset           */
#define QSPI_CR_LASTXFER (1UL << 24) /* Last Transfer            */

/* =========================================================================
   QSPI_MR bits — Secção 42, Página 1042
   ========================================================================= */
#define QSPI_MR_SMM (1UL << 0)                  /* Serial Memory Mode       */
#define QSPI_MR_LLB (1UL << 1)                  /* Local Loopback Enable    */
#define QSPI_MR_WDRBT (1UL << 2)                /* Wait Data Read Before Tx */
#define QSPI_MR_CSMODE_LASTXFER (0x1UL << 4)    /* CS released after LASTXFER */
#define QSPI_MR_NBBITS_8 (0x0UL << 8)           /* 8-bit transfer           */
#define QSPI_MR_DLYBCT(v) (((v) & 0xFFU) << 16) /* Delay Between Consecutive Transfers */
#define QSPI_MR_DLYCS(v) (((v) & 0xFFU) << 24)  /* Minimum Inactive QCS Delay */

/* =========================================================================
   QSPI_SR bits — Secção 42, Página 1047
   ========================================================================= */
#define QSPI_SR_RDRF (1UL << 0)     /* Receive Data Register Full  */
#define QSPI_SR_TDRE (1UL << 1)     /* Transmit Data Register Empty*/
#define QSPI_SR_TXEMPTY (1UL << 2)  /* Transmission Registers Empty*/
#define QSPI_SR_OVRES (1UL << 3)    /* Overrun Error               */
#define QSPI_SR_CSR (1UL << 8)      /* Chip Select Rise            */
#define QSPI_SR_CSS (1UL << 9)      /* Chip Select Status          */
#define QSPI_SR_INSTRE (1UL << 10)  /* Instruction End             */
#define QSPI_SR_QSPIENS (1UL << 24) /* QSPI Enable Status          */

/* =========================================================================
   QSPI_SCR bits — Secção 42, Página 1050
   ========================================================================= */
#define QSPI_SCR_CPOL (1UL << 0)              /* Clock Polarity              */
#define QSPI_SCR_CPHA (1UL << 1)              /* Clock Phase                 */
#define QSPI_SCR_SCBR(v) (((v) & 0xFFU) << 8) /* Serial Clock Baud Rate */

/* =========================================================================
   QSPI_IFR bits — Secção 42, Página 1054
   ========================================================================= */
#define QSPI_IFR_WIDTH_SINGLE (0x0UL << 0)      /* Single-bit SPI       */
#define QSPI_IFR_WIDTH_DUAL_OUT (0x1UL << 0)    /* Dual output          */
#define QSPI_IFR_WIDTH_QUAD_OUT (0x2UL << 0)    /* Quad output          */
#define QSPI_IFR_WIDTH_DUAL_IO (0x3UL << 0)     /* Dual I/O             */
#define QSPI_IFR_WIDTH_QUAD_IO (0x4UL << 0)     /* Quad I/O             */
#define QSPI_IFR_WIDTH_DUAL_CMD (0x5UL << 0)    /* Dual command         */
#define QSPI_IFR_WIDTH_QUAD_CMD (0x6UL << 0)    /* Quad command         */
#define QSPI_IFR_INSTEN (1UL << 4)              /* Instruction Enable          */
#define QSPI_IFR_ADDREN (1UL << 5)              /* Address Enable              */
#define QSPI_IFR_OPTEN (1UL << 6)               /* Option Enable               */
#define QSPI_IFR_DATAEN (1UL << 7)              /* Data Enable                 */
#define QSPI_IFR_OPTL_1 (0x0UL << 8)            /* Option 1 bit                */
#define QSPI_IFR_OPTL_2 (0x1UL << 8)            /* Option 2 bits               */
#define QSPI_IFR_OPTL_4 (0x2UL << 8)            /* Option 4 bits               */
#define QSPI_IFR_OPTL_8 (0x3UL << 8)            /* Option 8 bits               */
#define QSPI_IFR_ADDRL_24 (0x0UL << 10)         /* 24-bit address             */
#define QSPI_IFR_ADDRL_32 (0x1UL << 10)         /* 32-bit address             */
#define QSPI_IFR_TFRTYP_READ (0x0UL << 12)      /* Read transfer        */
#define QSPI_IFR_TFRTYP_READMEM (0x1UL << 12)   /* Read memory          */
#define QSPI_IFR_TFRTYP_WRITE (0x2UL << 12)     /* Write transfer       */
#define QSPI_IFR_TFRTYP_WRITEMEM (0x3UL << 12)  /* Write memory         */
#define QSPI_IFR_CRM (1UL << 14)                /* Continuous Read Mode        */
#define QSPI_IFR_NBDUM(v) (((v) & 0x1FU) << 16) /* Number of Dummy Cycles */

/* =========================================================================
   W25Q128 Commands — Datasheet W25Q128JV
   ========================================================================= */
#define W25Q_CMD_WRITE_ENABLE 0x06U   /* WREN                        */
#define W25Q_CMD_WRITE_DISABLE 0x04U  /* WRDI                        */
#define W25Q_CMD_READ_STATUS_1 0x05U  /* Read Status Register-1      */
#define W25Q_CMD_READ_STATUS_2 0x35U  /* Read Status Register-2      */
#define W25Q_CMD_READ_DATA 0x03U      /* Read Data (up to 50MHz)     */
#define W25Q_CMD_FAST_READ 0x0BU      /* Fast Read (dummy byte)      */
#define W25Q_CMD_PAGE_PROGRAM 0x02U   /* Page Program (256 bytes max)*/
#define W25Q_CMD_SECTOR_ERASE 0x20U   /* Sector Erase (4KB)          */
#define W25Q_CMD_BLOCK_ERASE_32 0x52U /* Block Erase (32KB)          */
#define W25Q_CMD_BLOCK_ERASE_64 0xD8U /* Block Erase (64KB)          */
#define W25Q_CMD_CHIP_ERASE 0xC7U     /* Chip Erase                  */
#define W25Q_CMD_READ_JEDEC_ID 0x9FU  /* Read JEDEC ID               */
#define W25Q_CMD_POWER_DOWN 0xB9U     /* Power Down                  */
#define W25Q_CMD_RELEASE_PD 0xABU     /* Release Power Down          */

/* W25Q Status Register-1 bits */
#define W25Q_SR1_BUSY 0x01U /* Erase/Write in progress     */
#define W25Q_SR1_WEL 0x02U  /* Write Enable Latch          */

/* =========================================================================
   W25Q128 Properties
   ========================================================================= */
#define W25Q_PAGE_SIZE 256U    /* Page size in bytes          */
#define W25Q_SECTOR_SIZE 4096U /* Sector size (4KB)           */
#define W25Q_BLOCK_SIZE_32K 32768U
#define W25Q_BLOCK_SIZE_64K 65536U
#define W25Q_TOTAL_SIZE (2UL * 1024UL * 1024UL) /* 2MB — S25FL116K */
#define W25Q_FAST_READ_DUMMY 8U                 /* 8 dummy cycles for fast read*/

/* =========================================================================
   Configuração
   ========================================================================= */
#define QSPI_CLK_DIV 36U /* SCK = MCK/(SCBR+1) = 150MHz/37 ≈ 4 MHz */

/* =========================================================================
   Macros de acesso aos registos
   ========================================================================= */
#define QSPI_REG(offset) (*(volatile uint32_t *)(QSPI_BASE + (offset)))
#define QSPI_CR QSPI_REG(QSPI_CR_OFFSET)
#define QSPI_MR QSPI_REG(QSPI_MR_OFFSET)
#define QSPI_RDR QSPI_REG(QSPI_RDR_OFFSET)
#define QSPI_TDR QSPI_REG(QSPI_TDR_OFFSET)
#define QSPI_SR QSPI_REG(QSPI_SR_OFFSET)
#define QSPI_IER QSPI_REG(QSPI_IER_OFFSET)
#define QSPI_IDR QSPI_REG(QSPI_IDR_OFFSET)
#define QSPI_SCR QSPI_REG(QSPI_SCR_OFFSET)
#define QSPI_IAR QSPI_REG(QSPI_IAR_OFFSET)
#define QSPI_ICR QSPI_REG(QSPI_ICR_OFFSET)
#define QSPI_IFR QSPI_REG(QSPI_IFR_OFFSET)

/* =========================================================================
   Protótipos HAL
   ========================================================================= */
uint8_t hal_qspi_init(void);
void hal_qspi_send_command(uint8_t cmd);
void hal_qspi_send_command_addr(uint8_t cmd, uint32_t addr);
uint8_t hal_qspi_read_status(void);
uint8_t hal_qspi_is_busy(void);
uint8_t hal_qspi_instruction_done(void);
void hal_qspi_read_memory(uint32_t addr, uint8_t *buf, uint32_t len);
void hal_qspi_write_memory(uint32_t addr, const uint8_t *buf, uint32_t len);
void hal_qspi_clear_write_protection(void);

#endif