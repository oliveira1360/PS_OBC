/**
 * @file main.c
 * @brief Ponto de entrada do bootloader OTA — ATSAMV71Q21B.
 *
 * O main() é o ponto de entrada após o Reset_Handler do startup.
 * Inicializa o sistema e delega toda a lógica para bootloader_run(),
 * que não retorna (salta para a aplicação ou fica em loop de segurança).
 */

#include "bootloader.h"
#include "../inc/system_samv71.h"

// #pragma config BOOT_MODE = SET

int main(void)
{
    debug_uart_init();
    debug_uart_puts("\n[BOOT] test boot print\n");
    for (volatile uint32_t d = 0; d < 200000U; d++)
    {
    }

    /* Inicialização mínima do sistema:
     *  - Desabilita WDT e RSWDT
     *  - Configura flash wait states (0 WS @ 12 MHz)
     *  - Activa clock do QSPI no PMC */
    system_boot_init();

    (void)bootloader_run();

    /* Segurança: se bootloader_run() retornar inesperadamente */
    while (1)
    {
        /* loop de segurança */
    }

    return 0; /* Nunca alcançado */
}

#define DBG_REG(addr) (*(volatile uint32_t *)(addr))

/* USART1 registers */
#define US1_CR_ADDR 0x40028000UL
#define US1_MR_ADDR 0x40028004UL
#define US1_CSR_ADDR 0x40028014UL
#define US1_THR_ADDR 0x4002801CUL
#define US1_BRGR_ADDR 0x40028020UL

/* Bits */
#define US_CSR_TXRDY (1UL << 1)

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
    DBG_REG(0x400E0E04) = (1UL << 21);   /* PIOA_PDR */
    DBG_REG(0x400E0E70) &= ~(1UL << 21); /* ABCDSR0 = 0 */
    DBG_REG(0x400E0E74) &= ~(1UL << 21); /* ABCDSR1 = 0 → Periph A */

    /* 5. PB04 (TXD1) → Peripheral D */
    DBG_REG(0x400E1004) = (1UL << 4);  /* PIOB_PDR */
    DBG_REG(0x400E1070) |= (1UL << 4); /* ABCDSR0 = 1 */
    DBG_REG(0x400E1074) |= (1UL << 4); /* ABCDSR1 = 1 → Periph D */

    /* 6. Reset USART1 */
    DBG_REG(US1_CR_ADDR) = (1UL << 2) | (1UL << 3); /* RSTRX | RSTTX */

    /* 7. Mode: 8N1 */
    DBG_REG(US1_MR_ADDR) = (0x3UL << 6) | (0x4UL << 9);

    /* 8. Baud rate: divisor 78 (MCK ~4MHz, 9600 baud no PuTTY) */
    DBG_REG(US1_BRGR_ADDR) = 78U;

    /* 9. Enable TX e RX */
    DBG_REG(US1_CR_ADDR) = (1UL << 4) | (1UL << 6); /* RXEN | TXEN */

    debug_uart_ready = 1U;
}

/**
 * @brief Envia um caractere pelo USART1 (bloqueante).
 */
static void debug_uart_putc(char c)
{
    /* Timeout evita bloqueio infinito se UART TX ficar preso */
    uint32_t t = 0x100000U;
    while (((DBG_REG(US1_CSR_ADDR) & US_CSR_TXRDY) == 0U) && --t)
    {
    }
    if (t != 0U)
    {
        DBG_REG(US1_THR_ADDR) = (uint32_t)c;
    }
}

/**
 * @brief Função _mon_putc requerida pelo XC32 para redirecionar printf().
 */
void _mon_putc(char c)
{
    if (!debug_uart_ready)
        return;

    if (c == '\n')
    {
        debug_uart_putc('\r');
    }
    debug_uart_putc(c);
}

/**
 * @brief Envia uma string diretamente pela UART, sem usar a biblioteca <stdio.h>
 */
void debug_uart_puts(const char *s)
{
    if (!debug_uart_ready)
        return;

    while (*s)
    {
        if (*s == '\n')
        {
            // Reutilizamos a sua função estática
            extern void _mon_putc(char c);
            _mon_putc('\r');
        }
        _mon_putc(*s);
        s++;
    }
}

void debug_uart_hex(const char *label, uint32_t val)
{
    debug_uart_puts(label);
    const char *hex = "0123456789ABCDEF";
    char buf[11];
    buf[0] = '0';
    buf[1] = 'x';
    for (int i = 0; i < 8; i++)
        buf[2 + i] = hex[(val >> ((7 - i) * 4)) & 0xF];
    buf[10] = '\0';
    debug_uart_puts(buf);
    debug_uart_puts("\n");
}