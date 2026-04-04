#include <stdio.h>
#include "modes.h"
#include "states.h"
#include "app/sensors.h"
#include "app/mission.h"
#include "app/init.h"
#include "peripherals/gnss.h"
#include "peripherals/imu.h"
#include "peripherals/eps.h"
#include "peripherals/pressure.h"
#include "peripherals/temperature.h"
#include "hal/hal_i2c.h"

#include <time.h>

int main(void)
{

    if (!system_init())
        hal_system_reset();

    States state = nominal_mode;
    clock_t last_time = clock();
    sensors_read_all();

    while (1)
    {
        sensors_tick();


        switch (state)
        {
        case nominal_mode:
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
            state = decommissioningMode();
            break;
        }

        clock_t now = clock();
        if (((now - last_time) * 1000) / CLOCKS_PER_SEC > TIME_TO_UPDATE_VALUES)
        {
            sensors_print();
            sensors_read_all();
            last_time = clock();

        }
    }

    return 0;
}