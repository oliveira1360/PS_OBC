/**
 * @file mock_hal_i2c.h
 * @brief Mock para hal_i2c — controlo total do comportamento HAL I2C em testes.
 */

#ifndef MOCK_HAL_I2C_H
#define MOCK_HAL_I2C_H

#include <stdint.h>
#include <string.h>

#define MOCK_I2C_SEQ_MAX  256
#define MOCK_I2C_BUF_MAX  128

typedef struct {
    /* --- hal_i2c_bus_free() --- */
    uint8_t bus_free_seq[MOCK_I2C_SEQ_MAX];
    int     bus_free_seq_len;
    int     bus_free_call_count;
    int     bus_free_default; /* 1 = livre por omissão */

    /* --- hal_i2c_get_ack() --- */
    uint8_t ack_seq[MOCK_I2C_SEQ_MAX]; /* 1=ACK, 0=NACK */
    int     ack_seq_len;
    int     ack_call_count;
    int     ack_default; /* 1 = ACK por omissão */

    /* --- hal_i2c_tx_ready() --- */
    uint8_t tx_ready_seq[MOCK_I2C_SEQ_MAX];
    int     tx_ready_seq_len;
    int     tx_ready_call_count;
    int     tx_ready_default; /* 1 = pronto por omissão */

    /* --- hal_i2c_rx_ready() --- */
    uint8_t rx_ready_seq[MOCK_I2C_SEQ_MAX];
    int     rx_ready_seq_len;
    int     rx_ready_call_count;
    int     rx_ready_default;

    /* --- hal_i2c_read_byte() --- */
    uint8_t rx_data_seq[MOCK_I2C_BUF_MAX];
    int     rx_data_seq_len;
    int     rx_data_call_count;
    uint8_t rx_data_default;

    /* Call counts para funções void */
    int start_count;
    int stop_count;
    int send_byte_count;
    int send_addr_count;
    int send_ack_count;
    int send_nack_count;
    int request_byte_count;
    int restart_read_count;
    int bus_recovery_count;
    int init_count;

    /* Argumentos gravados */
    uint8_t sent_bytes[MOCK_I2C_BUF_MAX];
    int     sent_bytes_count;
    uint8_t last_addr;
    uint8_t restart_read_last_addr;

} mock_i2c_t;

extern mock_i2c_t mock_i2c;

void mock_i2c_reset(void);
void mock_i2c_set_bus_free_seq(const uint8_t *seq, int len);
void mock_i2c_set_ack_seq(const uint8_t *seq, int len);
void mock_i2c_set_tx_ready_seq(const uint8_t *seq, int len);
void mock_i2c_set_rx_ready_seq(const uint8_t *seq, int len);
void mock_i2c_set_rx_data_seq(const uint8_t *seq, int len);

#endif /* MOCK_HAL_I2C_H */
