/**
 * @file test_usart_driver.c
 * @brief Unit tests for USART Driver
 *
 * Test Coverage:
 *   - Asynchronous send (TX) operations
 *   - Asynchronous receive (RX) operations
 *   - Busy rejection (state != IDLE)
 *   - Error state handling
 *   - State machine transitions
 *   - Idle state robustness
 */

#include <stdint.h>
#include <string.h>
#include "test_utils.h"
#include "drivers/usart_driver.h"
#include "hal/hal_usart.h"

/* Callback status tracking */
static int usart_callback_status = -99;
static void usart_test_callback(int status)
{
    usart_callback_status = status;
}

/* ========================================================================
   Test 1: Complete send operation
   ======================================================================== */
static void test_usart_send_complete(void)
{
    TEST("test_usart_send_complete");

    usart_handle_t handle;
    uint8_t data[4] = {0xAA, 0xBB, 0xCC, 0xDD};

    memset(&handle, 0, sizeof(handle));
    handle.state = SERIAL_IDLE;
    handle.callback = usart_test_callback;  /* send_async doesn't set callback */
    usart_callback_status = -99;

    /* Initiate send */
    usart_send_async(&handle, data, 4);

    /* Verify setup */
    ASSERT_EQ(handle.state, SERIAL_TX_BUSY, "State changes to TX_BUSY");
    ASSERT_EQ(handle.tx_len, 4, "TX length set correctly");
    ASSERT_EQ(handle.tx_index, 0, "TX index initialized to 0");

    /* Execute state machine */
    for (int tick = 0; tick < 100 && handle.state != SERIAL_IDLE; tick++)
    {
        usart_tick(&handle);
    }

    ASSERT_EQ(handle.state, SERIAL_IDLE, "Returns to IDLE after send");
    ASSERT_EQ(usart_callback_status, 0, "Callback invoked with success (0)");
    ASSERT_EQ(handle.tx_index, 4, "All 4 bytes transmitted");
}

/* ========================================================================
   Test 2: Complete receive operation
   ======================================================================== */
static void test_usart_recv_complete(void)
{
    TEST("test_usart_recv_complete");

    usart_handle_t handle;
    uint8_t buffer[6];

    memset(&handle, 0, sizeof(handle));
    memset(buffer, 0, sizeof(buffer));
    handle.state = SERIAL_IDLE;
    usart_callback_status = -99;

    /* Initiate receive */
    usart_recv_async(&handle, buffer, 6, usart_test_callback);

    /* Verify setup */
    ASSERT_EQ(handle.state, SERIAL_RX_BUSY, "State changes to RX_BUSY");
    ASSERT_EQ(handle.rx_len, 6, "RX length set correctly");
    ASSERT_EQ(handle.rx_index, 0, "RX index initialized to 0");
    ASSERT_EQ(handle.rx_buf, buffer, "RX buffer pointer set");

    /* Execute state machine */
    for (int tick = 0; tick < 150 && handle.state != SERIAL_IDLE; tick++)
    {
        usart_tick(&handle);
    }

    ASSERT_EQ(handle.state, SERIAL_IDLE, "Returns to IDLE after receive");
    ASSERT_EQ(usart_callback_status, 0, "Callback invoked with success (0)");
    ASSERT_EQ(handle.rx_index, 6, "All 6 bytes received");
}

/* ========================================================================
   Test 3: Send busy rejection
   ======================================================================== */
static void test_usart_send_busy_rejection(void)
{
    TEST("test_usart_send_busy_rejection");

    usart_handle_t handle;
    uint8_t data1[2] = {0x11, 0x22};
    uint8_t data2[3] = {0x33, 0x44, 0x55};

    memset(&handle, 0, sizeof(handle));
    handle.state = SERIAL_TX_BUSY;  /* Already transmitting */
    handle.tx_buf = data1;
    handle.tx_len = 2;

    /* Try to send while busy */
    usart_send_async(&handle, data2, 3);

    /* Verify rejection - no state change */
    ASSERT_EQ(handle.state, SERIAL_TX_BUSY, "State unchanged when busy");
    ASSERT_EQ(handle.tx_len, 2, "TX length not overwritten");
    ASSERT_EQ(handle.tx_buf, data1, "TX buffer not changed");
}

/* ========================================================================
   Test 4: Receive busy rejection
   ======================================================================== */
static void test_usart_recv_busy_rejection(void)
{
    TEST("test_usart_recv_busy_rejection");

    usart_handle_t handle;
    uint8_t buf1[4];
    uint8_t buf2[8];

    memset(&handle, 0, sizeof(handle));
    handle.state = SERIAL_RX_BUSY;  /* Already receiving */
    handle.rx_buf = buf1;
    handle.rx_len = 4;

    /* Try to receive while busy */
    usart_recv_async(&handle, buf2, 8, usart_test_callback);

    /* Verify rejection - no state change */
    ASSERT_EQ(handle.state, SERIAL_RX_BUSY, "State unchanged when busy");
    ASSERT_EQ(handle.rx_len, 4, "RX length not overwritten");
    ASSERT_EQ(handle.rx_buf, buf1, "RX buffer not changed");
}

/* ========================================================================
   Test 5: Idle state - repeated ticks do nothing
   ======================================================================== */
static void test_usart_idle_no_crash(void)
{
    TEST("test_usart_idle_no_crash");

    usart_handle_t handle;
    memset(&handle, 0, sizeof(handle));
    handle.state = SERIAL_IDLE;

    /* Multiple ticks in IDLE should not crash */
    for (int tick = 0; tick < 10; tick++)
    {
        usart_tick(&handle);
        ASSERT_EQ(handle.state, SERIAL_IDLE, "Remains in IDLE");
    }
}

/* ========================================================================
   Test 6: Error state - tick does nothing
   ======================================================================== */
static void test_usart_error_state(void)
{
    TEST("test_usart_error_state");

    usart_handle_t handle;
    memset(&handle, 0, sizeof(handle));
    handle.state = SERIAL_ERROR;

    /* Tick in ERROR state - should remain ERROR */
    for (int tick = 0; tick < 5; tick++)
    {
        usart_tick(&handle);
        ASSERT_EQ(handle.state, SERIAL_ERROR, "Remains in ERROR state");
    }
}

/* ========================================================================
   Test 7: Send timeout handling
   ======================================================================== */
static void test_usart_send_timeout(void)
{
    TEST("test_usart_send_timeout");

    usart_handle_t handle;
    uint8_t data[2] = {0x77, 0x88};

    memset(&handle, 0, sizeof(handle));
    handle.state = SERIAL_TX_BUSY;
    handle.tx_buf = data;
    handle.tx_len = 2;
    handle.tx_index = 0;
    handle.timeout = 0;
    handle.callback = usart_test_callback;
    usart_callback_status = -99;

    /* Bloqueia TX para forçar timeout (HAL normalmente retorna tx_ready=1) */
    hal_usart_set_tx_ready(0);

    /* Simulate timeout by incrementing beyond USART_TIMEOUT_MAX */
    for (int tick = 0; tick < USART_TIMEOUT_MAX + 5; tick++)
    {
        usart_tick(&handle);
    }

    /* Restaura TX para não afectar testes seguintes */
    hal_usart_set_tx_ready(1);

    ASSERT_EQ(handle.state, SERIAL_ERROR, "Timeout transitions to ERROR state");
    ASSERT_EQ(usart_callback_status, -1, "Timeout invokes error callback");
}

/* ========================================================================
   Test 8: Receive timeout handling
   ======================================================================== */
static void test_usart_recv_timeout(void)
{
    TEST("test_usart_recv_timeout");

    usart_handle_t handle;
    uint8_t buffer[3];

    memset(&handle, 0, sizeof(handle));
    handle.state = SERIAL_RX_BUSY;
    handle.rx_buf = buffer;
    handle.rx_len = 3;
    handle.rx_index = 0;
    handle.timeout = 0;
    handle.callback = usart_test_callback;
    usart_callback_status = -99;

    /* Desativa auto-regen e esgota o buffer para rx_ready ficar a 0 */
    hal_usart_set_rx_auto_regen(0);
    while (hal_usart_data_available())
        hal_usart_read_char();

    /* Simulate timeout */
    for (int tick = 0; tick < USART_TIMEOUT_MAX + 5; tick++)
    {
        usart_tick(&handle);
    }

    /* Restaura auto-regen e regenera pacote para testes seguintes */
    hal_usart_set_rx_auto_regen(1);
    hal_usart_prepare_rx(); /* gera novo pacote: rx_ready = 1 */

    ASSERT_EQ(handle.state, SERIAL_ERROR, "RX timeout transitions to ERROR");
    ASSERT_EQ(usart_callback_status, -1, "RX timeout invokes error callback");
}

/* ========================================================================
   Test 9: Boundary - single byte send
   ======================================================================== */
static void test_usart_send_single_byte(void)
{
    TEST("test_usart_send_single_byte");

    usart_handle_t handle;
    uint8_t data[1] = {0xFF};

    memset(&handle, 0, sizeof(handle));
    handle.state = SERIAL_IDLE;
    handle.callback = usart_test_callback;
    usart_callback_status = -99;

    usart_send_async(&handle, data, 1);

    for (int tick = 0; tick < 50 && handle.state != SERIAL_IDLE; tick++)
    {
        usart_tick(&handle);
    }

    ASSERT_EQ(handle.state, SERIAL_IDLE, "Single byte send completes");
    ASSERT_EQ(usart_callback_status, 0, "Callback success");
    ASSERT_EQ(handle.tx_index, 1, "Single byte transmitted");
}

/* ========================================================================
   Test 10: Boundary - single byte receive
   ======================================================================== */
static void test_usart_recv_single_byte(void)
{
    TEST("test_usart_recv_single_byte");

    usart_handle_t handle;
    uint8_t buffer[1];

    memset(&handle, 0, sizeof(handle));
    memset(buffer, 0, 1);
    handle.state = SERIAL_IDLE;
    usart_callback_status = -99;

    usart_recv_async(&handle, buffer, 1, usart_test_callback);

    for (int tick = 0; tick < 50 && handle.state != SERIAL_IDLE; tick++)
    {
        usart_tick(&handle);
    }

    ASSERT_EQ(handle.state, SERIAL_IDLE, "Single byte receive completes");
    ASSERT_EQ(usart_callback_status, 0, "Callback success");
    ASSERT_EQ(handle.rx_index, 1, "Single byte received");
}

/* ========================================================================
   Test 11: Large buffer - 16 bytes send
   ======================================================================== */
static void test_usart_send_large_buffer(void)
{
    TEST("test_usart_send_large_buffer");

    usart_handle_t handle;
    uint8_t data[16];
    for (int i = 0; i < 16; i++)
        data[i] = i + 1;

    memset(&handle, 0, sizeof(handle));
    handle.state = SERIAL_IDLE;
    handle.callback = usart_test_callback;
    usart_callback_status = -99;

    usart_send_async(&handle, data, 16);

    for (int tick = 0; tick < 200 && handle.state != SERIAL_IDLE; tick++)
    {
        usart_tick(&handle);
    }

    ASSERT_EQ(handle.state, SERIAL_IDLE, "Large buffer send completes");
    ASSERT_EQ(usart_callback_status, 0, "Callback success");
    ASSERT_EQ(handle.tx_index, 16, "All 16 bytes transmitted");
}

/* ========================================================================
   Test 12: Large buffer - 16 bytes receive
   ======================================================================== */
static void test_usart_recv_large_buffer(void)
{
    TEST("test_usart_recv_large_buffer");

    usart_handle_t handle;
    uint8_t buffer[16];

    memset(&handle, 0, sizeof(handle));
    memset(buffer, 0, 16);
    handle.state = SERIAL_IDLE;
    usart_callback_status = -99;

    usart_recv_async(&handle, buffer, 16, usart_test_callback);

    for (int tick = 0; tick < 250 && handle.state != SERIAL_IDLE; tick++)
    {
        usart_tick(&handle);
    }

    ASSERT_EQ(handle.state, SERIAL_IDLE, "Large buffer receive completes");
    ASSERT_EQ(usart_callback_status, 0, "Callback success");
    ASSERT_EQ(handle.rx_index, 16, "All 16 bytes received");
}

/* ========================================================================
   Test 13: Interleaved send and receive (separate handles)
   ======================================================================== */
static void test_usart_interleaved_operations(void)
{
    TEST("test_usart_interleaved_operations");

    usart_handle_t tx_handle, rx_handle;
    uint8_t tx_data[3] = {0xAA, 0xBB, 0xCC};
    uint8_t rx_buffer[4];

    memset(&tx_handle, 0, sizeof(tx_handle));
    memset(&rx_handle, 0, sizeof(rx_handle));
    memset(rx_buffer, 0, 4);

    tx_handle.state = SERIAL_IDLE;
    tx_handle.callback = usart_test_callback;
    rx_handle.state = SERIAL_IDLE;
    usart_callback_status = -99;

    /* Start TX */
    usart_send_async(&tx_handle, tx_data, 3);
    ASSERT_EQ(tx_handle.state, SERIAL_TX_BUSY, "TX initiated");

    /* Start RX on separate handle */
    usart_recv_async(&rx_handle, rx_buffer, 4, usart_test_callback);
    ASSERT_EQ(rx_handle.state, SERIAL_RX_BUSY, "RX initiated");

    /* Interleave ticks */
    int tx_done = 0, rx_done = 0;
    for (int tick = 0; tick < 200; tick++)
    {
        usart_tick(&tx_handle);
        usart_tick(&rx_handle);

        if (tx_handle.state == SERIAL_IDLE && !tx_done)
            tx_done = 1;
        if (rx_handle.state == SERIAL_IDLE && !rx_done)
            rx_done = 1;

        if (tx_done && rx_done)
            break;
    }

    ASSERT_EQ(tx_done, 1, "TX completed");
    ASSERT_EQ(rx_done, 1, "RX completed");
    ASSERT_EQ(tx_handle.state, SERIAL_IDLE, "TX handle idle");
    ASSERT_EQ(rx_handle.state, SERIAL_IDLE, "RX handle idle");
}

/* ========================================================================
   Test runner function
   ======================================================================== */
void run_usart_driver_tests(void)
{
    TEST_SUITE("USART Driver Unit Tests");

    test_usart_send_complete();
    test_usart_recv_complete();
    test_usart_send_busy_rejection();
    test_usart_recv_busy_rejection();
    test_usart_idle_no_crash();
    test_usart_error_state();
    test_usart_send_timeout();
    test_usart_recv_timeout();
    test_usart_send_single_byte();
    test_usart_recv_single_byte();
    test_usart_send_large_buffer();
    test_usart_recv_large_buffer();
    test_usart_interleaved_operations();

    TEST_SUMMARY();
}
