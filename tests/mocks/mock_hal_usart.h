/**
 * @file mock_hal_usart.h
 * @brief Mock para hal_usart — controlo total do comportamento HAL USART em testes.
 */

#ifndef MOCK_HAL_USART_H
#define MOCK_HAL_USART_H

#include <stdint.h>
#include <string.h>

#define MOCK_USART_SEQ_MAX  256
#define MOCK_USART_BUF_MAX  256

typedef struct {
    /* --- hal_usart_tx_ready() --- */
    uint8_t tx_ready_seq[MOCK_USART_SEQ_MAX];
    int     tx_ready_seq_len;
    int     tx_ready_call_count;
    int     tx_ready_default; /* 1 = pronto por omissão */

    /* --- hal_rx_data_availible() --- */
    uint8_t rx_avail_seq[MOCK_USART_SEQ_MAX];
    int     rx_avail_seq_len;
    int     rx_avail_call_count;
    int     rx_avail_default; /* 0 = sem dados por omissão */

    /* --- hal_usart_read_byte() --- */
    uint8_t rx_data_seq[MOCK_USART_BUF_MAX];
    int     rx_data_seq_len;
    int     rx_data_call_count;
    uint8_t rx_data_default;

    /* --- hal_usart_write_byte() --- */
    int     write_byte_count;
    uint8_t written_bytes[MOCK_USART_BUF_MAX];

    /* --- hal_usart_init() --- */
    int     init_count;

} mock_usart_t;

extern mock_usart_t mock_usart;

void mock_usart_reset(void);
void mock_usart_set_tx_ready_seq(const uint8_t *seq, int len);
void mock_usart_set_rx_avail_seq(const uint8_t *seq, int len);
void mock_usart_set_rx_data_seq(const uint8_t *seq, int len);

#endif /* MOCK_HAL_USART_H */
