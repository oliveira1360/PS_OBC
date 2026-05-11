#ifndef HAL_USART_H
#define HAL_USART_H
<<<<<<< HEAD
#include <stdio.h>
#include <stdint.h>


/* =========================================================================
   Base Addresses — Secção 10, Figure 10-1. SAM V71 Product Mapping, Página 43
=======

#include <stdint.h>

/* =========================================================================
   Base Addresses — Secção 10, Figure 10-1. Product Mapping, Página 43
>>>>>>> origin/OBC_board
   ========================================================================= */
#define USART0_BASE  0x40024000U  /* Página 43 */
#define USART1_BASE  0x40028000U  /* Página 43 */
#define USART2_BASE  0x4002C000U  /* Página 43 */
<<<<<<< HEAD
 
=======

>>>>>>> origin/OBC_board
/* =========================================================================
   Peripheral IDs — Secção 14, Table 14-1. Peripheral Identifiers, Páginas 57-58
   ========================================================================= */
#define ID_USART0    13U          /* Página 57 */
#define ID_USART1    14U          /* Página 57 */
#define ID_USART2    15U          /* Página 57 */
<<<<<<< HEAD
 
/* =========================================================================
   USART Register Offsets — Secção 46, Table 46-17. Register Mapping, Página 1197
   ========================================================================= */
#define US_CR_OFFSET    0x00U    /* Control Register      — Página 1197 */
#define US_MR_OFFSET    0x04U    /* Mode Register         — Página 1197 */
#define US_IER_OFFSET   0x08U    /* Interrupt Enable      — Página 1197 */
#define US_IDR_OFFSET   0x0CU    /* Interrupt Disable     — Página 1197 */
#define US_IMR_OFFSET   0x10U    /* Interrupt Mask        — Página 1197 */
#define US_CSR_OFFSET   0x14U    /* Channel Status        — Página 1197 */
#define US_RHR_OFFSET   0x18U    /* Receive Holding       — Página 1197 */
#define US_THR_OFFSET   0x1CU    /* Transmit Holding      — Página 1197 */
#define US_BRGR_OFFSET  0x20U    /* Baud Rate Generator   — Página 1197 */
#define US_RTOR_OFFSET  0x24U    /* Receiver Time-out     — Página 1197 */
#define US_TTGR_OFFSET  0x28U    /* Transmitter Timeguard — Página 1197 */
#define US_FIDI_OFFSET  0x40U    /* FI DI Ratio           — Página 1197 */
#define US_NER_OFFSET   0x44U    /* Number of Errors      — Página 1197 */
#define US_IF_OFFSET    0x4CU    /* IrDA Filter           — Página 1197 */
#define US_MAN_OFFSET   0x50U    /* Manchester Config     — Página 1197 */
#define US_LINMR_OFFSET 0x54U    /* LIN Mode              — Página 1197 */
#define US_WPMR_OFFSET  0xE4U    /* Write Protect Mode    — Página 1197 */
#define US_WPSR_OFFSET  0xE8U    /* Write Protect Status  — Página 1197 */
 
=======

/* =========================================================================
   USART Register Offsets — Secção 46, Table 46-17. Register Mapping, Página 1197
   ========================================================================= */
#define US_CR_OFFSET    0x00U     /* Control Register      — Página 1197 */
#define US_MR_OFFSET    0x04U     /* Mode Register         — Página 1197 */
#define US_IER_OFFSET   0x08U     /* Interrupt Enable      — Página 1197 */
#define US_IDR_OFFSET   0x0CU     /* Interrupt Disable     — Página 1197 */
#define US_IMR_OFFSET   0x10U     /* Interrupt Mask        — Página 1197 */
#define US_CSR_OFFSET   0x14U     /* Channel Status        — Página 1197 */
#define US_RHR_OFFSET   0x18U     /* Receive Holding       — Página 1197 */
#define US_THR_OFFSET   0x1CU     /* Transmit Holding      — Página 1197 */
#define US_BRGR_OFFSET  0x20U     /* Baud Rate Generator   — Página 1197 */

>>>>>>> origin/OBC_board
/* =========================================================================
   US_CR bits — Secção 46, Página 1198
   ========================================================================= */
#define US_CR_RSTRX     (1U << 2U)  /* Reset Receiver        — Página 1198 */
#define US_CR_RSTTX     (1U << 3U)  /* Reset Transmitter     — Página 1198 */
#define US_CR_RXEN      (1U << 4U)  /* Receiver Enable       — Página 1198 */
#define US_CR_RXDIS     (1U << 5U)  /* Receiver Disable      — Página 1198 */
#define US_CR_TXEN      (1U << 6U)  /* Transmitter Enable    — Página 1198 */
#define US_CR_TXDIS     (1U << 7U)  /* Transmitter Disable   — Página 1198 */
#define US_CR_RSTSTA    (1U << 8U)  /* Reset Status Bits     — Página 1198 */
<<<<<<< HEAD
 
=======

>>>>>>> origin/OBC_board
/* =========================================================================
   US_CSR bits — Secção 46, Página 1228
   ========================================================================= */
#define US_CSR_RXRDY    (1U << 0U)  /* Receiver Ready        — Página 1228 */
#define US_CSR_TXRDY    (1U << 1U)  /* Transmitter Ready     — Página 1228 */
#define US_CSR_RXBRK    (1U << 2U)  /* Break Received        — Página 1228 */
#define US_CSR_OVRE     (1U << 5U)  /* Overrun Error         — Página 1228 */
#define US_CSR_FRAME    (1U << 6U)  /* Framing Error         — Página 1228 */
#define US_CSR_PARE     (1U << 7U)  /* Parity Error          — Página 1228 */
#define US_CSR_TXEMPTY  (1U << 9U)  /* Transmitter Empty     — Página 1228 */
<<<<<<< HEAD
 
/* =========================================================================
   Configuração — valores em board.h
   ========================================================================= */
#define USART_BAUDRATE   115200U
 
/* =========================================================================
   Macros de acesso aos registos
   ========================================================================= */
#define REG(addr)     (*(volatile uint32_t *)(addr))
#define USART1_CR     REG(USART1_BASE + US_CR_OFFSET)
#define USART1_MR     REG(USART1_BASE + US_MR_OFFSET)
#define USART1_CSR    REG(USART1_BASE + US_CSR_OFFSET)
#define USART1_RHR    REG(USART1_BASE + US_RHR_OFFSET)
#define USART1_THR    REG(USART1_BASE + US_THR_OFFSET)
#define USART1_BRGR   REG(USART1_BASE + US_BRGR_OFFSET)


uint8_t hal_usart_init(void);
uint8_t hal_usart_is_tx_ready(void);
void hal_usart_write_char(uint8_t data);
uint8_t hal_usart_data_available(void);
uint8_t hal_usart_read_char(void);
void hal_usart_prepare_rx(void);

/* Funções de controlo para simulação/testes */
void hal_usart_set_tx_ready(uint8_t v);
void hal_usart_set_rx_auto_regen(uint8_t v);
=======

/* =========================================================================
   Configuração
   ========================================================================= */
#define USART_BAUDRATE  9600U

/* =========================================================================
   Macros de acesso aos registos — USART0 (EXT1: PB00=RXD0, PB01=TXD0)
   ========================================================================= */
#define REG_US(addr)    (*(volatile uint32_t *)(addr))
#define USART0_CR       REG_US(USART0_BASE + US_CR_OFFSET)
#define USART0_MR       REG_US(USART0_BASE + US_MR_OFFSET)
#define USART0_CSR      REG_US(USART0_BASE + US_CSR_OFFSET)
#define USART0_RHR      REG_US(USART0_BASE + US_RHR_OFFSET)
#define USART0_THR      REG_US(USART0_BASE + US_THR_OFFSET)
#define USART0_BRGR     REG_US(USART0_BASE + US_BRGR_OFFSET)

/* =========================================================================
   Protótipos
   ========================================================================= */
uint8_t hal_usart_init(void);
uint8_t hal_usart_tx_ready(void);
void    hal_usart_write_byte(uint8_t byte);
uint8_t hal_usart_read_byte(void);
uint8_t hal_rx_data_availible(void);
>>>>>>> origin/OBC_board

#endif