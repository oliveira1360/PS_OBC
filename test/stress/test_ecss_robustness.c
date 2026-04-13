/**
 * @file test_ecss_robustness.c
 * @brief Testes de robustez e stress (ECSS 5.5.3.2c, 5.6.3.1)
 *
 * - Stress: leituras repetidas
 * - Boundary: limites de seguranca
 * - Error injection: enderecos I2C invalidos
 * - Depletion: esgotamento de bateria
 */
#include "test_utils.h"
#include "app/sensors.h"
#include "app/mission.h"
#include "app/modes.h"
#include "app/states.h"
#include "drivers/i2c_driver.h"
#include "hal/hal_i2c.h"
#include "config/board.h"
#include <string.h>

extern int isSystemSafe(void);

static void test_stress_repeated_reads(void)
{
    TEST("Stress: 100 consecutive read cycles");
    for (int i = 0; i < 100; i++)
    {
        sensors_read_all();
        for (int j = 0; j < 20; j++) sensors_tick();
    }
    ASSERT(eps.voltage > 0.0f, "EPS valid after 100 cycles");
    ASSERT(gnss.altitude > 0.0f, "GNSS valid after 100 cycles");
    ASSERT(pressure.pressure > 0.0f, "Pressure valid after 100 cycles");
}

static void test_boundary_all_safe_limits(void)
{
    TEST("Boundary: exact safety limits (ECSS n-1, n, n+1)");
    eps.current = 1.5f;
    temperature.temperature = 25.0f;
    BATTERY_STATUS = 80.0f;

    /* Voltage boundaries */
    eps.voltage = 3.1f;
    ASSERT_EQ(isSystemSafe(), 1, "Safe at LOWEST_SAFE_VOLTAGE (3.1)");

    eps.voltage = 6.5f;
    ASSERT_EQ(isSystemSafe(), 1, "Safe at MAX_SAFE_VOLTAGE (6.5)");

    eps.voltage = 3.0f;
    ASSERT_EQ(isSystemSafe(), 0, "Not safe below LOWEST_SAFE_VOLTAGE");

    eps.voltage = 6.6f;
    ASSERT_EQ(isSystemSafe(), 0, "Not safe above MAX_SAFE_VOLTAGE");

    /* Current boundaries */
    eps.voltage = 5.0f;
    eps.current = 1.0f;
    ASSERT_EQ(isSystemSafe(), 1, "Safe at LOWEST_SAFE_CURRENT (1)");

    eps.current = 2.0f;
    ASSERT_EQ(isSystemSafe(), 1, "Safe at MAX_SAFE_CURRENT (2)");

    eps.current = 0.9f;
    ASSERT_EQ(isSystemSafe(), 0, "Not safe below LOWEST_SAFE_CURRENT");

    eps.current = 2.1f;
    ASSERT_EQ(isSystemSafe(), 0, "Not safe above MAX_SAFE_CURRENT");

    /* Temperature boundaries */
    eps.current = 1.5f;
    temperature.temperature = -10.0f;
    ASSERT_EQ(isSystemSafe(), 1, "Safe at LOWEST_SAFE_TEMP (-10)");

    temperature.temperature = 50.0f;
    ASSERT_EQ(isSystemSafe(), 1, "Safe at MAX_SAFE_TEMP (50)");

    temperature.temperature = -11.0f;
    ASSERT_EQ(isSystemSafe(), 0, "Not safe below LOWEST_SAFE_TEMP");

    temperature.temperature = 51.0f;
    ASSERT_EQ(isSystemSafe(), 0, "Not safe above MAX_SAFE_TEMP");
}

static void test_i2c_invalid_addresses(void)
{
    TEST("Error injection: I2C invalid addresses (NACK)");

    /* Drena transações de sensores pendentes do teste anterior para
     * garantir que o barramento I2C está livre (bus_free = 1).
     * Máximo estimado: 5 sensores * ~39 ticks cada = ~200 ticks. */
    for (int drain = 0; drain < 300; drain++)
        sensors_tick();

    uint8_t addrs[] = {0x00, 0x01, 0xFF};

    for (int a = 0; a < 3; a++)
    {
        i2c_handle_t h;
        uint8_t buf[4];
        int cb_result = -99;

        memset(&h, 0, sizeof(h));
        h.state = I2C_STARTING;
        h.addr = addrs[a];
        h.buf = buf;
        h.len = 4;
        h.rw = 1;
        h.callback = NULL; /* no callback - just check state */

        for (int t = 0; t < 20; t++) i2c_tick(&h);

        ASSERT_EQ(h.state, I2C_IDLE, "Returns to IDLE on NACK");
        (void)cb_result;
    }
}

static void test_stress_battery_depletion(void)
{
    TEST("Stress: battery depletion via stateCheck()");
    BATTERY_STATUS = 100.0f;

    stateCheck(); /* -20 -> 80 */
    ASSERT_FLOAT_RANGE(BATTERY_STATUS, 78.0f, 82.0f, "~80 after 1st stateCheck");

    stateCheck(); /* -20 -> 60 */
    ASSERT_FLOAT_RANGE(BATTERY_STATUS, 58.0f, 62.0f, "~60 after 2nd stateCheck");

    stateCheck(); /* -20 -> 40 */
    ASSERT_FLOAT_RANGE(BATTERY_STATUS, 38.0f, 42.0f, "~40 after 3rd stateCheck");

    stateCheck(); /* -20 -> 20 */
    ASSERT_FLOAT_RANGE(BATTERY_STATUS, 18.0f, 22.0f, "~20 after 4th stateCheck");

    stateCheck(); /* -20 -> 0 */
    ASSERT_FLOAT_RANGE(BATTERY_STATUS, -2.0f, 2.0f, "~0 after 5th stateCheck");
}

static void test_stress_rapid_sensor_ticks(void)
{
    TEST("Stress: 1000 rapid sensor ticks");
    sensors_read_all();
    for (int i = 0; i < 1000; i++) sensors_tick();

    ASSERT(eps.voltage > 0.0f, "EPS valid after 1000 ticks");
    ASSERT(imu.az > 0.0f, "IMU valid after 1000 ticks");
}

void run_ecss_robustness_tests(void)
{
    TEST_SUITE("ECSS Robustness/Stress Tests (5.5.3.2c, 5.6.3.1)");
    test_stress_repeated_reads();
    test_boundary_all_safe_limits();
    test_i2c_invalid_addresses();
    test_stress_battery_depletion();
    test_stress_rapid_sensor_ticks();
}