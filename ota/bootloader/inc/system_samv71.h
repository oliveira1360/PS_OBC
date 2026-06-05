    /**
     * @file system_samv71.h
     * @brief Inicialização mínima do sistema para o bootloader (ATSAMV71Q21B).
     *
     * O bootloader arranca com o oscilador interno a ~12 MHz.
     * Não é necessário configurar PLL — o bootloader não necessita de alta
     * velocidade. Os wait states da flash interna a 12 MHz são 0.
     *
     * Registos Cortex-M7 usados:
     *  - SCB->VTOR : Vector Table Offset Register (para redirect para a app)
     *  - SCB->ICSR : Interrupt Control and State Register
     */

    #ifndef SYSTEM_SAMV71_H
    #define SYSTEM_SAMV71_H

    #include <stdint.h>

    /* =========================================================================
    * Cortex-M7 System Control Block (SCB)
    * ========================================================================= */
    #define SCB_BASE    0xE000ED00UL
    #define SCB_VTOR    (*(volatile uint32_t *)(SCB_BASE + 0x08U))  /**< Vector Table Offset */
    #define SCB_AIRCR   (*(volatile uint32_t *)(SCB_BASE + 0x0CU))  /**< App Interrupt/Reset */
    #define SCB_CCR     (*(volatile uint32_t *)(SCB_BASE + 0x14U))  /**< Config/Control Reg  */

    /* Cortex-M7 Cache Control (para invalidar I-Cache após escrever na flash) */
    #define SCB_ICIALLU (*(volatile uint32_t *)(0xE000EF50UL))  /**< I-Cache Invalidate All */
    #define SCB_CCR_IC  (1UL << 17)  /**< I-Cache enable bit                       */
    #define SCB_CCR_DC  (1UL << 16)  /**< D-Cache enable bit                       */

    /* =========================================================================
    * NVIC — desabilitar todas as interrupções antes de saltar para a app
    * ========================================================================= */
    #define NVIC_ICER0  (*(volatile uint32_t *)0xE000E180UL)
    #define NVIC_ICER1  (*(volatile uint32_t *)0xE000E184UL)
    #define NVIC_ICER2  (*(volatile uint32_t *)0xE000E188UL)
    #define NVIC_ICER3  (*(volatile uint32_t *)0xE000E18CUL)

    /* =========================================================================
    * SysTick — desabilitar antes de saltar para a app
    * ========================================================================= */
    #define SYST_CSR    (*(volatile uint32_t *)0xE000E010UL)

    /* =========================================================================
    * PMC — Power Management Controller
    * ========================================================================= */
    #define PMC_BASE_SYS         0x400E0600UL
    #define PMC_PCER0     (*(volatile uint32_t *)(PMC_BASE_SYS + 0x10U))
    #define PMC_PCER1     (*(volatile uint32_t *)(PMC_BASE_SYS + 0x100U))

    /* =========================================================================
    * Protótipos
    * ========================================================================= */

    /**
     * @brief Inicialização mínima do sistema para o bootloader.
     *
     * Desabilita watchdog, configura flash wait states (0 WS a 12 MHz),
     * e activa o clock do QSPI.
     */
    void system_boot_init(void);

    /**
     * @brief Desabilita I-Cache e D-Cache antes de escrever na flash interna.
     *
     * Deve ser chamada antes de flash_efc_write_firmware() para evitar
     * que a cache sirva dados desactualizados após a escrita.
     */
    /**
     * @brief Invalida e reabilita I-Cache após escrever na flash interna.
     */
    void system_cache_invalidate(void);

    /**
     * @brief Desabilita todas as interrupções e periféricos antes de
     *        saltar para a aplicação.
     */
    void system_prepare_jump(void);

    void system_cache_disable(void);
    #endif /* SYSTEM_SAMV71_H */
