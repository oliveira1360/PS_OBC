/**
 * @file mock_hal_spi.c
 * @brief Implementação do mock HAL SPI.
 *
 * Substitui hal_spi.c na compilação dos testes de driver.
 * Não acede a registos de hardware; toda a lógica é controlada
 * pela struct mock_spi.
 */

#include "mock_hal_spi.h"

/* Instância global */
mock_spi_t mock_spi;

/* =========================================================================
 * Controlo do mock
 * ========================================================================= */

void mock_spi_reset(void)
{
    memset(&mock_spi, 0, sizeof(mock_spi));
    mock_spi.tx_ready_default  = 1; /* Por omissão: TX sempre pronto */
    mock_spi.rx_ready_default  = 1; /* Por omissão: RX sempre pronto */
    mock_spi.rx_data_default   = 0x00U;
    mock_spi.init_return       = 1U;
}

void mock_spi_set_tx_ready_seq(const uint8_t *seq, int len)
{
    int n = (len < MOCK_SEQ_MAX) ? len : MOCK_SEQ_MAX;
    memcpy(mock_spi.tx_ready_seq, seq, (size_t)n);
    mock_spi.tx_ready_seq_len   = n;
    mock_spi.tx_ready_call_count = 0;
}

void mock_spi_set_rx_ready_seq(const uint8_t *seq, int len)
{
    int n = (len < MOCK_SEQ_MAX) ? len : MOCK_SEQ_MAX;
    memcpy(mock_spi.rx_ready_seq, seq, (size_t)n);
    mock_spi.rx_ready_seq_len   = n;
    mock_spi.rx_ready_call_count = 0;
}

void mock_spi_set_rx_data_seq(const uint8_t *seq, int len)
{
    int n = (len < MOCK_BUF_MAX) ? len : MOCK_BUF_MAX;
    memcpy(mock_spi.rx_data_seq, seq, (size_t)n);
    mock_spi.rx_data_seq_len   = n;
    mock_spi.rx_data_call_count = 0;
}

/* =========================================================================
 * Implementações das funções HAL (substituem hal_spi.c)
 * ========================================================================= */

uint8_t hal_spi_init(void)
{
    mock_spi.init_call_count++;
    return mock_spi.init_return;
}

uint8_t hal_spi_tx_ready(void)
{
    int idx = mock_spi.tx_ready_call_count++;
    if (idx < mock_spi.tx_ready_seq_len)
        return mock_spi.tx_ready_seq[idx];
    return (uint8_t)mock_spi.tx_ready_default;
}

uint8_t hal_spi_rx_ready(void)
{
    int idx = mock_spi.rx_ready_call_count++;
    if (idx < mock_spi.rx_ready_seq_len)
        return mock_spi.rx_ready_seq[idx];
    return (uint8_t)mock_spi.rx_ready_default;
}

void hal_spi_cs_low(uint8_t cs_pin)
{
    mock_spi.cs_low_call_count++;
    mock_spi.cs_low_last_pin = cs_pin;
}

void hal_spi_cs_high(uint8_t cs_pin)
{
    mock_spi.cs_high_call_count++;
    mock_spi.cs_high_last_pin = cs_pin;
}

void hal_spi_send_byte(uint8_t data)
{
    int idx = mock_spi.send_byte_call_count++;
    if (idx < MOCK_BUF_MAX)
        mock_spi.sent_bytes[idx] = data;
}

uint8_t hal_spi_read_byte(void)
{
    int idx = mock_spi.rx_data_call_count++;
    if (idx < mock_spi.rx_data_seq_len)
        return mock_spi.rx_data_seq[idx];
    return mock_spi.rx_data_default;
}

void hal_spi_prepare_transfer(void)
{
    mock_spi.prepare_call_count++;
}
