#include <stdio.h>
#include "app/states.h"
#include "app/modes.h"
#include "app/mission.h"
#include "app/sensors.h"
#include "app/init.h"
#include "peripherals/gnss.h"
#include "peripherals/imu.h"
#include "peripherals/eps.h"
#include "peripherals/pressure.h"
#include "peripherals/temperature.h"
#include "peripherals/ttc.h"
#include "hal/hal_i2c.h"
#include "hal/hal_system.h"
#include "hal/hal_debug_uart.h"
#include "hal/hal_systick.h"
#include "hal/hal_usart.h"
#include "config/board.h"

int main(void)
{
    debug_uart_init();

    if (!system_init())
    {
        hal_system_reset();
    }
    printf("USART0_CSR:  0x%08X\n", (unsigned int)(*(volatile uint32_t *)(0x40024000 + 0x14)));
    printf("USART0_MR:   0x%08X\n", (unsigned int)(*(volatile uint32_t *)(0x40024000 + 0x04)));
    printf("USART0_BRGR: 0x%08X\n", (unsigned int)(*(volatile uint32_t *)(0x40024000 + 0x20)));
    States state = nominal_mode;

    sensors_read_all();
    printf("sensors_read_all OK\n");
    ttc_read_async();

    /*
    // Depois do system_init, antes do while(1)
    printf("Loopback test...\n");
    while (!(USART0_CSR & US_CSR_TXRDY))
    {
    }
    USART0_THR = 0xAB;

    uint32_t timeout = 0;
    while (!(USART0_CSR & US_CSR_RXRDY))
    {
        if (++timeout > 5000000UL)
        {
            printf("loopback timeout\n");
            break;
        }
    }
    if (USART0_CSR & US_CSR_RXRDY)
        printf("loopback: 0x%02X\n", (uint8_t)(USART0_RHR & 0xFF));
        */

    uint16_t prop_divider = 0U;
    while (1)
    {
        sensors_tick();
        //propulsor_tick(); /* tick é leve — só avança a FSM 1 estado */

        /* Lê propulsor só a cada ~100 ciclos para não sobrecarregar */
        /*
        prop_divider++;
        if (prop_divider >= 100U)
        {
            prop_divider = 0U;
            propulsor_read_async();
        }
        */

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