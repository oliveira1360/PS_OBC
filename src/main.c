#include <stdio.h>
#include "app/sensors.h"
#include "app/init.h"
#include "peripherals/ttc.h"
#include "hal/hal_system.h"
#include "hal/hal_debug_uart.h"
#include "testsForBoard/deterministic_test.h"
#include "testsForBoard/qspi_test.h"
#include "testsForBoard/unit_tests.h"
#include "testsForBoard/wcet_test.h"
#include "testsForBoard/seu_injection_test.h"


int main(void)
{
    debug_uart_init();

    printf("\n === OBC BOOT === FW TesteAONovo\n");
    printf("beta 1.100");
    
    for (size_t i = 0; i < 10; i++)
    {
        printf("\n");
    }
    if (!system_init())
    {
        hal_system_reset();
    }

    // run_unit_tests();
    test_qspi_rw();
    test_wcet();
    test_seu_injection();

    ttc_read_async();

    while (1)
    {
        mission_lifecycle();
    }

    
    return 0;
}
