#include <stdint.h>
#include "app/init.h"
#include "hal/hal_gpio.h"
#include "hal/hal_i2c.h"
#include "hal/hal_peripherals_init.h"
#include "hal/hal_qspi.h"
#include "hal/hal_spi.h"
#include "hal/hal_usart.h"
#include "hal/hal_system.h"
<<<<<<< HEAD

static uint8_t self_test(void);


=======
#include "hal/hal_systick.h"
#include <stdio.h>

static uint8_t self_test(void);

>>>>>>> origin/OBC_board
init_status_t init_status = {0};

static int init_all(void)
{
<<<<<<< HEAD
    if (!init_status.gpio)
        init_status.gpio = hal_gpio_init(); 

    if (!init_status.i2c)
        init_status.i2c = hal_i2c_init();

    if (!init_status.spi)
        init_status.spi = hal_spi_init();
=======
    hal_systick_init();

    if (!init_status.gpio)
        init_status.gpio = hal_gpio_init();

    if (!init_status.i2c)
    {
        init_status.i2c = hal_i2c_init();
        hal_i2c_bus_recovery();
    }

    
    if (!init_status.spi)
        init_status.spi = hal_spi_init();
        
>>>>>>> origin/OBC_board

    if (!init_status.qspi)
        init_status.qspi = hal_qspi_init();

<<<<<<< HEAD
    if (!init_status.usart)
        init_status.usart = hal_usart_init();
=======
    printf("init_status.usart before init code: %d", init_status.usart);

    if (!init_status.usart)
    {
        init_status.usart = hal_usart_init();
        printf("init_status.usart code after init:  %d", init_status.usart);

    }
>>>>>>> origin/OBC_board

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

int system_init(void)
{
    uint8_t attempts = 0;

    while (!init_all())
    {
        attempts++;
        if (attempts >= MAX_INIT_RETRIES)
<<<<<<< HEAD
            return 0;   /* falha crítica */
    }

    return self_test();  /* 1=pass, 0=fail → reinicia */
}

static uint8_t self_test(void){
     return hal_self_test_gpio()   &&
           hal_self_test_i2c()    &&
           hal_self_test_spi()    &&
           hal_self_test_qspi()   &&
           hal_self_test_usart()  &&
=======
            return 0; /* falha crítica */
    }

    return self_test(); /* 1=pass, 0=fail → reinicia */
}

static uint8_t self_test(void)
{
    return hal_self_test_gpio() &&
           hal_self_test_i2c() &&
           hal_self_test_spi() &&
           hal_self_test_qspi() &&
           hal_self_test_usart() &&
>>>>>>> origin/OBC_board
           hal_self_test_memory();
}