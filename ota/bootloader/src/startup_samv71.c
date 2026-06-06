/**
 * @file startup_samv71.c
 * @brief Startup mínimo do bootloader para ATSAMV71Q21B.
 */

#include <stdint.h>

/* Símbolos do linker script */
extern uint32_t _estack;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sidata;
extern uint32_t _sbss;
extern uint32_t _ebss;

void Reset_Handler(void);
void Default_Handler(void);
int  main(void);

#define WEAK_DEFAULT __attribute__((weak, alias("Default_Handler")))

extern void __pic32c_data_initialization(void);


void NMI_Handler(void)        WEAK_DEFAULT;
void MemManage_Handler(void)  WEAK_DEFAULT;
void UsageFault_Handler(void) WEAK_DEFAULT;
void SVC_Handler(void)        WEAK_DEFAULT;
void DebugMon_Handler(void)   WEAK_DEFAULT;
void PendSV_Handler(void)     WEAK_DEFAULT;
void SysTick_Handler(void)    WEAK_DEFAULT;

__attribute__((naked))
void BusFault_Handler(void)
{
    __asm__ volatile (
        "ldr r0, =0x00410000  \n"
        "ldr r1, [r0, #0]     \n"
        "ldr r2, [r0, #4]     \n"
        "msr msp, r1          \n"
        "bx  r2               \n"
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

__attribute__((section(".isr_vector"), used))
const uint32_t g_pfnVectors[] = {
    (uint32_t)&_estack,
    (uint32_t)Reset_Handler,
    (uint32_t)NMI_Handler,
    (uint32_t)HardFault_Handler,
    (uint32_t)MemManage_Handler,
    (uint32_t)BusFault_Handler,
    (uint32_t)UsageFault_Handler,
    0U, 0U, 0U, 0U,
    (uint32_t)SVC_Handler,
    (uint32_t)DebugMon_Handler,
    0U,
    (uint32_t)PendSV_Handler,
    (uint32_t)SysTick_Handler,
    (uint32_t)Default_Handler,  /* IRQ0  */
    (uint32_t)Default_Handler,  /* IRQ1  */
    (uint32_t)Default_Handler,  /* IRQ2  */
    (uint32_t)Default_Handler,  /* IRQ3  */
    (uint32_t)Default_Handler,  /* IRQ4  */
    (uint32_t)Default_Handler,  /* IRQ5  */
    (uint32_t)Default_Handler,  /* IRQ6  */
    (uint32_t)Default_Handler,  /* IRQ7  */
    (uint32_t)Default_Handler,  /* IRQ8  */
    (uint32_t)Default_Handler,  /* IRQ9  */
    (uint32_t)Default_Handler,  /* IRQ10 */
    (uint32_t)Default_Handler,  /* IRQ11 */
    (uint32_t)Default_Handler,  /* IRQ12 */
    (uint32_t)Default_Handler,  /* IRQ13 */
    (uint32_t)Default_Handler,  /* IRQ14 */
    (uint32_t)Default_Handler,  /* IRQ15 */
    (uint32_t)Default_Handler,  /* IRQ16 */
    (uint32_t)Default_Handler,  /* IRQ17 */
    (uint32_t)Default_Handler,  /* IRQ18 */
    (uint32_t)Default_Handler,  /* IRQ19 */
    (uint32_t)Default_Handler,  /* IRQ20 */
    (uint32_t)Default_Handler,  /* IRQ21 */
    (uint32_t)Default_Handler,  /* IRQ22 */
    (uint32_t)Default_Handler,  /* IRQ23 */
    (uint32_t)Default_Handler,  /* IRQ24 */
    (uint32_t)Default_Handler,  /* IRQ25 */
    (uint32_t)Default_Handler,  /* IRQ26 */
    (uint32_t)Default_Handler,  /* IRQ27 */
    (uint32_t)Default_Handler,  /* IRQ28 */
    (uint32_t)Default_Handler,  /* IRQ29 */
    (uint32_t)Default_Handler,  /* IRQ30 */
    (uint32_t)Default_Handler,  /* IRQ31 */
    (uint32_t)Default_Handler,  /* IRQ32 */
    (uint32_t)Default_Handler,  /* IRQ33 */
    (uint32_t)Default_Handler,  /* IRQ34 */
    (uint32_t)Default_Handler,  /* IRQ35 */
    (uint32_t)Default_Handler,  /* IRQ36 */
    (uint32_t)Default_Handler,  /* IRQ37 */
    (uint32_t)Default_Handler,  /* IRQ38 */
    (uint32_t)Default_Handler,  /* IRQ39 */
    (uint32_t)Default_Handler,  /* IRQ40 */
    (uint32_t)Default_Handler,  /* IRQ41 */
    (uint32_t)Default_Handler,  /* IRQ42 */
    (uint32_t)Default_Handler,  /* IRQ43 QSPI */
    (uint32_t)Default_Handler,  /* IRQ44 */
    (uint32_t)Default_Handler,  /* IRQ45 */
    (uint32_t)Default_Handler,  /* IRQ46 */
    (uint32_t)Default_Handler,  /* IRQ47 */
    (uint32_t)Default_Handler,  /* IRQ48 */
    (uint32_t)Default_Handler,  /* IRQ49 */
    (uint32_t)Default_Handler,  /* IRQ50 */
    (uint32_t)Default_Handler,  /* IRQ51 */
    (uint32_t)Default_Handler,  /* IRQ52 */
    (uint32_t)Default_Handler,  /* IRQ53 */
    (uint32_t)Default_Handler,  /* IRQ54 */
    (uint32_t)Default_Handler,  /* IRQ55 */
    (uint32_t)Default_Handler,  /* IRQ56 */
    (uint32_t)Default_Handler,  /* IRQ57 */
    (uint32_t)Default_Handler,  /* IRQ58 */
    (uint32_t)Default_Handler,  /* IRQ59 */
    (uint32_t)Default_Handler,  /* IRQ60 */
    (uint32_t)Default_Handler,  /* IRQ61 */
    (uint32_t)Default_Handler,  /* IRQ62 */
    (uint32_t)Default_Handler,  /* IRQ63 */
    (uint32_t)Default_Handler,  /* IRQ64 */
    (uint32_t)Default_Handler,  /* IRQ65 */
    (uint32_t)Default_Handler,  /* IRQ66 */
    (uint32_t)Default_Handler,  /* IRQ67 */
    (uint32_t)Default_Handler,  /* IRQ68 */
    (uint32_t)Default_Handler,  /* IRQ69 */
    (uint32_t)Default_Handler,  /* IRQ70 */
    (uint32_t)Default_Handler,  /* IRQ71 */
    (uint32_t)Default_Handler,  /* IRQ72 */
    (uint32_t)Default_Handler,  /* IRQ73 */
};

__attribute__((noreturn))
void Reset_Handler(void)
{
    __pic32c_data_initialization();   /* XC32: inicializa .data/.bss/.ramfunc via .dinit */

    (void)main();
    while (1) {}
}

void Default_Handler(void)
{
    while (1)
    {
    }
}