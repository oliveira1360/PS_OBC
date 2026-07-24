/**
 * @file hal_system.c
 * @brief Simulação da Hardware Abstraction Layer (HAL) para controlo de sistema.
 *
 * Este módulo contém funções simuladas para operações de nível de sistema, 
 * como o reset do microcontrolador, controlo do watchdog timer e rotinas 
 * de autoteste (BIST - Built-In Self-Test) dos vários periféricos. Na versão 
 * atual de simulação, os testes retornam sempre sucesso.
 */

#include "hal/hal_system.h"
#include <stdint.h>

/* =========================================================================
 * PMC (Power Management Controller) — registos para configurar 300 MHz
 * ========================================================================= */
#define PMC_BASE        0x400E0600UL
#define CKGR_MOR        (*(volatile uint32_t *)(PMC_BASE + 0x20U))
#define CKGR_PLLAR      (*(volatile uint32_t *)(PMC_BASE + 0x28U))
#define PMC_MCKR        (*(volatile uint32_t *)(PMC_BASE + 0x30U))
#define PMC_SR          (*(volatile uint32_t *)(PMC_BASE + 0x68U))

#define MOR_KEY             (0x37UL << 16)  /* password obrigatória */
#define MOR_MOSCRCEN        (1UL << 3)      /* enable RC interno */
#define MOR_MOSCRCF_12MHZ   (2UL << 4)      /* RC a 12 MHz */

#define SR_LOCKA        (1UL << 1)          /* PLLA locked */
#define SR_MCKRDY       (1UL << 3)          /* Master clock ready */
#define SR_MOSCRCS      (1UL << 17)         /* RC estabilizado */

#define PLLAR_ONE       (1UL << 29)         /* bit obrigatório a 1 */
#define PLLAR_MULA(v)   (((v) & 0x7FFUL) << 16)
#define PLLAR_COUNT     (0x3FUL << 8)       /* ciclos SLCK até lock */
#define PLLAR_DIVA(v)   ((v) & 0xFFUL)

#define MCKR_CSS_MASK   (3UL << 0)
#define MCKR_CSS_PLLA   (2UL << 0)
#define MCKR_PRES_MASK  (7UL << 4)
#define MCKR_PRES_CLK_1 (0UL << 4)
#define MCKR_MDIV_MASK  (3UL << 8)
#define MCKR_MDIV_DIV2  (1UL << 8)          /* MCK = HCLK/2 */

/* EEFC — Flash wait states */
#define EEFC_FMR        (*(volatile uint32_t *)0x400E0C00UL)
#define EEFC_FMR_FWS(v) (((v) & 0xFUL) << 8)
#define EEFC_FMR_CLOE   (1UL << 16)

/**
 * @brief Configura o sistema para 300 MHz (HCLK) / 150 MHz (MCK).
 *
 * Fonte: RC interno de 12 MHz → PLLA ×25 = 300 MHz.
 * MDIV=2 → MCK (periféricos) = 150 MHz, o máximo permitido no SAMV71.
 * Chamar ANTES de inicializar SysTick/USART/I2C/etc., porque os
 * divisores desses periféricos assumem já o clock novo.
 */
void hal_clock_init_300mhz(void)
{
    uint32_t mckr;

    EEFC_FMR = EEFC_FMR_FWS(5U) | EEFC_FMR_CLOE;

    CKGR_MOR = MOR_KEY | MOR_MOSCRCEN | MOR_MOSCRCF_12MHZ;
    while (!(PMC_SR & SR_MOSCRCS)) {}

    CKGR_PLLAR = PLLAR_ONE | PLLAR_MULA(24U) | PLLAR_COUNT | PLLAR_DIVA(1U);
    while (!(PMC_SR & SR_LOCKA)) {}

    mckr = PMC_MCKR;
    mckr = (mckr & ~MCKR_PRES_MASK) | MCKR_PRES_CLK_1;
    PMC_MCKR = mckr;
    while (!(PMC_SR & SR_MCKRDY)) {}

    mckr = (mckr & ~MCKR_MDIV_MASK) | MCKR_MDIV_DIV2;
    PMC_MCKR = mckr;
    while (!(PMC_SR & SR_MCKRDY)) {}

    mckr = (mckr & ~MCKR_CSS_MASK) | MCKR_CSS_PLLA;
    PMC_MCKR = mckr;
    while (!(PMC_SR & SR_MCKRDY)) {}
}

/**
 * @brief Executa um reset por software ao sistema (Simulação).
 *
 * Em hardware real, esta função acionaria o registo de reset do 
 * microcontrolador (ex: NVIC_SystemReset no ARM Cortex). Na simulação, 
 * a função está vazia.
 */
void hal_system_reset(void)
{
    /* NVIC_SystemReset — Cortex-M7 SCB AIRCR
     * VECTKEY=0x05FA nos bits [31:16], SYSRESETREQ no bit 2. */
    volatile uint32_t *AIRCR = (volatile uint32_t *)0xE000ED0CUL;
    *AIRCR = (0x05FAUL << 16U) | (1UL << 2U);

    __asm__ volatile ("dsb" ::: "memory");
    __asm__ volatile ("isb" ::: "memory");

    while (1) {}   /* nunca chega aqui */
}

/**
 * @brief Alimenta ("kicks") o Watchdog Timer (Simulação).
 *
 * Em hardware real, esta função deve ser chamada periodicamente para 
 * evitar que o Watchdog reinicie o sistema por inatividade. Na simulação,
 * a função está vazia.
 */
void hal_watchdog_kick(void)
{
}

/**
 * @brief Executa o autoteste do barramento I2C (Simulação).
 *
 * @return 1 indicando que o teste passou com sucesso.
 */
int hal_self_test_i2c(void)
{
    return 1;
}

/**
 * @brief Executa o autoteste do barramento SPI (Simulação).
 *
 * @return 1 indicando que o teste passou com sucesso.
 */
int hal_self_test_spi(void)
{
    return 1;
}

/**
 * @brief Executa o autoteste do barramento QSPI (Simulação).
 *
 * @return 1 indicando que o teste passou com sucesso.
 */
int hal_self_test_qspi(void)
{
    return 1;
}

/**
 * @brief Executa o autoteste dos periféricos USART (Simulação).
 *
 * @return 1 indicando que o teste passou com sucesso.
 */
int hal_self_test_usart(void)
{
    return 1;
}

/**
 * @brief Executa o autoteste dos pinos GPIO (Simulação).
 *
 * @return 1 indicando que o teste passou com sucesso.
 */
int hal_self_test_gpio(void)
{
    return 1;
}

/**
 * @brief Executa o autoteste da memória interna/externa (Simulação).
 *
 * @return 1 indicando que o teste passou com sucesso.
 */
int hal_self_test_memory(void)
{
    return 1;
}