/**
 * @file system_samv71.c
 * @brief Inicialização mínima do sistema para o bootloader (ATSAMV71Q21B).
 */

#include "system_samv71.h"
#include "qspi_boot.h"   /* ID_QSPI_PERIPH */

/* =========================================================================
 * Watchdog Timer (WDT) — desabilitar no arranque do bootloader
 * ========================================================================= */
#define WDT_BASE   0x400E1850UL
#define WDT_MR     (*(volatile uint32_t *)(WDT_BASE + 0x04U))
#define WDT_MR_WDDIS (1UL << 15)  /**< Watchdog Disable bit                  */

/* =========================================================================
 * Reinforced Safety Watchdog Timer (RSWDT) — também desabilitar
 * ========================================================================= */
#define RSWDT_BASE  0x400E1900UL
#define RSWDT_MR    (*(volatile uint32_t *)(RSWDT_BASE + 0x04U))
#define RSWDT_MR_WDDIS (1UL << 15)

/* =========================================================================
 * EFC — Flash wait states
 * ========================================================================= */
#define EFC_BASE_SYS  0x400E0C00UL
#define EFC_FMR_SYS   (*(volatile uint32_t *)(EFC_BASE_SYS + 0x00U))
/* FWS = 0 wait states para f < ~17 MHz (oscilador interno 12 MHz) */
#define EFC_FMR_FWS(v) (((v) & 0xFU) << 8)

/* =========================================================================
 * system_boot_init
 * ========================================================================= */
void system_boot_init(void)
{
    /* 1. Desabilita WDT — evita reset inesperado durante a operação OTA */
    WDT_MR  = WDT_MR_WDDIS;
    RSWDT_MR = RSWDT_MR_WDDIS;

    /* 2. Flash wait states: 0 WS é suficiente a 12 MHz (oscilador RC interno)
     *    Se o sistema arrancar com PLL activo (>17 MHz), ajustar aqui.
     *    A 12 MHz o reset default já funciona, mas ser explícito é mais seguro. */
    EFC_FMR_SYS = EFC_FMR_FWS(0U) | (1UL << 16); /* FAM=1: Full access mode   */

    /* 3. Activa clock do QSPI no PMC (peripheral ID 43 → PCER1 bit 11) */
    PMC_PCER1 = (1UL << (ID_QSPI_PERIPH - 32U));
}

/* =========================================================================
 * system_cache_disable
 * ========================================================================= */
void system_cache_disable(void)
{
    /* Desabilita I-Cache */
    if (SCB_CCR & SCB_CCR_IC)
    {
        /* Barreira de instrução antes */
        __asm__ volatile ("dsb" ::: "memory");
        __asm__ volatile ("isb" ::: "memory");
        SCB_CCR &= ~SCB_CCR_IC;
        __asm__ volatile ("isb" ::: "memory");
    }
}

/* =========================================================================
 * system_cache_invalidate
 * ========================================================================= */
void system_cache_invalidate(void)
{
    /* Invalida toda a I-Cache */
    __asm__ volatile ("dsb" ::: "memory");
    SCB_ICIALLU = 0U;  /* Escrever qualquer valor invalida a I-Cache */
    __asm__ volatile ("dsb" ::: "memory");
    __asm__ volatile ("isb" ::: "memory");

    /* Re-habilita I-Cache */
    SCB_CCR |= SCB_CCR_IC;
    __asm__ volatile ("dsb" ::: "memory");
    __asm__ volatile ("isb" ::: "memory");
}

/* =========================================================================
 * system_prepare_jump
 * ========================================================================= */
void system_prepare_jump(void)
{
    /* Desabilita SysTick */
    SYST_CSR = 0U;

    /* Desabilita todas as interrupções externas (NVIC) */
    NVIC_ICER0 = 0xFFFFFFFFUL;
    NVIC_ICER1 = 0xFFFFFFFFUL;
    NVIC_ICER2 = 0xFFFFFFFFUL;
    NVIC_ICER3 = 0xFFFFFFFFUL;

    /* Barreira de memória para garantir que as escritas nos registos
     * ficam completas antes do salto */
    __asm__ volatile ("dsb" ::: "memory");
    __asm__ volatile ("isb" ::: "memory");
}
