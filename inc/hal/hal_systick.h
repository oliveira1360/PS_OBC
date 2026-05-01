#ifndef HAL_SYSTICK_H
#define HAL_SYSTICK_H

#include <stdint.h>

void     hal_systick_init(void);
uint32_t __attribute__((noinline)) hal_systick_get_ms(void);
#endif