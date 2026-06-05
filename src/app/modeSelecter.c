#include "app/modes.h"
#include "app/mission.h"
#include "app/sensors.h"
#include "hal/hal_qspi.h"



States state = nominal_mode;


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
        if (!COMM_WINDOW_OPEN)
            return nominal_mode;
        if (OTA_REQUESTED && BATTERY_STATUS > 70.0f)
            return ota_mode;

        return communication_mode;

    case ota_mode:
        if (!OTA_REQUESTED)
            return communication_mode;
        return ota_mode;

    case safe_mode:
        if (battery_critical)
            return ultra_low_power_mode;
        if (OTA_REQUESTED && BATTERY_STATUS > 70.0f)
            return ota_mode;
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

void mission_lifecycle()
{
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
}