/**
 * @file mock_hal_i2c.c
 * @brief Implementação do mock HAL I2C.
 *
 * Substitui hal_i2c.c na compilação dos testes de driver.
 */

#include "mock_hal_i2c.h"

mock_i2c_t mock_i2c;

/* =========================================================================
 * Controlo do mock
 * ========================================================================= */

void mock_i2c_reset(void)
{
    memset(&mock_i2c, 0, sizeof(mock_i2c));
    mock_i2c.bus_free_default  = 1; /* barramento livre por omissão */
    mock_i2c.ack_default       = 1; /* ACK por omissão              */
    mock_i2c.tx_ready_default  = 1; /* TX pronto por omissão        */
    mock_i2c.rx_ready_default  = 1; /* RX pronto por omissão        */
    mock_i2c.rx_data_default   = 0x00U;
}

void mock_i2c_set_bus_free_seq(const uint8_t *seq, int len)
{
    int n = (len < MOCK_I2C_SEQ_MAX) ? len : MOCK_I2C_SEQ_MAX;
    memcpy(mock_i2c.bus_free_seq, seq, (size_t)n);
    mock_i2c.bus_free_seq_len  = n;
    mock_i2c.bus_free_call_count = 0;
}

void mock_i2c_set_ack_seq(const uint8_t *seq, int len)
{
    int n = (len < MOCK_I2C_SEQ_MAX) ? len : MOCK_I2C_SEQ_MAX;
    memcpy(mock_i2c.ack_seq, seq, (size_t)n);
    mock_i2c.ack_seq_len   = n;
    mock_i2c.ack_call_count = 0;
}

void mock_i2c_set_tx_ready_seq(const uint8_t *seq, int len)
{
    int n = (len < MOCK_I2C_SEQ_MAX) ? len : MOCK_I2C_SEQ_MAX;
    memcpy(mock_i2c.tx_ready_seq, seq, (size_t)n);
    mock_i2c.tx_ready_seq_len   = n;
    mock_i2c.tx_ready_call_count = 0;
}

void mock_i2c_set_rx_ready_seq(const uint8_t *seq, int len)
{
    int n = (len < MOCK_I2C_SEQ_MAX) ? len : MOCK_I2C_SEQ_MAX;
    memcpy(mock_i2c.rx_ready_seq, seq, (size_t)n);
    mock_i2c.rx_ready_seq_len   = n;
    mock_i2c.rx_ready_call_count = 0;
}

void mock_i2c_set_rx_data_seq(const uint8_t *seq, int len)
{
    int n = (len < MOCK_I2C_BUF_MAX) ? len : MOCK_I2C_BUF_MAX;
    memcpy(mock_i2c.rx_data_seq, seq, (size_t)n);
    mock_i2c.rx_data_seq_len   = n;
    mock_i2c.rx_data_call_count = 0;
}

/* =========================================================================
 * Implementações das funções HAL
 * ========================================================================= */

uint8_t hal_i2c_init(void)
{
    mock_i2c.init_count++;
    return 1U;
}

int hal_i2c_bus_free(void)
{
    int idx = mock_i2c.bus_free_call_count++;
    if (idx < mock_i2c.bus_free_seq_len)
        return (int)mock_i2c.bus_free_seq[idx];
    return mock_i2c.bus_free_default;
}

void hal_i2c_start(void)
{
    mock_i2c.start_count++;
}

void hal_i2c_stop(void)
{
    mock_i2c.stop_count++;
}

void hal_i2c_send_addr(uint8_t byte)
{
    mock_i2c.send_addr_count++;
    mock_i2c.last_addr = byte;
}

void hal_i2c_send_byte(uint8_t data)
{
    int idx = mock_i2c.sent_bytes_count++;
    if (idx < MOCK_I2C_BUF_MAX)
        mock_i2c.sent_bytes[idx] = data;
}

int hal_i2c_get_ack(void)
{
    int idx = mock_i2c.ack_call_count++;
    if (idx < mock_i2c.ack_seq_len)
        return (int)mock_i2c.ack_seq[idx];
    return mock_i2c.ack_default;
}

uint8_t hal_i2c_read_byte(void)
{
    int idx = mock_i2c.rx_data_call_count++;
    if (idx < mock_i2c.rx_data_seq_len)
        return mock_i2c.rx_data_seq[idx];
    return mock_i2c.rx_data_default;
}

void hal_i2c_send_ack(void)
{
    mock_i2c.send_ack_count++;
}

void hal_i2c_send_nack(void)
{
    mock_i2c.send_nack_count++;
}

int hal_i2c_tx_ready(void)
{
    int idx = mock_i2c.tx_ready_call_count++;
    if (idx < mock_i2c.tx_ready_seq_len)
        return (int)mock_i2c.tx_ready_seq[idx];
    return mock_i2c.tx_ready_default;
}

int hal_i2c_rx_ready(void)
{
    int idx = mock_i2c.rx_ready_call_count++;
    if (idx < mock_i2c.rx_ready_seq_len)
        return (int)mock_i2c.rx_ready_seq[idx];
    return mock_i2c.rx_ready_default;
}

void hal_i2c_request_byte(void)
{
    mock_i2c.request_byte_count++;
}

void hal_i2c_restart_read(uint8_t addr)
{
    mock_i2c.restart_read_count++;
    mock_i2c.restart_read_last_addr = addr;
}

void hal_i2c_bus_recovery(void)
{
    mock_i2c.bus_recovery_count++;
}
