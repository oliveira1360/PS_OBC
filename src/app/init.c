#include <stdint.h>
#include "app/init.h"
#include "hal/hal_gpio.h"
#include "hal/hal_i2c.h"
#include "hal/hal_peripherals_init.h"
#include "hal/hal_qspi.h"
#include "hal/hal_spi.h"
#include "hal/hal_usart.h"
#include "hal/hal_system.h"
#include "hal/hal_systick.h"
#include "peripherals/ttc.h"

static uint8_t self_test(void);
void recovery();
init_status_t init_status = {0};

static int init_all(void)
{
    hal_systick_init();

    if (!init_status.gpio)
        init_status.gpio = hal_gpio_init();

    if (!init_status.i2c)
        init_status.i2c = hal_i2c_init();

    if (!init_status.spi)
        init_status.spi = hal_spi_init();

    if (!init_status.qspi)
        init_status.qspi = hal_qspi_init();

    if (!init_status.usart)
        init_status.usart = hal_usart_init();

    if (!init_status.gnss)
        init_status.gnss = hal_gnss_init();

    if (!init_status.imu)
        init_status.imu = hal_imu_init();

    if (!init_status.ttc)
        init_status.ttc = hal_ttc_init();

    if (!init_status.pressure)
        init_status.pressure = hal_pressure_init();

    if (!init_status.temperature)
        init_status.temperature = hal_temperature_init();

    if (!init_status.ext_memory)
        init_status.ext_memory = hal_ext_memory_init();

    return init_status.gpio && init_status.i2c && init_status.spi &&
           init_status.qspi && init_status.usart && init_status.gnss &&
           init_status.imu && init_status.ttc && init_status.pressure &&
           init_status.temperature && init_status.ext_memory;
}

void recovery()
{
    hal_i2c_bus_recovery();
}

int system_init(void)
{
    uint8_t attempts = 0;

    while (!init_all())
    {
        recovery();
        attempts++;
        if (attempts >= MAX_INIT_RETRIES)
            return 0;
    }

    while (self_test())
    {
        attempts++;
        if (attempts >= MAX_INIT_RETRIES)
            return 0;
    }

    return 1;
}

static uint8_t self_test(void)
{
    return hal_self_test_gpio() &&
           hal_self_test_i2c() &&
           hal_self_test_spi() &&
           hal_self_test_qspi() &&
           hal_self_test_usart() &&
           hal_self_test_memory();
}