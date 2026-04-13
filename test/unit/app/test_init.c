/**
 * @file test_init.c
 * @brief Testes unitarios para o modulo de inicializacao (ECSS 5.5.3.2)
 */
#include "test_utils.h"
#include "app/init.h"

static void test_system_init_success(void)
{
    TEST("system_init returns 1 on success");
    init_status = (init_status_t){0};
    int result = system_init();
    ASSERT_EQ(result, 1, "system_init() returns 1");
}

static void test_init_status_fields(void)
{
    TEST("All 11 init_status fields set");
    init_status = (init_status_t){0};
    system_init();
    ASSERT_EQ(init_status.gpio, 1, "gpio initialized");
    ASSERT_EQ(init_status.i2c, 1, "i2c initialized");
    ASSERT_EQ(init_status.spi, 1, "spi initialized");
    ASSERT_EQ(init_status.qspi, 1, "qspi initialized");
    ASSERT_EQ(init_status.usart, 1, "usart initialized");
    ASSERT_EQ(init_status.gnss, 1, "gnss initialized");
    ASSERT_EQ(init_status.imu, 1, "imu initialized");
    ASSERT_EQ(init_status.ttc, 1, "ttc initialized");
    ASSERT_EQ(init_status.pressure, 1, "pressure initialized");
    ASSERT_EQ(init_status.temperature, 1, "temperature initialized");
    ASSERT_EQ(init_status.ext_memory, 1, "ext_memory initialized");
}

static void test_init_idempotent(void)
{
    TEST("system_init is idempotent");
    init_status = (init_status_t){0};
    int r1 = system_init();
    int r2 = system_init();
    ASSERT_EQ(r1, 1, "First call returns 1");
    ASSERT_EQ(r2, 1, "Second call returns 1 (idempotent)");
}

void run_init_tests(void)
{
    TEST_SUITE("Init Module Tests (ECSS 5.5.3.2)");
    test_system_init_success();
    test_init_status_fields();
    test_init_idempotent();
}
