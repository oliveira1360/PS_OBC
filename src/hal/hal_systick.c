#include "hal/hal_systick.h"

#define SYST_CSR   (*(volatile uint32_t *)0xE000E010UL)
#define SYST_RVR   (*(volatile uint32_t *)0xE000E014UL)
#define SYST_CVR   (*(volatile uint32_t *)0xE000E018UL)

#define SYST_CSR_ENABLE     (1UL << 0)
#define SYST_CSR_TICKINT    (1UL << 1)
#define SYST_CSR_CLKSOURCE  (1UL << 2)

#define MCK_HZ  6250000UL

static volatile uint32_t ms_counter = 0U;

void SysTick_Handler(void)
{
    ms_counter++;
}

void hal_systick_init(void)
{
    uint32_t ticks = (MCK_HZ / 1000U) - 1U;  // 1 interrupção por ms
    SYST_RVR = ticks;
    SYST_CVR = 0U;
    SYST_CSR = SYST_CSR_ENABLE | SYST_CSR_TICKINT | SYST_CSR_CLKSOURCE;
}

uint32_t hal_systick_get_ms(void)
{
    return *(volatile uint32_t *)&ms_counter;
}