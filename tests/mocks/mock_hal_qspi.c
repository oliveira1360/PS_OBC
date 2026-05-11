/**
 * @file mock_hal_qspi.c
 * @brief Implementação do mock HAL QSPI.
 *
 * Substitui hal_qspi.c na compilação dos testes de driver.
 */

#include "mock_hal_qspi.h"
#include <string.h>

mock_qspi_t mock_qspi;

void mock_qspi_reset(void)
{
    memset(&mock_qspi, 0, sizeof(mock_qspi));
    mock_qspi.busy_default        = 0; /* não ocupado por omissão */
    mock_qspi.status_default      = 0x00U;
    mock_qspi.instr_done_default  = 1;
    mock_qspi.init_return         = 1U;
    mock_qspi.read_memory_fill_byte = 0xABU; /* valor reconhecível em testes */
}

void mock_qspi_set_busy_seq(const uint8_t *seq, int len)
{
    int n = (len < MOCK_QSPI_SEQ_MAX) ? len : MOCK_QSPI_SEQ_MAX;
    memcpy(mock_qspi.busy_seq, seq, (size_t)n);
    mock_qspi.busy_seq_len   = n;
    mock_qspi.busy_call_count = 0;
}

void mock_qspi_set_status_seq(const uint8_t *seq, int len)
{
    int n = (len < MOCK_QSPI_SEQ_MAX) ? len : MOCK_QSPI_SEQ_MAX;
    memcpy(mock_qspi.status_seq, seq, (size_t)n);
    mock_qspi.status_seq_len   = n;
    mock_qspi.status_call_count = 0;
}

/* =========================================================================
 * Implementações das funções HAL
 * ========================================================================= */

uint8_t hal_qspi_init(void)
{
    mock_qspi.init_count++;
    return mock_qspi.init_return;
}

void hal_qspi_send_command(uint8_t cmd)
{
    int idx = mock_qspi.send_cmd_count++;
    if (idx < MOCK_QSPI_SEQ_MAX)
        mock_qspi.sent_cmds[idx] = cmd;
}

void hal_qspi_send_command_addr(uint8_t cmd, uint32_t addr)
{
    mock_qspi.send_cmd_addr_count++;
    mock_qspi.send_cmd_addr_last_cmd  = cmd;
    mock_qspi.send_cmd_addr_last_addr = addr;
}

uint8_t hal_qspi_read_status(void)
{
    int idx = mock_qspi.status_call_count++;
    if (idx < mock_qspi.status_seq_len)
        return mock_qspi.status_seq[idx];
    return mock_qspi.status_default;
}

uint8_t hal_qspi_is_busy(void)
{
    int idx = mock_qspi.busy_call_count++;
    if (idx < mock_qspi.busy_seq_len)
        return mock_qspi.busy_seq[idx];
    return (uint8_t)mock_qspi.busy_default;
}

uint8_t hal_qspi_instruction_done(void)
{
    int idx = mock_qspi.instr_done_call_count++;
    if (idx < mock_qspi.instr_done_seq_len)
        return mock_qspi.instr_done_seq[idx];
    return (uint8_t)mock_qspi.instr_done_default;
}

void hal_qspi_read_memory(uint32_t addr, uint8_t *buf, uint32_t len)
{
    mock_qspi.read_memory_count++;
    mock_qspi.read_memory_last_addr = addr;
    mock_qspi.read_memory_last_len  = len;
    /* Preenche o buffer com um valor reconhecível */
    if (buf && len > 0)
        memset(buf, mock_qspi.read_memory_fill_byte, (size_t)len);
}

void hal_qspi_write_memory(uint32_t addr, const uint8_t *buf, uint32_t len)
{
    mock_qspi.write_memory_count++;
    mock_qspi.write_memory_last_addr = addr;
    mock_qspi.write_memory_last_len  = len;
    if (buf && len > 0) {
        uint32_t n = (len < MOCK_QSPI_BUF_MAX) ? len : MOCK_QSPI_BUF_MAX;
        memcpy(mock_qspi.write_memory_last_buf, buf, (size_t)n);
    }
}
