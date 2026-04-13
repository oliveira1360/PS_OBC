/**
 * @file test_eps.c
 * @brief Testes unitarios para o EPS (ECSS 5.5.3.2)
 */
#include "test_utils.h"
#include "peripherals/eps.h"
#include "app/sensors.h"
#include "hal/hal_i2c.h"
#include "config/board.h"

static void test_eps_read_complete(void)
{
    TEST("EPS full read cycle");
    eps_read_async();
    for (int i = 0; i < 50; i++) eps_tick();
    ASSERT(eps.voltage > 0.0f, "Voltage populated");
    ASSERT(eps.current > 0.0f, "Current populated");
}

static void test_eps_voltage_range(void)
{
    TEST("EPS voltage range");
    eps_read_async();
    for (int i = 0; i < 50; i++) eps_tick();
    ASSERT_FLOAT_RANGE(eps.voltage, 3.0f, 9.0f, "Voltage in [3.0, 9.0] V");
}

static void test_eps_current_range(void)
{
    TEST("EPS current range");
    eps_read_async();
    for (int i = 0; i < 50; i++) eps_tick();
    ASSERT_FLOAT_RANGE(eps.current, 0.70f, 1.70f, "Current in [0.70, 1.70] A");
}

void run_eps_tests(void)
{
    TEST_SUITE("EPS Sensor Tests (ECSS 5.5.3.2)");
    test_eps_read_complete();
    test_eps_voltage_range();
    test_eps_current_range();
}
