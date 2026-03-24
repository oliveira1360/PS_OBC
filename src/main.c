#include <stdio.h>
#include "modes.h"
#include "states.h"
#include <time.h>

int main(void)
{
    States state = nominal_mode;
    clock_t tempo_inicial = clock();

    while (1)
    {
        clock_t tempo_atual = clock();
        unsigned long tempo_passado_ms = ((tempo_atual - tempo_inicial) * 1000) / CLOCKS_PER_SEC;
        if (tempo_passado_ms > 4000)
        {
            stateCheck();
            tempo_inicial = clock();
        }
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

    return 0;
}