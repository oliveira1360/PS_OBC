/**
 * @file test_gnss.c
 * @brief Testes unitarios para o modulo GNSS (ECSS 5.5.3.2)
 */
#include "test_utils.h"
#include "peripherals/gnss.h"
#include "app/sensors.h"
#include "hal/hal_i2c.h"
#include "config/board.h"

static void test_gnss_data_valid_ranges(void)
{
    TEST("GNSS data ranges after full read cycle");
    gnss_read_async();
    for (int i = 0; i < 50; i++) gnss_tick();

    ASSERT_FLOAT_RANGE(gnss.latitude, 0.0f, 52.0f, "Latitude in [0, 52]");
    ASSERT_FLOAT_RANGE(gnss.longitude, 0.0f, 180.0f, "Longitude in [0, 180]");
    ASSERT_FLOAT_RANGE(gnss.altitude, 510.0f, 530.0f, "Altitude in [510, 530] km");
    ASSERT_FLOAT_RANGE(gnss.speed, 7.5f, 8.0f, "Speed in [7.5, 8.0] km/s");
}

static void test_gnss_multiple_reads(void)
{
    TEST("GNSS data changes between reads");
    gnss_read_async();
    for (int i = 0; i < 50; i++) gnss_tick();
    float lat1 = gnss.latitude;
    float lon1 = gnss.longitude;

    gnss_read_async();
    for (int i = 0; i < 50; i++) gnss_tick();

    int changed = (gnss.latitude != lat1) || (gnss.longitude != lon1);
    ASSERT(changed, "GNSS data changed after second read");
}

static void test_gnss_busy_rejection(void)
{
    TEST("GNSS busy rejection");
    gnss_read_async();
    gnss_read_async(); /* should be ignored - busy */
    for (int i = 0; i < 50; i++) gnss_tick();

    ASSERT_FLOAT_RANGE(gnss.latitude, 0.0f, 52.0f, "State not corrupted after busy rejection");
}

void run_gnss_tests(void)
{
    TEST_SUITE("GNSS Sensor Tests (ECSS 5.5.3.2)");
    test_gnss_data_valid_ranges();
    test_gnss_multiple_reads();
    test_gnss_busy_rejection();
}
