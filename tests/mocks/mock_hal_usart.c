/**
 * @file mock_hal_usart.c
 * @brief Implementação do mock HAL USART.
 *
 * Substitui hal_usart.c na compilação dos testes de driver.
 */

#include "mock_hal_usart.h"

mock_usart_t mock_usart;

void mock_usart_reset(void)
{
    memset(&mock_usart, 0, sizeof(mock_usart));
    mock_usart.tx_ready_default = 1; /* TX pronto por omissão  */
    mock_usart.rx_avail_default = 0; /* sem dados por omissão  */
    mock_usart.rx_data_default  = 0x00U;
}

void mock_usart_set_tx_ready_seq(const uint8_t *seq, int len)
{
    int n = (len < MOCK_USART_SEQ_MAX) ? len : MOCK_USART_SEQ_MAX;
    memcpy(mock_usart.tx_ready_seq, seq, (size_t)n);
    mock_usart.tx_ready_seq_len   = n;
    mock_usart.tx_ready_call_count = 0;
}

void mock_usart_set_rx_avail_seq(const uint8_t *seq, int len)
{
    int n = (len < MOCK_USART_SEQ_MAX) ? len : MOCK_USART_SEQ_MAX;
    memcpy(mock_usart.rx_avail_seq, seq, (size_t)n);
    mock_usart.rx_avail_seq_len   = n;
    mock_usart.rx_avail_call_count = 0;
}

void mock_usart_set_rx_data_seq(const uint8_t *seq, int len)
{
    int n = (len < MOCK_USART_BUF_MAX) ? len : MOCK_USART_BUF_MAX;
    memcpy(mock_usart.rx_data_seq, seq, (size_t)n);
    mock_usart.rx_data_seq_len   = n;
    mock_usart.rx_data_call_count = 0;
}

/* =========================================================================
 * Implementações das funções HAL
 * ========================================================================= */

uint8_t hal_usart_init(void)
{
    mock_usart.init_count++;
    return 1U;
}

uint8_t hal_usart_tx_ready(void)
{
    int idx = mock_usart.tx_ready_call_count++;
    if (idx < mock_usart.tx_ready_seq_len)
        return mock_usart.tx_ready_seq[idx];
    return (uint8_t)mock_usart.tx_ready_default;
}

void hal_usart_write_byte(uint8_t byte)
{
    int idx = mock_usart.write_byte_count++;
    if (idx < MOCK_USART_BUF_MAX)
        mock_usart.written_bytes[idx] = byte;
}

uint8_t hal_usart_read_byte(void)
{
    int idx = mock_usart.rx_data_call_count++;
    if (idx < mock_usart.rx_data_seq_len)
        return mock_usart.rx_data_seq[idx];
    return mock_usart.rx_data_default;
}

uint8_t hal_rx_data_availible(void)
{
    int idx = mock_usart.rx_avail_call_count++;
    if (idx < mock_usart.rx_avail_seq_len)
        return mock_usart.rx_avail_seq[idx];
    return (uint8_t)mock_usart.rx_avail_default;
}
