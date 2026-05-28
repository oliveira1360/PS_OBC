#include <stdio.h>
#include "app/modes.h"
#include "app/sensors.h"
#include "app/init.h"
#include "peripherals/ttc.h"
#include "hal/hal_system.h"
#include "hal/hal_debug_uart.h"
#include "testsForBoard/deterministic_test.h"
// #include "testsForBoard/qspi_test.h"
#include "testsForBoard/unit_tests.h"

int main(void)
{
    debug_uart_init();

printf("=== OBC BOOT ===\r\n");   // \r\n garante flush imediato
    if (!system_init())
    {
        hal_system_reset();
    }

    run_unit_tests();
    //test_qspi_rw();

    ttc_read_async();
    for (size_t i = 0; i < 10; i++)
    {
        printf("\n");
    }
    printf("olaa");

    while (1)
    {
        sensors_tick();
        mission_lifecycle();
    }

    return 0;
}
