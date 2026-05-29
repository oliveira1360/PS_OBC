/**
 * @file startup_samv71.c
 * @brief Startup mínimo do bootloader para ATSAMV71Q21B.
 *
 * Contém:
 *  - Tabela de vectores de excepção (colocada em .isr_vector no início da flash)
 *  - Reset_Handler: copia .data, zera .bss, chama main()
 *  - Default_Handler: loop infinito para excepções não tratadas
 *
 * A tabela de vectores fica em 0x00400000 (início da flash interna),
 * conforme definido no linker script samv71q21_boot.ld.
 */

#include <stdint.h>

/* =========================================================================
 * Símbolos exportados pelo linker script
 * ========================================================================= */
extern uint32_t _estack;      /**< Topo da stack (definido no .ld)            */
extern uint32_t _sdata;       /**< Início de .data em RAM                     */
extern uint32_t _edata;       /**< Fim de .data em RAM                        */
extern uint32_t _sidata;      /**< LMA de .data (cópia na flash)              */
extern uint32_t _sbss;        /**< Início de .bss                             */
extern uint32_t _ebss;        /**< Fim de .bss                                */

/* =========================================================================
 * Declarações forward
 * ========================================================================= */
void Reset_Handler(void);
void Default_Handler(void);
int  main(void);

/* =========================================================================
 * Handlers fracos — podem ser substituídos noutros ficheiros
 * ========================================================================= */
#define WEAK_DEFAULT __attribute__((weak, alias("Default_Handler")))

void NMI_Handler(void)        WEAK_DEFAULT;
void MemManage_Handler(void)  WEAK_DEFAULT;
void UsageFault_Handler(void) WEAK_DEFAULT;
void SVC_Handler(void)        WEAK_DEFAULT;
void DebugMon_Handler(void)   WEAK_DEFAULT;
void PendSV_Handler(void)     WEAK_DEFAULT;
void SysTick_Handler(void)    WEAK_DEFAULT;

/* =========================================================================
 * BusFault_Handler e HardFault_Handler — saltam para a app em vez de
 * ficarem presos. Necessário para recuperar de falhas QSPI memory-mapped.
 * APP_START_ADDR = 0x00410000: vector[0]=SP inicial, vector[1]=Reset_Handler
 * ========================================================================= */
__attribute__((naked))
void BusFault_Handler(void)
{
    __asm__ volatile (
        "ldr r0, =0x00410000  \n"  /* Base da tabela de vectores da app    */
        "ldr r1, [r0, #0]     \n"  /* vector[0]: SP inicial da app         */
        "ldr r2, [r0, #4]     \n"  /* vector[1]: Reset_Handler da app      */
        "msr msp, r1          \n"  /* Configura MSP para a app             */
        "bx  r2               \n"  /* Salta para Reset_Handler da app      */
    );
}

__attribute__((naked))
void HardFault_Handler(void)
{
    __asm__ volatile (
        "ldr r0, =0x00410000  \n"
        "ldr r1, [r0, #0]     \n"
        "ldr r2, [r0, #4]     \n"
        "msr msp, r1          \n"
        "bx  r2               \n"
    );
}

/* =========================================================================
 * Tabela de vectores — DEVE estar em 0x00400000 (secção .isr_vector)
 * ========================================================================= */
__attribute__((section(".isr_vector"), used))
const uint32_t g_pfnVectors[] = {
    /* Stack pointer inicial */
    (uint32_t)&_estack,

    /* Excepções Cortex-M7 */
    (uint32_t)Reset_Handler,
    (uint32_t)NMI_Handler,
    (uint32_t)HardFault_Handler,
    (uint32_t)MemManage_Handler,
    (uint32_t)BusFault_Handler,
    (uint32_t)UsageFault_Handler,
    0U, 0U, 0U, 0U,               /* Reservado */
    (uint32_t)SVC_Handler,
    (uint32_t)DebugMon_Handler,
    0U,                            /* Reservado */
    (uint32_t)PendSV_Handler,
    (uint32_t)SysTick_Handler,

    /* IRQs externas — o bootloader não as usa; todas apontam para Default */
    /* Periféricos SAM V71 (IRQ0 a IRQ62) */
    (uint32_t)Default_Handler,  /* IRQ0:  SUPC         */
    (uint32_t)Default_Handler,  /* IRQ1:  RSTC         */
    (uint32_t)Default_Handler,  /* IRQ2:  RTC          */
    (uint32_t)Default_Handler,  /* IRQ3:  RTT          */
    (uint32_t)Default_Handler,  /* IRQ4:  WDT          */
    (uint32_t)Default_Handler,  /* IRQ5:  PMC          */
    (uint32_t)Default_Handler,  /* IRQ6:  EFC          */
    (uint32_t)Default_Handler,  /* IRQ7:  UART0        */
    (uint32_t)Default_Handler,  /* IRQ8:  UART1        */
    (uint32_t)Default_Handler,  /* IRQ9:  Reserved     */
    (uint32_t)Default_Handler,  /* IRQ10: PIOA         */
    (uint32_t)Default_Handler,  /* IRQ11: PIOB         */
    (uint32_t)Default_Handler,  /* IRQ12: PIOC         */
    (uint32_t)Default_Handler,  /* IRQ13: USART0       */
    (uint32_t)Default_Handler,  /* IRQ14: USART1       */
    (uint32_t)Default_Handler,  /* IRQ15: USART2       */
    (uint32_t)Default_Handler,  /* IRQ16: PIOD         */
    (uint32_t)Default_Handler,  /* IRQ17: PIOE         */
    (uint32_t)Default_Handler,  /* IRQ18: HSMCI        */
    (uint32_t)Default_Handler,  /* IRQ19: TWI0         */
    (uint32_t)Default_Handler,  /* IRQ20: TWI1         */
    (uint32_t)Default_Handler,  /* IRQ21: SPI0         */
    (uint32_t)Default_Handler,  /* IRQ22: SSC          */
    (uint32_t)Default_Handler,  /* IRQ23: TC0          */
    (uint32_t)Default_Handler,  /* IRQ24: TC1          */
    (uint32_t)Default_Handler,  /* IRQ25: TC2          */
    (uint32_t)Default_Handler,  /* IRQ26: TC3          */
    (uint32_t)Default_Handler,  /* IRQ27: TC4          */
    (uint32_t)Default_Handler,  /* IRQ28: TC5          */
    (uint32_t)Default_Handler,  /* IRQ29: AFEC0        */
    (uint32_t)Default_Handler,  /* IRQ30: DACC         */
    (uint32_t)Default_Handler,  /* IRQ31: PWM0         */
    (uint32_t)Default_Handler,  /* IRQ32: ICM          */
    (uint32_t)Default_Handler,  /* IRQ33: ACC          */
    (uint32_t)Default_Handler,  /* IRQ34: USBHS        */
    (uint32_t)Default_Handler,  /* IRQ35: MCAN0        */
    (uint32_t)Default_Handler,  /* IRQ36: Reserved     */
    (uint32_t)Default_Handler,  /* IRQ37: MCAN1        */
    (uint32_t)Default_Handler,  /* IRQ38: Reserved     */
    (uint32_t)Default_Handler,  /* IRQ39: GMAC         */
    (uint32_t)Default_Handler,  /* IRQ40: AFEC1        */
    (uint32_t)Default_Handler,  /* IRQ41: TWI2         */
    (uint32_t)Default_Handler,  /* IRQ42: SPI1         */
    (uint32_t)Default_Handler,  /* IRQ43: QSPI         */
    (uint32_t)Default_Handler,  /* IRQ44: UART2        */
    (uint32_t)Default_Handler,  /* IRQ45: UART3        */
    (uint32_t)Default_Handler,  /* IRQ46: UART4        */
    (uint32_t)Default_Handler,  /* IRQ47: TC6          */
    (uint32_t)Default_Handler,  /* IRQ48: TC7          */
    (uint32_t)Default_Handler,  /* IRQ49: TC8          */
    (uint32_t)Default_Handler,  /* IRQ50: TC9          */
    (uint32_t)Default_Handler,  /* IRQ51: TC10         */
    (uint32_t)Default_Handler,  /* IRQ52: TC11         */
    (uint32_t)Default_Handler,  /* IRQ53: MLB          */
    (uint32_t)Default_Handler,  /* IRQ54: Reserved     */
    (uint32_t)Default_Handler,  /* IRQ55: Reserved     */
    (uint32_t)Default_Handler,  /* IRQ56: AES          */
    (uint32_t)Default_Handler,  /* IRQ57: TRNG         */
    (uint32_t)Default_Handler,  /* IRQ58: XDMAC        */
    (uint32_t)Default_Handler,  /* IRQ59: ISI          */
    (uint32_t)Default_Handler,  /* IRQ60: PWM1         */
    (uint32_t)Default_Handler,  /* IRQ61: FPU          */
    (uint32_t)Default_Handler,  /* IRQ62: Reserved     */
    (uint32_t)Default_Handler,  /* IRQ63: RSWDT        */
    (uint32_t)Default_Handler,  /* IRQ64: CCW          */
    (uint32_t)Default_Handler,  /* IRQ65: CCF          */
    (uint32_t)Default_Handler,  /* IRQ66: GMAC_Q1      */
    (uint32_t)Default_Handler,  /* IRQ67: GMAC_Q2      */
    (uint32_t)Default_Handler,  /* IRQ68: IXC          */
    (uint32_t)Default_Handler,  /* IRQ69: I2SC0        */
    (uint32_t)Default_Handler,  /* IRQ70: I2SC1        */
    (uint32_t)Default_Handler,  /* IRQ71: GMAC_Q3      */
    (uint32_t)Default_Handler,  /* IRQ72: GMAC_Q4      */
    (uint32_t)Default_Handler,  /* IRQ73: GMAC_Q5      */
};

/* =========================================================================
 * Reset_Handler
 * ========================================================================= */
__attribute__((naked, noreturn))
void Reset_Handler(void)
{
    /* Copia secção .data da flash para RAM */
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;
    while (dst < &_edata)
    {
        *dst++ = *src++;
    }

    /* Zera secção .bss */
    dst = &_sbss;
    while (dst < &_ebss)
    {
        *dst++ = 0U;
    }

    /* Chama bootloader main */
    (void)main();

    /* Nunca deve chegar aqui */
    while (1)
    {
        /* loop de segurança */
    }
}

/* =========================================================================
 * Default_Handler — loop infinito para excepções não tratadas
 * ========================================================================= */
void Default_Handler(void)
{
    while (1)
    {
        /* Travar aqui permite depurar com o debugger */
    }
}
