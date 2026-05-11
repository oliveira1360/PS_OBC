#include <stdio.h>
<<<<<<< HEAD
#include "app/modes.h"
#include "app/states.h"
#include "app/sensors.h"
#include "app/mission.h"
=======
#include "app/states.h"
#include "app/modes.h"
#include "app/mission.h"
#include "app/sensors.h"
>>>>>>> origin/OBC_board
#include "app/init.h"
#include "peripherals/gnss.h"
#include "peripherals/imu.h"
#include "peripherals/eps.h"
#include "peripherals/pressure.h"
#include "peripherals/temperature.h"
#include "peripherals/ttc.h"
#include "hal/hal_i2c.h"
#include "hal/hal_system.h"
<<<<<<< HEAD

int main(void)
{

    if (!system_init())
        hal_system_reset();

=======
#include "hal/hal_debug_uart.h"
#include "hal/hal_systick.h"
#include "hal/hal_usart.h"
#include "hal/hal_spi.h"
#include "config/board.h"

int main(void)
{
    debug_uart_init();

    if (!system_init())
    {
        hal_system_reset();
    }
>>>>>>> origin/OBC_board
    States state = nominal_mode;

    sensors_read_all();
    ttc_read_async();
<<<<<<< HEAD
=======
    for (size_t i = 0; i < 10; i++)
    {
        printf("\n");
    }
    
>>>>>>> origin/OBC_board

    while (1)
    {
        sensors_tick();

        switch (state)
        {
        case nominal_mode:
<<<<<<< HEAD
            //printf("nominal_mode");
            state = nominalMode();
            break;
        case communication_mode:
            printf("communication_mode");
            state = communicationMode();
            break;
        case ota_mode:
            printf("ota_mode");
            state = otaMode();
            break;
        case safe_mode:
            printf("safe_mode");
            state = safeMode();
            break;
        case ultra_low_power_mode:
            printf("ultra_low_power_mode");
            state = ultraLowPowerMode();
            break;
        case decommissioning_mode:
            printf("decommissioning_mode");
=======
            state = nominalMode();
            break;
        case communication_mode:
            state = communicationMode();
            break;
        case ota_mode:
            state = otaMode();
            break;
        case safe_mode:
            state = safeMode();
            break;
        case ultra_low_power_mode:
            state = ultraLowPowerMode();
            break;
        case decommissioning_mode:
>>>>>>> origin/OBC_board
            state = decommissioningMode();
            break;
        }
    }

    return 0;
}