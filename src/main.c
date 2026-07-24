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
    hal_clock_init_300mhz();  
    debug_uart_init();

    printf("\n === OBC BOOT === FW TesteAONovo\n");
    printf("beta 1.100\n");


    system_lifecycle();

    return 0;
}
