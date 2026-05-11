/**
 * @file test_qspi_driver.c
 * @brief Testes unitários completos do QSPI driver (qspi_driver.c).
 *
 * Cobertura:
 *   Leitura:
 *     - Happy path: vai direto para SEND_COMMAND (sem WRITE_ENABLE), callback(1)
 *     - Dados lidos colocados no buffer
 *     - Ignorada se não está em IDLE
 *     - Timeout no estado READING
 *
 *   Escrita:
 *     - Happy path: WRITE_ENABLE → CHECK_WEL → SEND_COMMAND → WRITING → WAIT_BUSY → IDLE
 *     - WAIT_BUSY faz polling até busy=0
 *     - Timeout em WAIT_BUSY → ERROR → callback(0)
 *     - Ignorada se não está em IDLE
 *
 *   Erase:
 *     - Happy path: WRITE_ENABLE → CHECK_WEL → SEND_COMMAND(erase) → WAIT_BUSY → IDLE
 *     - Ignorada se não está em IDLE
 *
 *   Write Enable (WREN):
 *     - WEL set na 1ª tentativa: avança imediatamente
 *     - WEL não set na 1ª, set na 2ª: retry bem-sucedido
 *     - WEL nunca set após QSPI_RETRY_MAX tentativas → ERROR, error_code=1, callback(0)
 *
 *   Erros:
 *     - error_code correto: 1=WEL, 3=read timeout, 4=write timeout, 5=busy timeout
 *     - Estado ERROR limpa todos os campos e volta a IDLE
 *     - Operação inválida em SEND_COMMAND → error_code=2
 *     - Estado inválido (ex: SEU) → IDLE (proteção default)
 *
 *   Callbacks:
 *     - callback(1) em todos os caminhos de sucesso
 *     - callback(0) em todos os caminhos de erro
 *     - callback NULL não causa crash
 *
 * Nota sobre timeouts (QSPI_TIMEOUT_MAX = 5000):
 *   Pre-setamos h.timeout = QSPI_TIMEOUT_MAX - 1 antes do tick decisivo.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "../framework/test_runner.h"
#include "../mocks/mock_hal_qspi.h"
#include "drivers/qspi_driver.h"

/* W25Q_SR1_WEL definido em hal_qspi.h, mas precisa de ser acessível aqui */
#ifndef W25Q_SR1_WEL
#define W25Q_SR1_WEL 0x02U
#endif

/* =========================================================================
 * Helpers
 * ========================================================================= */

static int cb_result;
static int cb_called;

static void test_callback(int r)
{
    cb_called++;
    cb_result = r;
}

static void init_handle(qspi_handle_t *h)
{
    memset(h, 0, sizeof(*h));
    h->state = QSPI_IDLE;
}

/**
 * @brief Corre o tick até IDLE ou até atingir max iterações.
 */
static void run_to_idle(qspi_handle_t *h, int max_ticks)
{
    for (int i = 0; i < max_ticks && h->state != QSPI_IDLE; i++)
        qspi_tick(h);
}

/* =========================================================================
 * GRUPO 1: Leitura nominal
 * ========================================================================= */

/** Read happy path: SEND_COMMAND → READING → IDLE, callback(1). */
static void test_qspi_read_happy_path(void)
{
    mock_qspi_reset();
    qspi_handle_t h;
    init_handle(&h);

    uint8_t buf[16] = {0};
    cb_called = 0; cb_result = 99;

    qspi_read_async(&h, 0x00001000UL, buf, 16U, test_callback);

    TEST_ASSERT_EQUAL_INT(QSPI_SEND_COMMAND, h.state,
                          "read começa em SEND_COMMAND (sem WRITE_ENABLE)");

    run_to_idle(&h, 20);

    TEST_ASSERT_EQUAL_INT(QSPI_IDLE, h.state, "IDLE após leitura");
    TEST_ASSERT_EQUAL_INT(1, cb_called,        "callback chamado");
    TEST_ASSERT_EQUAL_INT(1, cb_result,        "callback(1) em sucesso");
    TEST_ASSERT_EQUAL_INT(1, mock_qspi.read_memory_count, "hal_qspi_read_memory chamado");
}

/** Read: dados colocados no buffer pelo HAL mock. */
static void test_qspi_read_data_placed_in_buffer(void)
{
    mock_qspi_reset();
    mock_qspi.read_memory_fill_byte = 0xBEU;

    qspi_handle_t h;
    init_handle(&h);

    uint8_t buf[8] = {0};
    qspi_read_async(&h, 0x0000U, buf, 8U, NULL);
    run_to_idle(&h, 20);

    /* O mock preenche o buffer com fill_byte */
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT_EQUAL_UINT8(0xBEU, buf[i], "buf preenchido pelo mock");
    }
}

/** Read ignorada se não está em IDLE. */
static void test_qspi_read_ignored_when_busy(void)
{
    mock_qspi_reset();
    qspi_handle_t h;
    init_handle(&h);
    h.state = QSPI_WAIT_BUSY; /* simulação de ocupado */

    uint8_t buf[4] = {0};
    qspi_read_async(&h, 0x0000U, buf, 4U, test_callback);

    TEST_ASSERT_EQUAL_INT(QSPI_WAIT_BUSY, h.state, "estado não mudou");
}

/** Read timeout no estado READING (h->index < h->len). */
static void test_qspi_read_timeout_in_reading_state(void)
{
    mock_qspi_reset();
    qspi_handle_t h;
    init_handle(&h);
    cb_called = 0; cb_result = 99;

    /* Simula directamente o estado READING com index < len */
    h.state     = QSPI_READING;
    h.operation = QSPI_OP_READ;
    h.len       = 10U;
    h.index     = 0U; /* index < len → timeout path */
    h.callback  = test_callback;
    h.timeout   = QSPI_TIMEOUT_MAX - 1U;

    qspi_tick(&h); /* timeout++ → QSPI_TIMEOUT_MAX → ERROR */

    TEST_ASSERT_EQUAL_INT(QSPI_ERROR, h.state,  "ERROR após read timeout");
    TEST_ASSERT_EQUAL_INT(3U, h.error_code,     "error_code=3 (read timeout)");

    qspi_tick(&h); /* ERROR → IDLE */
    TEST_ASSERT_EQUAL_INT(QSPI_IDLE, h.state,  "IDLE após ERROR");
    TEST_ASSERT_EQUAL_INT(0, cb_result,         "callback(0) em read timeout");
}

/* =========================================================================
 * GRUPO 2: Escrita nominal
 * ========================================================================= */

/** Write happy path: WEL na 1ª tentativa → completo, callback(1). */
static void test_qspi_write_happy_path(void)
{
    mock_qspi_reset();
    /* Status: WEL bit set logo na primeira verificação */
    uint8_t status_seq[] = {W25Q_SR1_WEL};
    mock_qspi_set_status_seq(status_seq, 1);

    qspi_handle_t h;
    init_handle(&h);
    cb_called = 0; cb_result = 99;

    uint8_t buf[] = {0x01U, 0x02U, 0x03U};
    qspi_write_async(&h, 0x00002000UL, buf, 3U, test_callback);

    TEST_ASSERT_EQUAL_INT(QSPI_WRITE_ENABLE, h.state, "começa em WRITE_ENABLE");

    run_to_idle(&h, 20);

    TEST_ASSERT_EQUAL_INT(QSPI_IDLE, h.state, "IDLE após escrita");
    TEST_ASSERT_EQUAL_INT(1, cb_called,        "callback chamado");
    TEST_ASSERT_EQUAL_INT(1, cb_result,        "callback(1) em sucesso");
    TEST_ASSERT_EQUAL_INT(1, mock_qspi.write_memory_count, "write_memory chamado");
    TEST_ASSERT_EQUAL_INT(1, mock_qspi.send_cmd_count,
                          "WREN (send_command) enviado");
}

/** Write: dados enviados correctamente para o HAL. */
static void test_qspi_write_data_forwarded_to_hal(void)
{
    mock_qspi_reset();
    uint8_t status_seq[] = {W25Q_SR1_WEL};
    mock_qspi_set_status_seq(status_seq, 1);

    qspi_handle_t h;
    init_handle(&h);

    uint8_t buf[] = {0xAAU, 0xBBU, 0xCCU};
    qspi_write_async(&h, 0x00003000UL, buf, 3U, NULL);
    run_to_idle(&h, 20);

    TEST_ASSERT_EQUAL_UINT32(0x00003000UL, mock_qspi.write_memory_last_addr,
                             "endereço correto");
    TEST_ASSERT_EQUAL_UINT32(3U, mock_qspi.write_memory_last_len, "len correto");
    TEST_ASSERT_EQUAL_UINT8(0xAAU, mock_qspi.write_memory_last_buf[0], "dado[0]");
    TEST_ASSERT_EQUAL_UINT8(0xBBU, mock_qspi.write_memory_last_buf[1], "dado[1]");
    TEST_ASSERT_EQUAL_UINT8(0xCCU, mock_qspi.write_memory_last_buf[2], "dado[2]");
}

/** Write ignorada se não está em IDLE. */
static void test_qspi_write_ignored_when_busy(void)
{
    mock_qspi_reset();
    qspi_handle_t h;
    init_handle(&h);
    h.state = QSPI_WRITING;

    uint8_t buf[4] = {0};
    qspi_write_async(&h, 0x0000U, buf, 4U, test_callback);

    TEST_ASSERT_EQUAL_INT(QSPI_WRITING, h.state, "estado não mudou");
}

/** WAIT_BUSY faz polling: fica em WAIT_BUSY enquanto busy=1, transita quando busy=0. */
static void test_qspi_write_busy_polling_then_done(void)
{
    mock_qspi_reset();
    uint8_t status_seq[] = {W25Q_SR1_WEL};
    mock_qspi_set_status_seq(status_seq, 1);
    /* busy: 3 vezes ocupado, depois livre */
    uint8_t busy_seq[] = {1, 1, 1, 0};
    mock_qspi_set_busy_seq(busy_seq, 4);

    qspi_handle_t h;
    init_handle(&h);
    cb_called = 0; cb_result = 99;

    uint8_t buf[] = {0xFFU};
    qspi_write_async(&h, 0x0000U, buf, 1U, test_callback);
    run_to_idle(&h, 30);

    TEST_ASSERT_EQUAL_INT(QSPI_IDLE, h.state, "IDLE após busy polling");
    TEST_ASSERT_EQUAL_INT(1, cb_result,        "callback(1) após busy");
    TEST_ASSERT_TRUE(mock_qspi.busy_call_count >= 4, "busy consultado ≥ 4 vezes");
}

/** WAIT_BUSY timeout → ERROR → callback(0), error_code=5. */
static void test_qspi_write_busy_timeout(void)
{
    mock_qspi_reset();
    mock_qspi.busy_default = 1; /* sempre ocupado */

    qspi_handle_t h;
    init_handle(&h);
    h.state     = QSPI_WAIT_BUSY;
    h.operation = QSPI_OP_WRITE;
    h.timeout   = QSPI_TIMEOUT_MAX - 1U;
    h.callback  = test_callback;
    cb_called = 0; cb_result = 99;

    qspi_tick(&h); /* timeout → ERROR */
    TEST_ASSERT_EQUAL_INT(QSPI_ERROR, h.state, "ERROR em busy timeout");
    TEST_ASSERT_EQUAL_INT(5U, h.error_code,    "error_code=5");

    qspi_tick(&h); /* ERROR → IDLE */
    TEST_ASSERT_EQUAL_INT(0, cb_result, "callback(0)");
}

/** Write timeout no estado WRITING. */
static void test_qspi_write_timeout_in_writing_state(void)
{
    mock_qspi_reset();
    qspi_handle_t h;
    init_handle(&h);
    h.state     = QSPI_WRITING;
    h.operation = QSPI_OP_WRITE;
    h.len       = 10U;
    h.index     = 0U; /* index < len → timeout path */
    h.timeout   = QSPI_TIMEOUT_MAX - 1U;
    h.callback  = test_callback;
    cb_called = 0; cb_result = 99;

    qspi_tick(&h);
    TEST_ASSERT_EQUAL_INT(QSPI_ERROR, h.state, "ERROR após write timeout");
    TEST_ASSERT_EQUAL_INT(4U, h.error_code,    "error_code=4 (write timeout)");

    qspi_tick(&h);
    TEST_ASSERT_EQUAL_INT(0, cb_result, "callback(0)");
}

/* =========================================================================
 * GRUPO 3: Erase nominal
 * ========================================================================= */

/** Erase happy path: WEL → SEND_COMMAND(erase) → WAIT_BUSY → IDLE, callback(1). */
static void test_qspi_erase_happy_path(void)
{
    mock_qspi_reset();
    uint8_t status_seq[] = {W25Q_SR1_WEL};
    mock_qspi_set_status_seq(status_seq, 1);

    qspi_handle_t h;
    init_handle(&h);
    cb_called = 0; cb_result = 99;

    qspi_erase_sector_async(&h, 0x00004000UL, test_callback);
    run_to_idle(&h, 20);

    TEST_ASSERT_EQUAL_INT(QSPI_IDLE, h.state, "IDLE após erase");
    TEST_ASSERT_EQUAL_INT(1, cb_result,        "callback(1)");
    /* Verifica que o comando de erase foi enviado com o endereço correto */
    TEST_ASSERT_EQUAL_INT(1, mock_qspi.send_cmd_addr_count, "send_command_addr chamado");
    TEST_ASSERT_EQUAL_UINT8(0x20U, mock_qspi.send_cmd_addr_last_cmd,
                            "comando SECTOR_ERASE (0x20)");
    TEST_ASSERT_EQUAL_UINT32(0x00004000UL, mock_qspi.send_cmd_addr_last_addr,
                             "endereço erase correto");
}

/** Erase ignorado se não está em IDLE. */
static void test_qspi_erase_ignored_when_busy(void)
{
    mock_qspi_reset();
    qspi_handle_t h;
    init_handle(&h);
    h.state = QSPI_CHECK_WEL;

    qspi_erase_sector_async(&h, 0x0000U, test_callback);
    TEST_ASSERT_EQUAL_INT(QSPI_CHECK_WEL, h.state, "estado não mudou");
}

/* =========================================================================
 * GRUPO 4: Write Enable (WREN) e retries
 * ========================================================================= */

/** WEL não set na 1ª tentativa, set na 2ª → retry bem-sucedido. */
static void test_qspi_write_enable_retry_success(void)
{
    mock_qspi_reset();
    /* 1ª verificação: sem WEL; 2ª: com WEL */
    uint8_t status_seq[] = {0x00U, W25Q_SR1_WEL};
    mock_qspi_set_status_seq(status_seq, 2);

    qspi_handle_t h;
    init_handle(&h);
    cb_called = 0; cb_result = 99;

    uint8_t buf[] = {0x42U};
    qspi_write_async(&h, 0x0000U, buf, 1U, test_callback);
    run_to_idle(&h, 30);

    TEST_ASSERT_EQUAL_INT(QSPI_IDLE, h.state, "IDLE após retry WEL");
    TEST_ASSERT_EQUAL_INT(1, cb_result,        "callback(1) em sucesso com retry");
    /* WREN deve ter sido enviado 2 vezes */
    TEST_ASSERT_TRUE(mock_qspi.send_cmd_count >= 2, "WREN enviado ≥ 2 vezes");
}

/** WEL nunca set → ERROR após QSPI_RETRY_MAX, error_code=1, callback(0). */
static void test_qspi_write_enable_max_retry_fails(void)
{
    mock_qspi_reset();
    mock_qspi.status_default = 0x00U; /* WEL nunca set */

    qspi_handle_t h;
    init_handle(&h);
    cb_called = 0; cb_result = 99;

    uint8_t buf[] = {0x00U};
    qspi_write_async(&h, 0x0000U, buf, 1U, test_callback);
    run_to_idle(&h, 30);

    TEST_ASSERT_EQUAL_INT(QSPI_IDLE, h.state, "IDLE após max retry");
    TEST_ASSERT_EQUAL_INT(0, cb_result,        "callback(0) em falha WEL");
    /* error_code já foi limpo pelo estado ERROR → IDLE,
       mas verificamos que o callback foi chamado com 0 */
    TEST_ASSERT_EQUAL_INT(1, cb_called, "callback chamado exatamente 1 vez");
}

/* =========================================================================
 * GRUPO 5: Erros e robustez
 * ========================================================================= */

/** Estado ERROR limpa todos os campos (index, len, timeout, retry, operation). */
static void test_qspi_error_state_clears_all_fields(void)
{
    mock_qspi_reset();
    qspi_handle_t h;
    init_handle(&h);

    h.state      = QSPI_ERROR;
    h.index      = 42U;
    h.len        = 100U;
    h.timeout    = 999U;
    h.retry      = 3U;
    h.operation  = QSPI_OP_WRITE;
    h.error_code = 5U;
    h.callback   = test_callback;
    cb_called = 0; cb_result = 99;

    qspi_tick(&h);

    TEST_ASSERT_EQUAL_INT(QSPI_IDLE,     h.state,     "IDLE após ERROR");
    TEST_ASSERT_EQUAL_INT(0,             h.index,     "index limpo");
    TEST_ASSERT_EQUAL_INT(0,             h.len,       "len limpo");
    TEST_ASSERT_EQUAL_INT(0,             h.timeout,   "timeout limpo");
    TEST_ASSERT_EQUAL_INT(0,             h.retry,     "retry limpo");
    TEST_ASSERT_EQUAL_INT(QSPI_OP_NONE, h.operation, "operation limpo");
    TEST_ASSERT_EQUAL_INT(0, cb_result,              "callback(0)");
}

/** Estado inválido (proteção SEU) → IDLE sem crash. */
static void test_qspi_invalid_state_returns_to_idle(void)
{
    mock_qspi_reset();
    qspi_handle_t h;
    init_handle(&h);
    h.state = (qspi_state_t)0xFFU;

    qspi_tick(&h);
    TEST_ASSERT_EQUAL_INT(QSPI_IDLE, h.state, "DEFAULT → IDLE (protecção SEU)");
}

/** Operação inválida (QSPI_OP_NONE) em SEND_COMMAND → error_code=2. */
static void test_qspi_invalid_operation_in_send_command(void)
{
    mock_qspi_reset();
    qspi_handle_t h;
    init_handle(&h);
    h.state     = QSPI_SEND_COMMAND;
    h.operation = QSPI_OP_NONE;
    h.callback  = test_callback;
    cb_called = 0; cb_result = 99;

    qspi_tick(&h); /* → ERROR (operation desconhecida) */
    TEST_ASSERT_EQUAL_INT(QSPI_ERROR, h.state, "ERROR em operação inválida");
    TEST_ASSERT_EQUAL_INT(2U, h.error_code,    "error_code=2");

    qspi_tick(&h); /* → IDLE */
    TEST_ASSERT_EQUAL_INT(0, cb_result, "callback(0)");
}

/** Callback NULL não causa crash em sucesso (read). */
static void test_qspi_null_callback_no_crash_on_success(void)
{
    mock_qspi_reset();
    qspi_handle_t h;
    init_handle(&h);

    uint8_t buf[4] = {0};
    qspi_read_async(&h, 0x0000U, buf, 4U, NULL);
    run_to_idle(&h, 20);

    TEST_ASSERT_EQUAL_INT(QSPI_IDLE, h.state, "IDLE sem callback");
}

/** Callback NULL não causa crash em erro. */
static void test_qspi_null_callback_no_crash_on_error(void)
{
    mock_qspi_reset();
    qspi_handle_t h;
    init_handle(&h);
    h.state    = QSPI_ERROR;
    h.callback = NULL;

    qspi_tick(&h);
    TEST_ASSERT_EQUAL_INT(QSPI_IDLE, h.state, "IDLE sem callback em erro");
}

/** IDLE permanece em IDLE — tick não muda estado. */
static void test_qspi_idle_tick_is_noop(void)
{
    mock_qspi_reset();
    qspi_handle_t h;
    init_handle(&h);

    qspi_tick(&h);
    qspi_tick(&h);

    TEST_ASSERT_EQUAL_INT(QSPI_IDLE, h.state,         "permanece IDLE");
    TEST_ASSERT_EQUAL_INT(0, mock_qspi.send_cmd_count, "nenhum comando enviado");
}

/** Read: hal_qspi_read_memory chamado com endereço e comprimento correctos. */
static void test_qspi_read_address_and_len_forwarded(void)
{
    mock_qspi_reset();
    qspi_handle_t h;
    init_handle(&h);

    uint8_t buf[32] = {0};
    qspi_read_async(&h, 0x00080000UL, buf, 32U, NULL);
    run_to_idle(&h, 20);

    TEST_ASSERT_EQUAL_UINT32(0x00080000UL, mock_qspi.read_memory_last_addr,
                             "endereço de leitura correto");
    TEST_ASSERT_EQUAL_UINT32(32U, mock_qspi.read_memory_last_len,
                             "comprimento de leitura correto");
}

/* =========================================================================
 * main
 * ========================================================================= */

int main(void)
{
    TEST_BEGIN("QSPI Driver — Sistema Espacial OBC");

    /* Grupo 1: Leitura */
    RUN_TEST(test_qspi_read_happy_path);
    RUN_TEST(test_qspi_read_data_placed_in_buffer);
    RUN_TEST(test_qspi_read_ignored_when_busy);
    RUN_TEST(test_qspi_read_timeout_in_reading_state);

    /* Grupo 2: Escrita */
    RUN_TEST(test_qspi_write_happy_path);
    RUN_TEST(test_qspi_write_data_forwarded_to_hal);
    RUN_TEST(test_qspi_write_ignored_when_busy);
    RUN_TEST(test_qspi_write_busy_polling_then_done);
    RUN_TEST(test_qspi_write_busy_timeout);
    RUN_TEST(test_qspi_write_timeout_in_writing_state);

    /* Grupo 3: Erase */
    RUN_TEST(test_qspi_erase_happy_path);
    RUN_TEST(test_qspi_erase_ignored_when_busy);

    /* Grupo 4: Write Enable */
    RUN_TEST(test_qspi_write_enable_retry_success);
    RUN_TEST(test_qspi_write_enable_max_retry_fails);

    /* Grupo 5: Erros e robustez */
    RUN_TEST(test_qspi_error_state_clears_all_fields);
    RUN_TEST(test_qspi_invalid_state_returns_to_idle);
    RUN_TEST(test_qspi_invalid_operation_in_send_command);
    RUN_TEST(test_qspi_null_callback_no_crash_on_success);
    RUN_TEST(test_qspi_null_callback_no_crash_on_error);
    RUN_TEST(test_qspi_idle_tick_is_noop);
    RUN_TEST(test_qspi_read_address_and_len_forwarded);

    TEST_END();
}
