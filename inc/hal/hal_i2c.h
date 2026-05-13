#ifndef HAL_I2C_H
#define HAL_I2C_H

#include <stdint.h>

#define TWI0_BASE 0x40018000U /* Página 43 */
#define TWI1_BASE 0x4001C000U /* Página 43 */
#define TWI2_BASE 0x40060000U /* Página 43 */

/* =========================================================================
   Peripheral IDs — Secção 14, Table 14-1. Peripheral Identifiers, Páginas 57-58
   ========================================================================= */
#define ID_TWI0 19U /* Página 57 */
#define ID_TWI1 20U /* Página 57 */
#define ID_TWI2 41U /* Página 58 */

/* =========================================================================
   TWIHS Register Offsets — Secção 43, Table 43-7. Register Mapping, Página 1048
   ========================================================================= */
#define TWI_CR_OFFSET 0x00U    /* Control Register      — Página 1048 */
#define TWI_MMR_OFFSET 0x04U   /* Master Mode Register  — Página 1048 */
#define TWI_SMR_OFFSET 0x08U   /* Slave Mode Register   — Página 1048 */
#define TWI_IADR_OFFSET 0x0CU  /* Internal Address      — Página 1048 */
#define TWI_CWGR_OFFSET 0x10U  /* Clock Waveform        — Página 1048 */
#define TWI_SR_OFFSET 0x20U    /* Status Register       — Página 1048 */
#define TWI_IER_OFFSET 0x24U   /* Interrupt Enable      — Página 1048 */
#define TWI_IDR_OFFSET 0x28U   /* Interrupt Disable     — Página 1048 */
#define TWI_IMR_OFFSET 0x2CU   /* Interrupt Mask        — Página 1048 */
#define TWI_RHR_OFFSET 0x30U   /* Receive Holding       — Página 1048 */
#define TWI_THR_OFFSET 0x34U   /* Transmit Holding      — Página 1048 */
#define TWI_FILTR_OFFSET 0x44U /* Filter Register       — Página 1048 */
#define TWI_SWMR_OFFSET 0x4CU  /* SleepWalking Match    — Página 1048 */

/* =========================================================================
   TWIHS_CR bits — Secção 43.7.1, Páginas 1050-1051
   ========================================================================= */
#define TWI_CR_START (1U << 0U) /* Send START          — Página 1050 */
#define TWI_CR_STOP (1U << 1U)  /* Send STOP           — Página 1050 */
#define TWI_CR_MSEN (1U << 2U)  /* Master Mode Enable  — Página 1050 */
#define TWI_CR_MSDIS (1U << 3U) /* Master Mode Disable — Página 1050 */
#define TWI_CR_SVEN (1U << 4U)  /* Slave Mode Enable   — Página 1051 */
#define TWI_CR_SVDIS (1U << 5U) /* Slave Mode Disable  — Página 1051 */
#define TWI_CR_SWRST (1U << 7U) /* Software Reset      — Página 1051 */

/* =========================================================================
   TWIHS_SR bits — Secção 43.7.6, Páginas 1057-1058
   ========================================================================= */
#define TWI_SR_TXCOMP (1U << 0U)  /* Transmission Complete  — Página 1057 */
#define TWI_SR_RXRDY (1U << 1U)   /* Receive Holding Full   — Página 1057 */
#define TWI_SR_TXRDY (1U << 2U)   /* Transmit Holding Empty — Página 1057 */
#define TWI_SR_SVREAD (1U << 3U)  /* Slave Read             — Página 1057 */
#define TWI_SR_SVACC (1U << 4U)   /* Slave Access           — Página 1057 */
#define TWI_SR_NACK (1U << 8U)    /* Not Acknowledged       — Página 1058 */
#define TWI_SR_ARBLST (1U << 9U)  /* Arbitration Lost       — Página 1058 */
#define TWI_SR_EOSACC (1U << 11U) /* End of Slave Access    — Página 1058 */

/* =========================================================================
   Macros de acesso aos registos
   ========================================================================= */
#define REG(addr) (*(volatile uint32_t *)(addr))
#define TWI0_CR REG(TWI0_BASE + TWI_CR_OFFSET)
#define TWI0_MMR REG(TWI0_BASE + TWI_MMR_OFFSET)
#define TWI0_SR REG(TWI0_BASE + TWI_SR_OFFSET)
#define TWI0_RHR REG(TWI0_BASE + TWI_RHR_OFFSET)
#define TWI0_THR REG(TWI0_BASE + TWI_THR_OFFSET)
#define TWI0_CWGR REG(TWI0_BASE + TWI_CWGR_OFFSET)
#define TWI0_IADR REG(TWI0_BASE + TWI_IADR_OFFSET)

int hal_i2c_bus_free(void);
void hal_i2c_start(void);
void hal_i2c_stop(void);
void hal_i2c_send_byte(uint8_t addr);
int hal_i2c_get_ack(void);
uint8_t hal_i2c_read_byte(void);
void hal_i2c_send_addr(uint8_t byte);
void hal_i2c_send_ack(void);
void hal_i2c_send_nack(void);
uint8_t hal_i2c_init(void);
int hal_i2c_tx_ready(void);
int hal_i2c_rx_ready(void);
void hal_i2c_request_byte(void);
void hal_i2c_restart_read(uint8_t addr);
void hal_i2c_bus_recovery(void);

#endif