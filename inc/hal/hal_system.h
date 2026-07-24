#ifndef HAL_SYSTEM_H
#define HAL_SYSTEM_H

void hal_clock_init_300mhz(void);
void hal_system_reset(void);
void hal_watchdog_kick(void);  

int hal_self_test_i2c(void);
int hal_self_test_spi(void);
int hal_self_test_qspi(void);
int hal_self_test_usart(void);
int hal_self_test_gpio(void);
int hal_self_test_memory(void);

#endif