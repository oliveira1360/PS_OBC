/**
 * @file mock_hal_qspi.h
 * @brief Mock para hal_qspi — controlo total do comportamento HAL QSPI em testes.
 */

#ifndef MOCK_HAL_QSPI_H
#define MOCK_HAL_QSPI_H

#include <stdint.h>
#include <string.h>

#define MOCK_QSPI_SEQ_MAX  256
#define MOCK_QSPI_BUF_MAX  256

typedef struct {
    /* --- hal_qspi_is_busy() --- */
    uint8_t busy_seq[MOCK_QSPI_SEQ_MAX]; /* 1=busy, 0=idle */
    int     busy_seq_len;
    int     busy_call_count;
    int     busy_default; /* 0 = não ocupado por omissão */

    /* --- hal_qspi_read_status() --- */
    uint8_t status_seq[MOCK_QSPI_SEQ_MAX]; /* valor do status register */
    int     status_seq_len;
    int     status_call_count;
    uint8_t status_default; /* 0x00 por omissão */

    /* --- hal_qspi_instruction_done() --- */
    uint8_t instr_done_seq[MOCK_QSPI_SEQ_MAX];
    int     instr_done_seq_len;
    int     instr_done_call_count;
    int     instr_done_default;

    /* --- hal_qspi_read_memory() --- */
    int      read_memory_count;
    uint32_t read_memory_last_addr;
    uint32_t read_memory_last_len;
    uint8_t  read_memory_fill_byte; /* byte com que preenche o buf */

    /* --- hal_qspi_write_memory() --- */
    int      write_memory_count;
    uint32_t write_memory_last_addr;
    uint32_t write_memory_last_len;
    uint8_t  write_memory_last_buf[MOCK_QSPI_BUF_MAX];

    /* --- hal_qspi_send_command() --- */
    int     send_cmd_count;
    uint8_t sent_cmds[MOCK_QSPI_SEQ_MAX];

    /* --- hal_qspi_send_command_addr() --- */
    int      send_cmd_addr_count;
    uint8_t  send_cmd_addr_last_cmd;
    uint32_t send_cmd_addr_last_addr;

    /* --- hal_qspi_init() --- */
    int      init_count;
    uint8_t  init_return;

} mock_qspi_t;

extern mock_qspi_t mock_qspi;

void mock_qspi_reset(void);
void mock_qspi_set_busy_seq(const uint8_t *seq, int len);
void mock_qspi_set_status_seq(const uint8_t *seq, int len);

#endif /* MOCK_HAL_QSPI_H */
