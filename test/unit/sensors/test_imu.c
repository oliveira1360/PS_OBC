/**
 * @file test_imu.c
 * @brief Testes unitarios para o modulo IMU (ECSS 5.5.3.2)
 */
#include "test_utils.h"
#include "peripherals/imu.h"
#include "app/sensors.h"
#include "hal/hal_i2c.h"
#include "config/board.h"

static void test_imu_read_complete(void)
{
    TEST("IMU full read cycle - 9 axes populated");
    imu_read_async();
    for (int i = 0; i < 50; i++) imu_tick();

    ASSERT_FLOAT_RANGE(imu.az, 0.90f, 1.10f, "az near 1g");
    ASSERT_FLOAT_RANGE(imu.ax, 0.0f, 0.10f, "ax small");
    ASSERT_FLOAT_RANGE(imu.ay, 0.0f, 0.10f, "ay small");
    ASSERT_FLOAT_RANGE(imu.gx, 0.0f, 0.20f, "gx small");
    ASSERT_FLOAT_RANGE(imu.gy, 0.0f, 0.20f, "gy small");
    ASSERT_FLOAT_RANGE(imu.gz, 0.0f, 0.20f, "gz small");
    ASSERT_FLOAT_RANGE(imu.mx, 0.10f, 0.30f, "mx earth field");
    ASSERT_FLOAT_RANGE(imu.my, 0.05f, 0.20f, "my earth field");
    ASSERT_FLOAT_RANGE(imu.mz, 0.30f, 0.55f, "mz earth field");
}

static void test_imu_busy_rejection(void)
{
    TEST("IMU busy rejection");
    imu_read_async();
    imu_read_async();
    for (int i = 0; i < 50; i++) imu_tick();
    ASSERT_FLOAT_RANGE(imu.az, 0.90f, 1.10f, "State not corrupted");
}

void run_imu_tests(void)
{
    TEST_SUITE("IMU Sensor Tests (ECSS 5.5.3.2)");
    test_imu_read_complete();
    test_imu_busy_rejection();
}
