#include <stdio.h>
#include "states.h"
#include "modes.h"


States nominalMode(void) {
    return getMode(nominal_mode); 
}

States communicationMode(void) {
    return getMode(communication_mode); 
}

States otaMode(void) {
    return getMode(ota_mode); 
}

States safeMode(void) {
    return getMode(safe_mode);
}

States ultraLowPowerMode(void) {
    return getMode(ultra_low_power_mode);
}

States decommissioningMode(void) {
    return getMode(decommissioning_mode);
}

int isSystemSafe(void) {
    if (BATTERY_STATUS < 30.0f) return 0;
    if (TEMP_INTERNAL > 60.0f) return 0;
    if (TEMP_INTERNAL < -10.0f) return 0;
    if (EPS_BUS_VOLTAGE < 6.5f) return 0;
    
    return 1;
}

States getMode(States currentState) {
    int safe = isSystemSafe();
    int battery_critical = (BATTERY_STATUS < 10.0f);

    switch (currentState) {
        case nominal_mode:
            if (!safe) return safe_mode;
            if (COMM_WINDOW_OPEN) return communication_mode;
            return nominal_mode;

        case communication_mode:
            if (!safe) return safe_mode;
            if (OTA_REQUESTED) return ota_mode;
            if (!COMM_WINDOW_OPEN) return nominal_mode;
            return communication_mode;

        case ota_mode:
            if (!safe) return safe_mode;
            if (!OTA_REQUESTED) return communication_mode;
            return ota_mode;

        case safe_mode:
            if (battery_critical) return ultra_low_power_mode;
            if (safe) return nominal_mode;
            return safe_mode;

        case ultra_low_power_mode:
            if (MISSION_TIMEOUT) return decommissioning_mode;
            if (!battery_critical) return safe_mode;
            return ultra_low_power_mode;

        case decommissioning_mode:
            return decommissioning_mode;

        default:
            return safe_mode;
    }
}