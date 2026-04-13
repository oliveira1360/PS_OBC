/**
 * @file test_i2c_driver.c
 * @brief Unit tests for I2C Driver (ECSS 5.5.3.2 - Boundary, Error, Stress)
 *
 * Test Coverage:
 *   - Single read/write operations
 *   - State machine transitions
 *   - NACK handling with invalid addresses
 *   - Concurrent operations on multiple peripherals
 *   - Boundary conditions (min/max buffer lengths)
 *   - Device address validation (all 5 I2C devices)
 *   - Non-blocking tick behavior
 */

#include <stdint.h>
#include <string.h>
#include "test_utils.h"
#include "drivers/i2c_driver.h"
#include "config/board.h"

/* Global callback flag for async tracking */
static int callback_status = -99;
static void test_callback(int status)
{
    callback_status = status;
}

/* ========================================================================
   Test 1: Single device read (GNSS)
   ======================================================================== */
static void test_i2c_read_single_device(void)
{
    TEST("test_i2c_read_single_device");

    i2c_handle_t handle;
    uint8_t buffer[GNSS_BUF_LEN];

    memset(&handle, 0, sizeof(handle));
    memset(buffer, 0, GNSS_BUF_LEN);

    handle.state = I2C_IDLE;
    handle.addr = GNSS_ADDR;
    handle.buf = buffer;
    handle.len = GNSS_BUF_LEN;
    handle.rw = 1;  /* READ */
    handle.callback = test_callback;
    callback_status = -99;

    /* Verify initial state */
    ASSERT_EQ(handle.state, I2C_IDLE, "Initial state is IDLE");
    ASSERT_EQ(handle.addr, GNSS_ADDR, "Address set to GNSS");
    ASSERT_EQ(handle.len, GNSS_BUF_LEN, "Length set to GNSS buffer size");

    /* Manually start read operation (normally done by i2c_read) */
    handle.state = I2C_STARTING;

    /* Tick through state machine - should eventually complete */
    for (int tick = 0; tick < 100 && handle.state != I2C_IDLE; tick++)
    {
        i2c_tick(&handle);
    }

    /* Verify completion */
    ASSERT_EQ(handle.state, I2C_IDLE, "State returns to IDLE after read");
    ASSERT_EQ(callback_status, 0, "Callback invoked with success (0)");
    /* buffer[7] = spd_dec = 75..84, nunca zero independentemente do orbit_step */
    ASSERT_NEQ(buffer[7], 0, "Buffer filled with data (speed byte always >= 75)");
}

/* ========================================================================
   Test 2: Write operation
   ======================================================================== */
static void test_i2c_write_operation(void)
{
    TEST("test_i2c_write_operation");

    i2c_handle_t handle;
    uint8_t data[4] = {0xAA, 0xBB, 0xCC, 0xDD};

    memset(&handle, 0, sizeof(handle));
    handle.state = I2C_IDLE;
    handle.addr = IMU_ADDR;
    handle.buf = data;
    handle.len = 4;
    handle.rw = 0;  /* WRITE */
    handle.callback = test_callback;
    callback_status = -99;

    /* Start write */
    handle.state = I2C_STARTING;

    /* Tick through state machine */
    for (int tick = 0; tick < 100 && handle.state != I2C_IDLE; tick++)
    {
        i2c_tick(&handle);
    }

    ASSERT_EQ(handle.state, I2C_IDLE, "State returns to IDLE after write");
    ASSERT_EQ(callback_status, 0, "Callback invoked with success (0)");
    ASSERT_EQ(handle.index, 4, "All 4 bytes transmitted");
}

/* ========================================================================
   Test 3: NACK handling - invalid device address
   ======================================================================== */
static void test_i2c_nack_handling(void)
{
    TEST("test_i2c_nack_handling");

    i2c_handle_t handle;
    uint8_t buffer[8];

    memset(&handle, 0, sizeof(handle));
    handle.state = I2C_IDLE;
    handle.addr = 0x99;  /* Invalid address - no device */
    handle.buf = buffer;
    handle.len = 8;
    handle.rw = 1;  /* READ */
    handle.callback = test_callback;
    callback_status = -99;

    /* Start read */
    handle.state = I2C_STARTING;

    /* Tick through state machine - should fail quickly */
    for (int tick = 0; tick < 20 && handle.state != I2C_IDLE; tick++)
    {
        i2c_tick(&handle);
    }

    ASSERT_EQ(handle.state, I2C_IDLE, "State returns to IDLE on NACK");
    ASSERT_EQ(callback_status, -1, "Callback invoked with error (-1) on NACK");
}

/* ========================================================================
   Test 4: Multiple peripherals - concurrent reads
   ======================================================================== */
static void test_i2c_multiple_peripherals(void)
{
    TEST("test_i2c_multiple_peripherals");

    i2c_handle_t gnss_handle, imu_handle, press_handle;
    uint8_t gnss_buf[GNSS_BUF_LEN];
    uint8_t imu_buf[IMU_BUF_LEN];
    uint8_t press_buf[PRES_BUF_LEN];

    memset(&gnss_handle, 0, sizeof(gnss_handle));
    memset(&imu_handle, 0, sizeof(imu_handle));
    memset(&press_handle, 0, sizeof(press_handle));

    /* Initialize all handles */
    gnss_handle.state = I2C_STARTING;
    gnss_handle.addr = GNSS_ADDR;
    gnss_handle.buf = gnss_buf;
    gnss_handle.len = GNSS_BUF_LEN;
    gnss_handle.rw = 1;
    gnss_handle.callback = test_callback;

    imu_handle.state = I2C_STARTING;
    imu_handle.addr = IMU_ADDR;
    imu_handle.buf = imu_buf;
    imu_handle.len = IMU_BUF_LEN;
    imu_handle.rw = 1;
    imu_handle.callback = test_callback;

    press_handle.state = I2C_STARTING;
    press_handle.addr = PRESS_ADDR;
    press_handle.buf = press_buf;
    press_handle.len = PRES_BUF_LEN;
    press_handle.rw = 1;
    press_handle.callback = test_callback;

    /* Execute concurrent ticks (simulation of separate I2C channels) */
    int gnss_done = 0, imu_done = 0, press_done = 0;
    for (int tick = 0; tick < 200; tick++)
    {
        if (gnss_handle.state != I2C_IDLE)
            i2c_tick(&gnss_handle);
        else if (!gnss_done)
        {
            gnss_done = 1;
            ASSERT_EQ(callback_status, 0, "GNSS read completed successfully");
        }

        if (imu_handle.state != I2C_IDLE)
            i2c_tick(&imu_handle);
        else if (!imu_done)
        {
            imu_done = 1;
            ASSERT_EQ(callback_status, 0, "IMU read completed successfully");
        }

        if (press_handle.state != I2C_IDLE)
            i2c_tick(&press_handle);
        else if (!press_done)
        {
            press_done = 1;
            ASSERT_EQ(callback_status, 0, "Pressure read completed successfully");
        }

        if (gnss_done && imu_done && press_done)
            break;
    }

    ASSERT_EQ(gnss_done, 1, "GNSS transfer completed");
    ASSERT_EQ(imu_done, 1, "IMU transfer completed");
    ASSERT_EQ(press_done, 1, "Pressure transfer completed");
}

/* ========================================================================
   Test 5: Non-blocking behavior - tick advances state machine
   ======================================================================== */
static void test_i2c_non_blocking(void)
{
    TEST("test_i2c_non_blocking");

    i2c_handle_t handle;
    uint8_t buffer[8];

    memset(&handle, 0, sizeof(handle));
    handle.state = I2C_STARTING;
    handle.addr = TEMP_ADDR;
    handle.buf = buffer;
    handle.len = TEMP_BUF_LEN;
    handle.rw = 1;
    handle.callback = test_callback;
    callback_status = -99;

    /* Each tick should advance state machine without blocking */
    i2c_state_t prev_state = handle.state;
    int state_changed = 0;

    for (int tick = 0; tick < 20; tick++)
    {
        i2c_tick(&handle);

        if (handle.state != prev_state)
        {
            state_changed = 1;
            break;
        }
    }

    ASSERT_EQ(state_changed, 1, "State machine advances with tick calls");

    /* Completar a transação para libertar o bus — evita que testes seguintes bloqueiem */
    for (int tick = 0; tick < 100 && handle.state != I2C_IDLE; tick++)
        i2c_tick(&handle);
}

/* ========================================================================
   Test 6: Boundary - minimum buffer length (1 byte)
   ======================================================================== */
static void test_i2c_boundary_min_length(void)
{
    TEST("test_i2c_boundary_min_length");

    i2c_handle_t handle;
    uint8_t buffer[1];

    memset(&handle, 0, sizeof(handle));
    handle.state = I2C_STARTING;
    handle.addr = EPS_ADDR;
    handle.buf = buffer;
    handle.len = 1;  /* Minimum */
    handle.rw = 1;   /* READ */
    handle.callback = test_callback;
    callback_status = -99;

    for (int tick = 0; tick < 100 && handle.state != I2C_IDLE; tick++)
    {
        i2c_tick(&handle);
    }

    ASSERT_EQ(handle.state, I2C_IDLE, "Handles min length (1) correctly");
    ASSERT_EQ(callback_status, 0, "Min length read completes successfully");
}

/* ========================================================================
   Test 7: Boundary - maximum buffer length (IMU = 18 bytes)
   ======================================================================== */
static void test_i2c_boundary_max_length(void)
{
    TEST("test_i2c_boundary_max_length");

    i2c_handle_t handle;
    uint8_t buffer[IMU_BUF_LEN];

    memset(&handle, 0, sizeof(handle));
    memset(buffer, 0, IMU_BUF_LEN);

    handle.state = I2C_STARTING;
    handle.addr = IMU_ADDR;
    handle.buf = buffer;
    handle.len = IMU_BUF_LEN;  /* Maximum (18) */
    handle.rw = 1;             /* READ */
    handle.callback = test_callback;
    callback_status = -99;

    for (int tick = 0; tick < 200 && handle.state != I2C_IDLE; tick++)
    {
        i2c_tick(&handle);
    }

    ASSERT_EQ(handle.state, I2C_IDLE, "Handles max length (18) correctly");
    ASSERT_EQ(callback_status, 0, "Max length read completes successfully");
    ASSERT_EQ(handle.index, IMU_BUF_LEN, "All 18 bytes received");
}

/* ========================================================================
   Test 8: IDLE state - tick should do nothing
   ======================================================================== */
static void test_i2c_idle_no_crash(void)
{
    TEST("test_i2c_idle_no_crash");

    i2c_handle_t handle;
    memset(&handle, 0, sizeof(handle));
    handle.state = I2C_IDLE;

    /* Tick multiple times in IDLE - should not crash or change state */
    for (int tick = 0; tick < 10; tick++)
    {
        i2c_tick(&handle);
        ASSERT_EQ(handle.state, I2C_IDLE, "Remains in IDLE state");
    }
}

/* ========================================================================
   Test 9: All device addresses - ACK verification
   ======================================================================== */
static void test_i2c_all_devices(void)
{
    TEST("test_i2c_all_devices");

    uint8_t addresses[] = {GNSS_ADDR, IMU_ADDR, PRESS_ADDR, TEMP_ADDR, EPS_ADDR};
    const char* names[] = {"GNSS", "IMU", "PRESSURE", "TEMPERATURE", "EPS"};
    uint8_t buf_sizes[] = {GNSS_BUF_LEN, IMU_BUF_LEN, PRES_BUF_LEN, TEMP_BUF_LEN, EPS_BUF_LEN};

    for (int i = 0; i < 5; i++)
    {
        i2c_handle_t handle;
        uint8_t buffer[IMU_BUF_LEN];  /* Use max size */

        memset(&handle, 0, sizeof(handle));
        handle.state = I2C_STARTING;
        handle.addr = addresses[i];
        handle.buf = buffer;
        handle.len = buf_sizes[i];
        handle.rw = 1;
        handle.callback = test_callback;
        callback_status = -99;

        /* Execute read */
        for (int tick = 0; tick < 200 && handle.state != I2C_IDLE; tick++)
        {
            i2c_tick(&handle);
        }

        char msg[50];
        snprintf(msg, sizeof(msg), "%s device read completes", names[i]);
        ASSERT_EQ(handle.state, I2C_IDLE, msg);
        ASSERT_EQ(callback_status, 0, names[i]);
    }
}

/* ========================================================================
   Test runner function
   ======================================================================== */
void run_i2c_driver_tests(void)
{
    TEST_SUITE("I2C Driver Unit Tests (ECSS 5.5.3.2)");

    test_i2c_read_single_device();
    test_i2c_write_operation();
    test_i2c_nack_handling();
    test_i2c_multiple_peripherals();
    test_i2c_non_blocking();
    test_i2c_boundary_min_length();
    test_i2c_boundary_max_length();
    test_i2c_idle_no_crash();
    test_i2c_all_devices();

    TEST_SUMMARY();
}
