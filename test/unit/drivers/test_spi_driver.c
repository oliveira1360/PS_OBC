/**
 * @file test_spi_driver.c
 * @brief Unit tests for SPI Driver
 *
 * Test Coverage:
 *   - Complete transfer cycle (CS, TX, RX, release)
 *   - Busy rejection (state != IDLE)
 *   - NULL buffer handling (tx_buf=NULL sends 0xFF, rx_buf=NULL discards)
 *   - State machine transitions
 *   - Idle state robustness
 */

#include <stdint.h>
#include <string.h>
#include "test_utils.h"
#include "drivers/spi_driver.h"

/* Callback status tracking */
static int spi_callback_status = -99;
static void spi_test_callback(int status)
{
    spi_callback_status = status;
}

/* ========================================================================
   Test 1: Complete SPI transfer cycle
   ======================================================================== */
static void test_spi_transfer_complete(void)
{
    TEST("test_spi_transfer_complete");

    spi_handle_t handle;
    uint8_t tx_buf[4] = {0x11, 0x22, 0x33, 0x44};
    uint8_t rx_buf[4] = {0x00, 0x00, 0x00, 0x00};

    memset(&handle, 0, sizeof(handle));
    handle.state = SPI_IDLE;
    spi_callback_status = -99;

    /* Initiate async transfer */
    spi_transfer_async(&handle, 5, tx_buf, rx_buf, 4, spi_test_callback);

    /* Verify setup */
    ASSERT_EQ(handle.state, SPI_CS_LOW, "Transfer starts with CS_LOW");
    ASSERT_EQ(handle.cs_pin, 5, "CS pin configured correctly");
    ASSERT_EQ(handle.len, 4, "Transfer length set");
    ASSERT_EQ(handle.index, 0, "Index initialized to 0");

    /* Execute state machine */
    for (int tick = 0; tick < 100 && handle.state != SPI_IDLE; tick++)
    {
        spi_tick(&handle);
    }

    ASSERT_EQ(handle.state, SPI_IDLE, "Returns to IDLE after transfer");
    ASSERT_EQ(spi_callback_status, 0, "Callback invoked with success (0)");
    ASSERT_EQ(handle.index, 4, "All 4 bytes transferred");
}

/* ========================================================================
   Test 2: Busy rejection - transfer_async on non-IDLE state
   ======================================================================== */
static void test_spi_busy_rejection(void)
{
    TEST("test_spi_busy_rejection");

    spi_handle_t handle;
    uint8_t tx_buf[2] = {0xAA, 0xBB};
    uint8_t rx_buf[2] = {0x00, 0x00};

    memset(&handle, 0, sizeof(handle));
    handle.state = SPI_CS_LOW;  /* Non-IDLE state */
    handle.cs_pin = 3;
    handle.len = 5;             /* Existing transfer */

    /* Try to start another transfer while busy */
    spi_transfer_async(&handle, 7, tx_buf, rx_buf, 2, spi_test_callback);

    /* Verify rejection - no state change */
    ASSERT_EQ(handle.state, SPI_CS_LOW, "State unchanged when busy");
    ASSERT_EQ(handle.cs_pin, 3, "CS pin not modified");
    ASSERT_EQ(handle.len, 5, "Length not overwritten");
}

/* ========================================================================
   Test 3: Idle state - repeated ticks do nothing
   ======================================================================== */
static void test_spi_idle_no_crash(void)
{
    TEST("test_spi_idle_no_crash");

    spi_handle_t handle;
    memset(&handle, 0, sizeof(handle));
    handle.state = SPI_IDLE;

    /* Multiple ticks in IDLE should not crash */
    for (int tick = 0; tick < 10; tick++)
    {
        spi_tick(&handle);
        ASSERT_EQ(handle.state, SPI_IDLE, "Remains in IDLE");
    }
}

/* ========================================================================
   Test 4: NULL TX buffer - should transmit 0xFF
   ======================================================================== */
static void test_spi_null_tx(void)
{
    TEST("test_spi_null_tx");

    spi_handle_t handle;
    uint8_t rx_buf[3] = {0x00, 0x00, 0x00};

    memset(&handle, 0, sizeof(handle));
    handle.state = SPI_IDLE;
    spi_callback_status = -99;

    /* Transfer with NULL tx_buf */
    spi_transfer_async(&handle, 6, NULL, rx_buf, 3, spi_test_callback);

    ASSERT_EQ(handle.tx_buf, NULL, "TX buffer is NULL");
    ASSERT_EQ(handle.state, SPI_CS_LOW, "Transfer initiated");

    /* Execute transfer */
    for (int tick = 0; tick < 100 && handle.state != SPI_IDLE; tick++)
    {
        spi_tick(&handle);
    }

    ASSERT_EQ(handle.state, SPI_IDLE, "Transfer completes with NULL TX");
    ASSERT_EQ(spi_callback_status, 0, "Callback success");
}

/* ========================================================================
   Test 5: NULL RX buffer - should discard data
   ======================================================================== */
static void test_spi_null_rx(void)
{
    TEST("test_spi_null_rx");

    spi_handle_t handle;
    uint8_t tx_buf[2] = {0x55, 0x66};

    memset(&handle, 0, sizeof(handle));
    handle.state = SPI_IDLE;
    spi_callback_status = -99;

    /* Transfer with NULL rx_buf */
    spi_transfer_async(&handle, 4, tx_buf, NULL, 2, spi_test_callback);

    ASSERT_EQ(handle.rx_buf, NULL, "RX buffer is NULL");
    ASSERT_EQ(handle.state, SPI_CS_LOW, "Transfer initiated");

    /* Execute transfer */
    for (int tick = 0; tick < 100 && handle.state != SPI_IDLE; tick++)
    {
        spi_tick(&handle);
    }

    ASSERT_EQ(handle.state, SPI_IDLE, "Transfer completes with NULL RX");
    ASSERT_EQ(spi_callback_status, 0, "Callback success");
    ASSERT_EQ(handle.index, 2, "Data read and discarded (index = len)");
}

/* ========================================================================
   Test 6: State transitions - verify full sequence
   ======================================================================== */
static void test_spi_state_transitions(void)
{
    TEST("test_spi_state_transitions");

    spi_handle_t handle;
    uint8_t tx_buf[1] = {0xCC};
    uint8_t rx_buf[1] = {0x00};

    memset(&handle, 0, sizeof(handle));
    handle.state = SPI_IDLE;
    spi_callback_status = -99;

    /* Initiate transfer */
    spi_transfer_async(&handle, 2, tx_buf, rx_buf, 1, spi_test_callback);
    ASSERT_EQ(handle.state, SPI_CS_LOW, "State 1: CS_LOW");

    /* Single tick to advance */
    spi_tick(&handle);
    ASSERT_EQ(handle.state, SPI_TRANSFER, "State 2: TRANSFER");

    /* Continue execution */
    spi_state_t prev_state = SPI_TRANSFER;
    spi_state_t states_seen[7];
    int state_count = 1;
    states_seen[0] = SPI_TRANSFER;

    for (int tick = 0; tick < 50 && handle.state != SPI_IDLE; tick++)
    {
        spi_tick(&handle);
        if (handle.state != prev_state)
        {
            states_seen[state_count++] = handle.state;
            prev_state = handle.state;
        }
    }

    ASSERT(state_count >= 5, "Multiple state transitions observed");
    ASSERT_EQ(handle.state, SPI_IDLE, "Final state is IDLE");
}

/* ========================================================================
   Test 7: Boundary - single byte transfer
   ======================================================================== */
static void test_spi_single_byte(void)
{
    TEST("test_spi_single_byte");

    spi_handle_t handle;
    uint8_t tx_buf[1] = {0x99};
    uint8_t rx_buf[1] = {0x00};

    memset(&handle, 0, sizeof(handle));
    handle.state = SPI_IDLE;
    spi_callback_status = -99;

    spi_transfer_async(&handle, 1, tx_buf, rx_buf, 1, spi_test_callback);

    for (int tick = 0; tick < 50 && handle.state != SPI_IDLE; tick++)
    {
        spi_tick(&handle);
    }

    ASSERT_EQ(handle.state, SPI_IDLE, "Single byte transfer completes");
    ASSERT_EQ(spi_callback_status, 0, "Callback success");
    ASSERT_EQ(handle.index, 1, "Single byte transferred");
}

/* ========================================================================
   Test 8: Boundary - maximum typical transfer (8 bytes)
   ======================================================================== */
static void test_spi_max_transfer(void)
{
    TEST("test_spi_max_transfer");

    spi_handle_t handle;
    uint8_t tx_buf[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    uint8_t rx_buf[8] = {0x00};

    memset(&handle, 0, sizeof(handle));
    handle.state = SPI_IDLE;
    spi_callback_status = -99;

    spi_transfer_async(&handle, 0, tx_buf, rx_buf, 8, spi_test_callback);

    for (int tick = 0; tick < 200 && handle.state != SPI_IDLE; tick++)
    {
        spi_tick(&handle);
    }

    ASSERT_EQ(handle.state, SPI_IDLE, "8-byte transfer completes");
    ASSERT_EQ(spi_callback_status, 0, "Callback success");
    ASSERT_EQ(handle.index, 8, "All 8 bytes transferred");
}

/* ========================================================================
   Test 9: Timeout handling - verify callback on timeout
   ======================================================================== */
static void test_spi_timeout(void)
{
    TEST("test_spi_timeout");

    spi_handle_t handle;
    uint8_t tx_buf[2] = {0x12, 0x34};
    uint8_t rx_buf[2] = {0x00, 0x00};

    memset(&handle, 0, sizeof(handle));
    handle.state = SPI_IDLE;
    spi_callback_status = -99;

    spi_transfer_async(&handle, 3, tx_buf, rx_buf, 2, spi_test_callback);

    /* Place handle in a wait state and simulate timeout */
    handle.state = SPI_WAIT_TX;
    handle.timeout = 0;

    /* Increment timeout beyond limit */
    for (int tick = 0; tick < SPI_TIMEOUT_MAX + 5; tick++)
    {
        spi_tick(&handle);
    }

    /* After timeout, should return to IDLE with error callback */
    ASSERT_EQ(handle.state, SPI_IDLE, "Timeout returns to IDLE");
    ASSERT_EQ(spi_callback_status, -1, "Timeout invokes error callback");
}

/* ========================================================================
   Test runner function
   ======================================================================== */
void run_spi_driver_tests(void)
{
    TEST_SUITE("SPI Driver Unit Tests");

    test_spi_transfer_complete();
    test_spi_busy_rejection();
    test_spi_idle_no_crash();
    test_spi_null_tx();
    test_spi_null_rx();
    test_spi_state_transitions();
    test_spi_single_byte();
    test_spi_max_transfer();
    test_spi_timeout();

    TEST_SUMMARY();
}
