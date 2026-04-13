/**
 * @file test_temperature.c
 * @brief Testes unitarios para o sensor de temperatura (ECSS 5.5.3.2)
 */
#include "test_utils.h"
#include "peripherals/temperature.h"
#include "app/sensors.h"
#include "hal/hal_i2c.h"
#include "config/board.h"

static void test_temperature_read_complete(void)
{
    TEST("Temperature full read cycle");
    temperature_read_async();
    for (int i = 0; i < 50; i++) temperature_tick();
    ASSERT_FLOAT_RANGE(temperature.temperature, 20.0f, 40.0f, "Temperature in [20, 40] C");
}

static void test_temperature_multiple_reads(void)
{
    TEST("Temperature data consistency across reads");
    temperature_read_async();
    for (int i = 0; i < 50; i++) temperature_tick();

    temperature_read_async();
    for (int i = 0; i < 50; i++) temperature_tick();
    ASSERT_FLOAT_RANGE(temperature.temperature, 20.0f, 40.0f, "Still valid after 2nd read");
}

void run_temperature_tests(void)
{
    TEST_SUITE("Temperature Sensor Tests (ECSS 5.5.3.2)");
    test_temperature_read_complete();
    test_temperature_multiple_reads();
}
