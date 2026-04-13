/**
 * @file test_pressure.c
 * @brief Testes unitarios para o sensor de pressao (ECSS 5.5.3.2)
 */
#include "test_utils.h"
#include "peripherals/pressure.h"
#include "app/sensors.h"
#include "hal/hal_i2c.h"
#include "config/board.h"

static void test_pressure_read_complete(void)
{
    TEST("Pressure full read cycle");
    pressure_read_async();
    for (int i = 0; i < 50; i++) pressure_tick();
    ASSERT_FLOAT_RANGE(pressure.pressure, 1005.0f, 1020.0f, "Pressure in [1005, 1020] hPa");
}

static void test_pressure_multiple_reads(void)
{
    TEST("Pressure data consistency across reads");
    pressure_read_async();
    for (int i = 0; i < 50; i++) pressure_tick();

    pressure_read_async();
    for (int i = 0; i < 50; i++) pressure_tick();
    ASSERT_FLOAT_RANGE(pressure.pressure, 1005.0f, 1020.0f, "Still valid after 2nd read");
}

void run_pressure_tests(void)
{
    TEST_SUITE("Pressure Sensor Tests (ECSS 5.5.3.2)");
    test_pressure_read_complete();
    test_pressure_multiple_reads();
}
