#ifndef HAL_SPI_H
#define HAL_SPI_H
#include <stdint.h>

/* Base Addresses — Secção 10, Figura 10-1, Página 43 */
#define SPI0_BASE    0x40008000U
#define SPI1_BASE    0x4005C000U

/* Peripheral IDs — Secção 14, Table 14-1, Páginas 57-58 */
#define ID_SPI0      21U   /* Página 57 */
#define ID_SPI1      42U   /* Página 58 */

/* Register Offsets — Secção 41, Register Mapping */
#define SPI_CR_OFFSET    0x00U  /* Control Register    */
#define SPI_MR_OFFSET    0x04U  /* Mode Register       */
#define SPI_RDR_OFFSET   0x08U  /* Receive Data        */
#define SPI_TDR_OFFSET   0x0CU  /* Transmit Data       */
#define SPI_SR_OFFSET    0x10U  /* Status Register     */
#define SPI_IER_OFFSET   0x14U  /* Interrupt Enable    */
#define SPI_IDR_OFFSET   0x18U  /* Interrupt Disable   */
#define SPI_IMR_OFFSET   0x1CU  /* Interrupt Mask      */
#define SPI_CSR0_OFFSET  0x30U  /* Chip Select 0       */
#define SPI_CSR1_OFFSET  0x34U  /* Chip Select 1       */
#define SPI_CSR2_OFFSET  0x38U  /* Chip Select 2       */
#define SPI_CSR3_OFFSET  0x3CU  /* Chip Select 3       */
#define SPI_WPMR_OFFSET  0xE4U  /* Write Protect Mode  */
#define SPI_WPSR_OFFSET  0xE8U  /* Write Protect Status*/

/* SR bits */
#define SPI_SR_RDRF      (1U << 0U)  /* Receive Data Register Full  */
#define SPI_SR_TDRE      (1U << 1U)  /* Transmit Data Register Empty*/
#define SPI_SR_MODF      (1U << 2U)  /* Mode Fault Error            */
#define SPI_SR_OVRES     (1U << 3U)  /* Overrun Error               */
#define SPI_SR_TXEMPTY   (1U << 9U)  /* Transmission Registers Empty*/
#define SPI_SR_SPIENS    (1U << 16U) /* SPI Enable Status           */

/* CR bits */
#define SPI_CR_SPIEN     (1U << 0U)  /* SPI Enable                  */
#define SPI_CR_SPIDIS    (1U << 1U)  /* SPI Disable                 */
#define SPI_CR_SWRST     (1U << 7U)  /* SPI Software Reset          */
#define SPI_CR_LASTXFER  (1U << 24U) /* Last Transfer               */

/* MR bits */
#define SPI_MR_MSTR      (1U << 0U)  /* Master/Slave Mode           */
#define SPI_MR_PS        (1U << 1U)  /* Peripheral Select           */
#define SPI_MR_MODFDIS   (1U << 4U)  /* Mode Fault Detection Disable*/

/* Macros de acesso */
#define REG(addr)        (*(volatile uint32_t *)(addr))
#define SPI0_CR          REG(SPI0_BASE + SPI_CR_OFFSET)
#define SPI0_MR          REG(SPI0_BASE + SPI_MR_OFFSET)
#define SPI0_RDR         REG(SPI0_BASE + SPI_RDR_OFFSET)
#define SPI0_TDR         REG(SPI0_BASE + SPI_TDR_OFFSET)
#define SPI0_SR          REG(SPI0_BASE + SPI_SR_OFFSET)
#define SPI0_CSR0        REG(SPI0_BASE + SPI_CSR0_OFFSET)

/* Interface pública */
uint8_t hal_spi_init(void);
uint8_t hal_spi_tx_ready(void);
uint8_t hal_spi_rx_ready(void);
void    hal_spi_cs_low(uint8_t cs_pin);
void    hal_spi_cs_high(uint8_t cs_pin);
void    hal_spi_send_byte(uint8_t data);
uint8_t hal_spi_read_byte(void);
void    hal_spi_prepare_transfer(void);

#endif