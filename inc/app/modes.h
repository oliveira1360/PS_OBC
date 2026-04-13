#ifndef MODE_H
#define MODE_H

typedef enum
{
    nominal_mode,
    communication_mode,
    ota_mode,
    safe_mode,
    ultra_low_power_mode,
    decommissioning_mode,
} States;

States nominalMode(void);
States communicationMode(void);
States otaMode(void);
States safeMode(void);
States ultraLowPowerMode(void);
States decommissioningMode(void);

States getMode(States currentState);
#endif