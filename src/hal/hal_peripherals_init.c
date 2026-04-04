#include "hal/hal_peripherals_init.h"
#include "hal/hal_i2c.h"
#include "drivers/i2c_driver.h"
#include "config/board.h"
#include <stdint.h>

uint8_t hal_gnss_init(void)
{
    return 1;
}

uint8_t hal_imu_init(void)
{
    return 1;
}

uint8_t hal_pressure_init(void)
{
    return 1;
}

uint8_t hal_temperature_init(void)
{
    return 1;
}

uint8_t hal_eps_init(void)
{
    return 1;
}

uint8_t hal_ttc_init(void)
{
    return 1;
}

uint8_t hal_ext_memory_init(void){
    return 1;
}