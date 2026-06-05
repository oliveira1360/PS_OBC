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