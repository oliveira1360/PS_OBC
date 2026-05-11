#include <stdio.h>
#include "app/states.h"
#include "app/modes.h"
#include "app/mission.h"
#include "app/sensors.h"
<<<<<<< HEAD

=======
#include "hal/hal_systick.h"

/*
>>>>>>> origin/OBC_board
#include <time.h> // remover no futuro, uso para o clock(tempo)

States nominalMode(void)
{
    static clock_t last_time = 0; 
    
    if (last_time == 0) {
        last_time = clock();
    }

    clock_t now = clock();

    if (((now - last_time) * 1000) / CLOCKS_PER_SEC > TIME_TO_UPDATE_VALUES)
    {
        sensors_print();
        sensors_read_all();
        last_time = clock(); 
    }

    return getMode(nominal_mode);
<<<<<<< HEAD
=======
}*/

States nominalMode(void)
{
    static uint32_t last_ms = 0U;

    uint32_t now = hal_systick_get_ms();

    if ((now - last_ms) >= TIME_TO_UPDATE_VALUES)
    {
        //sensors_print();
        sensors_read_all();
        last_ms = now;
    }

    return getMode(nominal_mode);
>>>>>>> origin/OBC_board
}

States communicationMode(void)
{
    return getMode(communication_mode);
}

States otaMode(void)
{
    return getMode(ota_mode);
}

States safeMode(void)
{
<<<<<<< HEAD
    static clock_t last_time = 0; 
    
    if (last_time == 0) {
        last_time = clock();
    }

    clock_t now = clock();

    if (((now - last_time) * 1000) / CLOCKS_PER_SEC > TIME_TO_UPDATE_VALUES)
    {
        sensors_print();
        sensors_read_all();
        last_time = clock(); 
    }
    
=======
    static uint32_t last_ms = 0U;

    uint32_t now = hal_systick_get_ms();

    if ((now - last_ms) >= TIME_TO_UPDATE_VALUES)
    {
        //sensors_print();
        sensors_read_all();
        last_ms = now;
    }

>>>>>>> origin/OBC_board
    return getMode(safe_mode);
}

States ultraLowPowerMode(void)
{
    return getMode(ultra_low_power_mode);
}

States decommissioningMode(void)
{
    return getMode(decommissioning_mode);
}

/**
 * @brief verify the system health
 *
 * @return 0 if the systema is not in safe mode, returns 1 if everything is right in the system
 */

int isSystemSafe(void)
{
    if (eps.voltage > MAX_SAFE_VOLTAGE || eps.voltage < LOWEST_SAFE_VOLTAGE)
        return 0;
    if (eps.current > MAX_SAFE_CURRENT || eps.current < LOWEST_SAFE_CURRENT)
        return 0;
    if (temperature.temperature > MAX_SAFE_TEMP || temperature.temperature < LOWEST_SAFE_TEMP)
        return 0;
    if (BATTERY_STATUS < LOWEST_SAFE_BATTERY)
        return 0;
    

    return 1;
}

States getMode(States currentState)
{
    int safe = isSystemSafe();
    int battery_critical = (BATTERY_STATUS < BATTERY_IN_CRITICAL_LEVEL);

    switch (currentState)
    {
    case nominal_mode:
        if (!safe)
            return safe_mode;
        if (COMM_WINDOW_OPEN)
            return communication_mode;
        return nominal_mode;

    case communication_mode:
        if (!safe)
            return safe_mode;
        if (OTA_REQUESTED)
            return ota_mode;
        if (!COMM_WINDOW_OPEN)
            return nominal_mode;
        return communication_mode;

    case ota_mode:
        if (!safe)
            return safe_mode;
        if (!OTA_REQUESTED)
            return communication_mode;
        return ota_mode;

    case safe_mode:
        if (battery_critical)
            return ultra_low_power_mode;
        if (safe)
            return nominal_mode;
        return safe_mode;

    case ultra_low_power_mode:
        if (!battery_critical)
            return safe_mode;
        if (MISSION_TIMEOUT)
            return decommissioning_mode;
        return ultra_low_power_mode;

    case decommissioning_mode:
        return decommissioning_mode;

    default:
        return nominal_mode;
    }
}
