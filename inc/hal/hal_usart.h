#ifndef HAL_USART_H
#define HAL_USART_H

#include <stdint.h>

/* =========================================================================
   Base Addresses — Secção 10, Figure 10-1. Product Mapping, Página 43
   ========================================================================= */
#define USART0_BASE  0x40024000U  /* Página 43 */
#define USART1_BASE  0x40028000U  /* Página 43 */
#define USART2_BASE  0x4002C000U  /* Página 43 */
/* =========================================================================
   Peripheral IDs — Secção 14, Table 14-1. Peripheral Identifiers, Páginas 57-58
   ========================================================================= */
#define ID_USART0    13U          /* Página 57 */
#define ID_USART1    14U          /* Página 57 */
#define ID_USART2    15U          /* Página 57 */

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
#define USART0_IER      REG_US(USART0_BASE + US_IER_OFFSET)
#define USART0_IDR      REG_US(USART0_BASE + US_IDR_OFFSET)
#define USART0_CSR      REG_US(USART0_BASE + US_CSR_OFFSET)
#define USART0_RHR      REG_US(USART0_BASE + US_RHR_OFFSET)
#define USART0_THR      REG_US(USART0_BASE + US_THR_OFFSET)
#define USART0_BRGR     REG_US(USART0_BASE + US_BRGR_OFFSET)

/* Interrupt enable bits */
#define US_IER_RXRDY    (1U << 0U)  /* Enable RXRDY interrupt */

/* NVIC — habilita/desabilita IRQ por número de periférico */
#define NVIC_ISER0      (*(volatile uint32_t *)0xE000E100UL)
#define NVIC_ISER1      (*(volatile uint32_t *)0xE000E104UL)  /* IRQs 32-63 */

/* Tamanho do circular buffer de RX (power of 2) — usado em sim mode */
#define RX_CIRC_BUF_SIZE  256U

/* =========================================================================
   XDMAC — eXtensible DMA Controller (Secção 27, Tabela 27-1, Página 647)
   ========================================================================= */
#define XDMAC_BASE              0x40078000UL
#define ID_XDMAC                58U   /* Tabela 14-1, Página 58 */

/* PMC — Power Management Controller (para ativar clock do XDMAC) */
#define PMC_BASE                0x400E0600UL
#define PMC_PCER1   (*(volatile uint32_t *)(PMC_BASE + 0x100U))  /* IDs 32-63 */

/* ---- Registos globais XDMAC ---- */
#define XDMAC_GIE   (*(volatile uint32_t *)(XDMAC_BASE + 0x00CU)) /* Global Interrupt Enable  */
#define XDMAC_GID   (*(volatile uint32_t *)(XDMAC_BASE + 0x010U)) /* Global Interrupt Disable */
#define XDMAC_GIS   (*(volatile uint32_t *)(XDMAC_BASE + 0x018U)) /* Global Interrupt Status  */
#define XDMAC_GE    (*(volatile uint32_t *)(XDMAC_BASE + 0x01CU)) /* Global Channel Enable    */
#define XDMAC_GD    (*(volatile uint32_t *)(XDMAC_BASE + 0x020U)) /* Global Channel Disable   */
#define XDMAC_GS    (*(volatile uint32_t *)(XDMAC_BASE + 0x024U)) /* Global Channel Status    */

/* ---- Registos por canal — canal n começa em 0x050 + n*0x040 ---- */
#define XDMAC_CH_BASE(n)   (XDMAC_BASE + 0x050U + (uint32_t)(n) * 0x040U)
#define XDMAC_CH_CIE(n)    (*(volatile uint32_t *)(XDMAC_CH_BASE(n) + 0x000U)) /* Interrupt Enable  */
#define XDMAC_CH_CID(n)    (*(volatile uint32_t *)(XDMAC_CH_BASE(n) + 0x004U)) /* Interrupt Disable */
#define XDMAC_CH_CIS(n)    (*(volatile uint32_t *)(XDMAC_CH_BASE(n) + 0x00CU)) /* Interrupt Status  */
#define XDMAC_CH_CSA(n)    (*(volatile uint32_t *)(XDMAC_CH_BASE(n) + 0x010U)) /* Source Address    */
#define XDMAC_CH_CDA(n)    (*(volatile uint32_t *)(XDMAC_CH_BASE(n) + 0x014U)) /* Dest Address      */
#define XDMAC_CH_CUBC(n)   (*(volatile uint32_t *)(XDMAC_CH_BASE(n) + 0x018U)) /* Microblock Len    */
#define XDMAC_CH_CBC(n)    (*(volatile uint32_t *)(XDMAC_CH_BASE(n) + 0x01CU)) /* Block Count       */
#define XDMAC_CH_CC(n)     (*(volatile uint32_t *)(XDMAC_CH_BASE(n) + 0x020U)) /* Channel Config    */

/* ---- Bits do registo CIE/CIS ---- */
#define XDMAC_CI_BIS    (1U << 0U)  /* Block transfer done   */
#define XDMAC_CI_RBEIS  (1U << 4U)  /* Read bus error        */
#define XDMAC_CI_WBEIS  (1U << 5U)  /* Write bus error       */
#define XDMAC_CI_ROIIS  (1U << 6U)  /* Request overflow      */

/* ---- Valor de CC para USART0 RX → memória (peripheral-to-memory, byte) ----
 *
 *  TYPE  = 1  (transferência periférico)        — bit 0
 *  DSYNC = 1  (periférico → memória)            — bit 4
 *  SWREQ = 0  (pedido de hardware)              — bit 6
 *  CSIZE = 0  (1 beat por pedido)               — bits 10:8
 *  DWIDTH= 0  (byte)                            — bits 12:11
 *  SIF   = 1  (AHB-Lite Interface 1 — periférico) — bit 13
 *  DIF   = 0  (AHB-Lite Interface 0 — sistema)  — bit 14
 *  SAM   = 0  (endereço fixo — USART0 RHR)      — bits 17:16
 *  DAM   = 1  (incrementar — buffer destino)    — bits 19:18
 *  PERID = 7  (USART0 RX, Tabela 18-3)          — bits 29:24
 */
#define XDMAC_USART0_RX_PERID   7U

#define XDMAC_CC_USART0_RX  \
    ((1U  <<  0U) |   /* TYPE:  periférico                      */ \
     (0U  <<  4U) |   /* DSYNC: 0 = periférico→memória (RX)    */ \
     (0U  <<  8U) |   /* CSIZE: 1 beat por pedido              */ \
     (0U  << 11U) |   /* DWIDTH: byte                          */ \
     (1U  << 13U) |   /* SIF:   bus periférico (fonte = RHR)   */ \
     (0U  << 14U) |   /* DIF:   bus sistema    (dest  = SRAM)  */ \
     (0U  << 16U) |   /* SAM:   fixo (USART0_RHR)              */ \
     (1U  << 18U) |   /* DAM:   incrementar (buffer destino)   */ \
     ((uint32_t)XDMAC_USART0_RX_PERID << 24U))  /* PERID = 7  */

/* Canal DMA reservado para USART0 RX */
#define DMA_CH_USART0_RX    0U

/* =========================================================================
   Protótipos
   ========================================================================= */
uint8_t hal_usart_init(void);
uint8_t hal_usart_tx_ready(void);
void    hal_usart_write_byte(uint8_t byte);
uint8_t hal_usart_read_byte(void);
uint8_t hal_rx_data_availible(void);

/* DMA RX */
void    hal_usart_dma_recv(uint8_t *buf, uint16_t len);
uint8_t hal_usart_dma_done(void);
void    hal_usart_dma_clear(void);

#endif