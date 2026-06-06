/**
 * @file system_samv71.c
 * @brief Inicialização mínima do sistema para o bootloader (ATSAMV71Q21B).
 */

#include "system_samv71.h"
#include "qspi_boot.h" /* ID_QSPI_PERIPH */

/* =========================================================================
 * Watchdog Timer (WDT) — desabilitar no arranque do bootloader
 * ========================================================================= */
#define WDT_BASE 0x400E1850UL
#define WDT_MR (*(volatile uint32_t *)(WDT_BASE + 0x04U))
#define WDT_MR_WDDIS (1UL << 15) /**< Watchdog Disable bit                  */

/* =========================================================================
 * Reinforced Safety Watchdog Timer (RSWDT) — também desabilitar
 * ========================================================================= */
#define RSWDT_BASE 0x400E1900UL
#define RSWDT_MR (*(volatile uint32_t *)(RSWDT_BASE + 0x04U))
#define RSWDT_MR_WDDIS (1UL << 15)

/* =========================================================================
 * EFC — Flash wait states
 * ========================================================================= */
#define EFC_BASE_SYS 0x400E0C00UL
#define EFC_FMR_SYS (*(volatile uint32_t *)(EFC_BASE_SYS + 0x00U))
/* FWS = 0 wait states para f < ~17 MHz (oscilador interno 12 MHz) */
#define EFC_FMR_FWS(v) (((v) & 0xFU) << 8)

/* =========================================================================
 * system_boot_init
 * ========================================================================= */
void system_boot_init(void)
{
#define SCB_CCR_REG (*(volatile uint32_t *)0xE000ED14UL)
   EFC_FMR_SYS = EFC_FMR_FWS(1U) | (1UL << 16);
    __asm__ volatile("dsb" ::: "memory");
    __asm__ volatile("isb" ::: "memory");

    /* 0. Set GPNVM bit 1 (boot from flash). */
    {
        volatile uint32_t *fsr = (volatile uint32_t *)0x400E0C08UL;
        volatile uint32_t *fcr = (volatile uint32_t *)0x400E0C04UL;
        uint32_t t;
        t = 2000000UL;
        while (!(*fsr & 1UL) && t) { t--; }
        *fcr = (0x5AUL << 24U) | (1UL << 8U) | 0x0BUL;
        t = 2000000UL;
        while (!(*fsr & 1UL) && t) { t--; }
    }

    /* 1. Desabilita WDT */
    WDT_MR = WDT_MR_WDDIS;
    RSWDT_MR = RSWDT_MR_WDDIS;

    /* (REMOVIDO o segundo EFC_FMR_SYS que punha FWS=0) */

    /* 3. Activa clock do QSPI no PMC */
    PMC_PCER1 = (1UL << (ID_QSPI_PERIPH - 32U));
}

/* =========================================================================
 * system_cache_disable
 * ========================================================================= */
void system_cache_disable(void)
{
    __asm__ volatile("dsb" ::: "memory");
    __asm__ volatile("isb" ::: "memory");

    /* Desliga I-Cache */
    SCB_CCR &= ~SCB_CCR_IC;

    /* Desliga D-Cache — SEM isto as escritas ao page latch do EFC
     * ficam no write-buffer/cache e o WP comita um latch vazio. */
    SCB_CCR &= ~(1UL << 16); /* DC bit */

    __asm__ volatile("dsb" ::: "memory");
    __asm__ volatile("isb" ::: "memory");
}

/* =========================================================================
 * system_cache_invalidate
 * ========================================================================= */
void system_cache_invalidate(void)
{
    __asm__ volatile("dsb" ::: "memory");

    /* Invalida I-Cache */
    SCB_ICIALLU = 0U;

    /* Invalida toda a D-Cache por set/way */
    /* SAMV71 Cortex-M7: 4-way, 128 sets, linha 32B (16KB D-Cache) */
    for (uint32_t set = 0U; set < 128U; set++)
    {
        for (uint32_t way = 0U; way < 4U; way++)
        {
            uint32_t r = (way << 30) | (set << 5);
            *(volatile uint32_t *)0xE000EF60UL = r;  /* DCISW */
        }
    }

    __asm__ volatile("dsb" ::: "memory");
    __asm__ volatile("isb" ::: "memory");

    /* NAO religa as caches — deixa OFF ate ao salto.
     * A app reconfigura no seu arranque. */
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
    __asm__ volatile("dsb" ::: "memory");
    __asm__ volatile("isb" ::: "memory");
}
