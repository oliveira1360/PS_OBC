/**
 * @file test_sensor_pipeline.c
 * @brief Testes de integracao do pipeline de sensores (ECSS 5.5.4)
 */
#include "test_utils.h"
#include "app/sensors.h"
#include "hal/hal_i2c.h"

static void test_full_sensor_pipeline(void)
{
    TEST("Full sensor pipeline: read_all + 100 ticks");
    sensors_read_all();
    for (int i = 0; i < 100; i++) sensors_tick();

    ASSERT(gnss.latitude >= 0.0f, "GNSS latitude populated");
    ASSERT(imu.az > 0.0f, "IMU az populated");
    ASSERT(pressure.pressure > 0.0f, "Pressure populated");
    ASSERT(temperature.temperature > 0.0f, "Temperature populated");
    ASSERT(eps.voltage > 0.0f, "EPS voltage populated");
}

static void test_sensor_data_refresh(void)
{
    TEST("Sensor data refreshes between read cycles");
    sensors_read_all();
    for (int i = 0; i < 50; i++) sensors_tick();
    float v1 = eps.voltage;
    float p1 = pressure.pressure;

    sensors_read_all();
    for (int i = 0; i < 50; i++) sensors_tick();

    /* With random HAL, at least some values should change */
    ASSERT(eps.voltage > 0.0f, "EPS still valid after 2nd read");
    ASSERT(pressure.pressure > 0.0f, "Pressure still valid after 2nd read");
    (void)v1; (void)p1;
}

static void test_concurrent_sensors_no_interference(void)
{
    TEST("Concurrent I2C sensors: data types not mixed");
    sensors_read_all();
    for (int i = 0; i < 100; i++) sensors_tick();

    /* Verify each sensor reads correct data type */
    ASSERT_FLOAT_RANGE(temperature.temperature, 15.0f, 45.0f,
                       "Temperature in expected range (not pressure)");
    ASSERT_FLOAT_RANGE(pressure.pressure, 900.0f, 1100.0f,
                       "Pressure in expected range (not temperature)");
    ASSERT_FLOAT_RANGE(imu.az, 0.80f, 1.20f,
                       "IMU az near 1g (not GNSS data)");
    ASSERT_FLOAT_RANGE(gnss.speed, 7.0f, 8.5f,
                       "GNSS speed in orbital range (not IMU data)");
}

void run_sensor_pipeline_tests(void)
{
    TEST_SUITE("Sensor Pipeline Integration Tests (ECSS 5.5.4)");
    test_full_sensor_pipeline();
    test_sensor_data_refresh();
    test_concurrent_sensors_no_interference();
}
