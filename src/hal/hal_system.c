#include "hal/hal_system.h"

void hal_system_reset(void)
{
}

void hal_watchdog_kick(void)
{
}

int hal_self_test_i2c(void)
{
    return 1;
}

int hal_self_test_spi(void)
{
    return 1;
}
int hal_self_test_qspi(void)
{
    return 1;
}
int hal_self_test_usart(void)
{
    return 1;
}
int hal_self_test_gpio(void)
{
    return 1;
}
int hal_self_test_memory(void)
{
    return 1;
}