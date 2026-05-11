/**
 * @file hal_debug_uart.c
 * @brief Printf via EDBG Virtual COM Port (USART1) do SAM V71 Xplained Ultra.
 *
 * PA21 = RXD1 (Peripheral A)
 * PB04 = TXD1 (Peripheral D)
 * EDBG CDC → PuTTY no PC via DEBUG USB a 9600 baud
 *
 * Valores confirmados empiricamente:
 *   CCFG_SYSIO (MATRIX) = 0x40088114 — liberta PB04 do JTAG/TDI
 *   PB04 Peripheral D: ABCDSR0=1, ABCDSR1=1
 *   MCK ≈ 4 MHz no boot (RC interno sem PLL)
 *   Divisor 78 → ~3200 baud? Funciona com PuTTY a 9600
 */

#include <stdint.h>

#define DBG_REG(addr) (*(volatile uint32_t *)(addr))

/* USART1 registers */
#define US1_CR_ADDR    0x40028000UL
#define US1_MR_ADDR    0x40028004UL
#define US1_CSR_ADDR   0x40028014UL
#define US1_THR_ADDR   0x4002801CUL
#define US1_BRGR_ADDR  0x40028020UL

/* Bits */
#define US_CSR_TXRDY   (1UL << 1)

static uint8_t debug_uart_ready = 0U;

/**
 * @brief Inicializa USART1 para debug via EDBG.
 * Chamar uma vez no início do main(), antes de qualquer printf().
 * PuTTY: COM3, 9600 baud, 8N1.
 */
void debug_uart_init(void)
{
    /* 1. Desativar Watchdog */
    DBG_REG(0x400E1854) = (1UL << 15);

    /* 2. Liberta PB04 do JTAG (TDI) via MATRIX CCFG_SYSIO */
    DBG_REG(0x40088114) |= (1UL << 4);

    /* 3. Clock PIOA, PIOB, USART1 no PMC */
    DBG_REG(0x400E0610) = (1UL << 10) | (1UL << 11) | (1UL << 14);

    /* 4. PA21 (RXD1) → Peripheral A */
    DBG_REG(0x400E0E04) = (1UL << 21);          /* PIOA_PDR */
    DBG_REG(0x400E0E70) &= ~(1UL << 21);        /* ABCDSR0 = 0 */
    DBG_REG(0x400E0E74) &= ~(1UL << 21);        /* ABCDSR1 = 0 → Periph A */

    /* 5. PB04 (TXD1) → Peripheral D */
    DBG_REG(0x400E1004) = (1UL << 4);            /* PIOB_PDR */
    DBG_REG(0x400E1070) |= (1UL << 4);           /* ABCDSR0 = 1 */
    DBG_REG(0x400E1074) |= (1UL << 4);           /* ABCDSR1 = 1 → Periph D */

    /* 6. Reset USART1 */
    DBG_REG(US1_CR_ADDR) = (1UL << 2) | (1UL << 3);  /* RSTRX | RSTTX */

    /* 7. Mode: 8N1 */
    DBG_REG(US1_MR_ADDR) = (0x3UL << 6) | (0x4UL << 9);

    /* 8. Baud rate: divisor 78 (MCK ~4MHz, 9600 baud no PuTTY) */
    DBG_REG(US1_BRGR_ADDR) = 78U;

    /* 9. Enable TX e RX */
    DBG_REG(US1_CR_ADDR) = (1UL << 4) | (1UL << 6);  /* RXEN | TXEN */

    debug_uart_ready = 1U;
}

/**
 * @brief Envia um caractere pelo USART1 (bloqueante).
 */
static void debug_uart_putc(char c)
{
    while ((DBG_REG(US1_CSR_ADDR) & US_CSR_TXRDY) == 0U) {}
    DBG_REG(US1_THR_ADDR) = (uint32_t)c;
}

/**
 * @brief Função _mon_putc requerida pelo XC32 para redirecionar printf().
 */
void _mon_putc(char c)
{
    if (!debug_uart_ready)
        return;

    if (c == '\n') {
        debug_uart_putc('\r');
    }
    debug_uart_putc(c);
}