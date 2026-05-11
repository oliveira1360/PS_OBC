/**
 * @file test_usart_driver.c
 * @brief Testes unitários completos do USART driver (usart_driver.c).
 *
 * Cobertura:
 *   TX:
 *     - Envio nominal de 1 e N bytes em ordem correcta
 *     - Chamada ignorada se já está a transmitir (busy)
 *     - Timeout TX → estado UART_TX_ERROR → volta a IDLE (sem callback)
 *     - Tick em IDLE não envia nada
 *     - Default/estado inválido → IDLE
 *
 *   RX:
 *     - Recepção nominal de 1 e N bytes, callback(1)
 *     - Idle: não entra em RECEIVING se não há dados
 *     - Idle: entra em RECEIVING quando dados disponíveis
 *     - Chamada ignorada se já está a receber (busy)
 *     - Timeout após primeiro byte: callback(0) + volta a IDLE
 *     - Sem timeout se nenhum byte foi recebido (espera infinita segura)
 *     - Estado ERROR: notifica callback(0), limpa estado
 *     - Default/estado inválido → IDLE
 *
 * Nota sobre USART_TIMEOUT_MAX (1 000 000):
 *   Em vez de iterar 1M vezes, pre-setamos h.timeout = USART_TIMEOUT_MAX - 1
 *   antes do tick decisivo.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "../framework/test_runner.h"
#include "../mocks/mock_hal_usart.h"
#include "drivers/usart_driver.h"

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

static void init_tx_handle(usart_handle_t *h)
{
    memset(h, 0, sizeof(*h));
    h->tx_state = UART_TX_IDLE;
    h->rx_state = UART_RX_IDLE;
}

/* =========================================================================
 * GRUPO 1: TX nominal
 * ========================================================================= */

/** Envio de 1 byte: verifica que o byte é escrito no HAL. */
static void test_usart_tx_single_byte(void)
{
    mock_usart_reset();
    usart_handle_t h;
    init_tx_handle(&h);

    uint8_t data[] = {0xA5U};
    usart_send_async(&h, data, 1U);
    usart_tx_tick(&h);

    TEST_ASSERT_EQUAL_INT(UART_TX_IDLE, h.tx_state,    "IDLE após 1 byte");
    TEST_ASSERT_EQUAL_INT(1, mock_usart.write_byte_count, "1 byte escrito");
    TEST_ASSERT_EQUAL_UINT8(0xA5U, mock_usart.written_bytes[0], "byte correto");
}

/** Envio de 4 bytes em ordem correcta. */
static void test_usart_tx_multiple_bytes_in_order(void)
{
    mock_usart_reset();
    usart_handle_t h;
    init_tx_handle(&h);

    uint8_t data[] = {0x01U, 0x02U, 0x03U, 0x04U};
    usart_send_async(&h, data, 4U);

    for (int i = 0; i < 4; i++)
        usart_tx_tick(&h);

    TEST_ASSERT_EQUAL_INT(UART_TX_IDLE, h.tx_state, "IDLE após 4 bytes");
    TEST_ASSERT_EQUAL_INT(4, mock_usart.write_byte_count, "4 bytes escritos");
    TEST_ASSERT_EQUAL_UINT8(0x01U, mock_usart.written_bytes[0], "byte 0");
    TEST_ASSERT_EQUAL_UINT8(0x02U, mock_usart.written_bytes[1], "byte 1");
    TEST_ASSERT_EQUAL_UINT8(0x03U, mock_usart.written_bytes[2], "byte 2");
    TEST_ASSERT_EQUAL_UINT8(0x04U, mock_usart.written_bytes[3], "byte 3");
}

/** Segunda chamada a send_async ignorada enquanto a FSM está ocupada. */
static void test_usart_tx_busy_ignores_new_send(void)
{
    mock_usart_reset();
    /* TX nunca pronto para manter a FSM ocupada */
    mock_usart.tx_ready_default = 0;

    usart_handle_t h;
    init_tx_handle(&h);

    uint8_t data1[] = {0xAAU};
    uint8_t data2[] = {0xBBU};

    usart_send_async(&h, data1, 1U);
    TEST_ASSERT_EQUAL_INT(UART_TX_TRANSMITTING, h.tx_state, "FSM ocupada");

    usart_send_async(&h, data2, 1U); /* deve ser ignorada */
    TEST_ASSERT_EQUAL_INT((uintptr_t)data1, (uintptr_t)h.tx_buf,
                          "tx_buf mantido da 1ª chamada");

    /* Limpa */
    mock_usart.tx_ready_default = 1;
    usart_tx_tick(&h);
}

/** Tick em IDLE não envia nenhum byte. */
static void test_usart_tx_idle_does_nothing(void)
{
    mock_usart_reset();
    usart_handle_t h;
    init_tx_handle(&h);

    usart_tx_tick(&h);
    usart_tx_tick(&h);

    TEST_ASSERT_EQUAL_INT(0, mock_usart.write_byte_count, "nenhum byte enviado");
    TEST_ASSERT_EQUAL_INT(UART_TX_IDLE, h.tx_state, "permanece IDLE");
}

/** Timeout TX: estado vai a ERROR e depois volta a IDLE no tick seguinte. */
static void test_usart_tx_timeout_goes_to_error_then_idle(void)
{
    mock_usart_reset();
    mock_usart.tx_ready_default = 0; /* TX nunca pronto */

    usart_handle_t h;
    init_tx_handle(&h);

    uint8_t data[] = {0xFFU};
    usart_send_async(&h, data, 1U);

    /* Pre-seta timeout para forçar expiração */
    h.timeout = USART_TIMEOUT_MAX - 1U;
    usart_tx_tick(&h); /* timeout++ → USART_TIMEOUT_MAX → ERROR */

    TEST_ASSERT_EQUAL_INT(UART_TX_ERROR, h.tx_state, "estado ERROR após timeout");

    usart_tx_tick(&h); /* ERROR → IDLE */
    TEST_ASSERT_EQUAL_INT(UART_TX_IDLE, h.tx_state, "IDLE após ERROR");
    TEST_ASSERT_EQUAL_INT(0, h.tx_len,              "tx_len limpo");
    TEST_ASSERT_EQUAL_INT(0, h.timeout,             "timeout limpo");
}

/** Default/estado inválido na FSM TX volta a IDLE. */
static void test_usart_tx_default_state_to_idle(void)
{
    mock_usart_reset();
    usart_handle_t h;
    init_tx_handle(&h);
    h.tx_state = (uart_tx_state_t)0xFFU; /* estado inválido */

    usart_tx_tick(&h);
    TEST_ASSERT_EQUAL_INT(UART_TX_IDLE, h.tx_state, "DEFAULT → IDLE");
}

/* =========================================================================
 * GRUPO 2: RX nominal
 * ========================================================================= */

/**
 * Recepção de 1 byte: callback(1) e buf preenchido.
 *
 * Nota de design: usart_recv_async() coloca o handle directamente em
 * UART_RX_RECEIVING (não em IDLE). O tick em RECEIVING com avail=1
 * lê o byte e transita para IDLE. O tick de IDLE verifica avail para
 * entrar em RECEIVING autonomamente (sem usart_recv_async prévia).
 */
static void test_usart_rx_single_byte_success(void)
{
    mock_usart_reset();
    mock_usart.rx_avail_default = 1; /* dado sempre disponível */
    uint8_t rx_data_seq[] = {0x42U};
    mock_usart_set_rx_data_seq(rx_data_seq, 1);

    usart_handle_t h;
    init_tx_handle(&h);

    uint8_t buf[1] = {0};
    cb_called = 0; cb_result = 99;

    usart_recv_async(&h, buf, 1U, test_callback);
    /* Após usart_recv_async: state = RECEIVING (não IDLE) */
    TEST_ASSERT_EQUAL_INT(UART_RX_RECEIVING, h.rx_state, "RECEIVING após recv_async");

    usart_rx_tick(&h); /* RECEIVING: lê byte, index=1 >= len=1 → IDLE + callback */

    TEST_ASSERT_EQUAL_INT(UART_RX_IDLE, h.rx_state, "IDLE após recepção");
    TEST_ASSERT_EQUAL_UINT8(0x42U, buf[0],          "byte recebido correto");
    TEST_ASSERT_EQUAL_INT(1, cb_called,              "callback chamado");
    TEST_ASSERT_EQUAL_INT(1, cb_result,              "callback(1) em sucesso");
}

/** Recepção de 3 bytes em sequência. */
static void test_usart_rx_multiple_bytes_success(void)
{
    mock_usart_reset();
    mock_usart.rx_avail_default = 1;
    uint8_t rx_data[] = {0xAAU, 0xBBU, 0xCCU};
    mock_usart_set_rx_data_seq(rx_data, 3);

    usart_handle_t h;
    init_tx_handle(&h);

    uint8_t buf[3] = {0};
    cb_called = 0;

    usart_recv_async(&h, buf, 3U, test_callback);
    /* 3 ticks: 1 por byte */
    usart_rx_tick(&h);
    usart_rx_tick(&h);
    usart_rx_tick(&h);

    TEST_ASSERT_EQUAL_INT(UART_RX_IDLE, h.rx_state, "IDLE");
    TEST_ASSERT_EQUAL_UINT8(0xAAU, buf[0], "buf[0]");
    TEST_ASSERT_EQUAL_UINT8(0xBBU, buf[1], "buf[1]");
    TEST_ASSERT_EQUAL_UINT8(0xCCU, buf[2], "buf[2]");
    TEST_ASSERT_EQUAL_INT(1, cb_called,    "callback chamado 1 vez");
    TEST_ASSERT_EQUAL_INT(1, cb_result,    "callback(1)");
}

/**
 * Após usart_recv_async, sem dados disponíveis, a FSM permanece em
 * RECEIVING sem avançar (nem completar, nem timeout antes do 1º byte).
 */
static void test_usart_rx_waits_for_data_in_receiving(void)
{
    mock_usart_reset();
    mock_usart.rx_avail_default = 0; /* sem dados */

    usart_handle_t h;
    init_tx_handle(&h);

    uint8_t buf[4] = {0};
    usart_recv_async(&h, buf, 4U, test_callback);

    /* State = RECEIVING, sem dados → fica parado */
    usart_rx_tick(&h);
    usart_rx_tick(&h);
    usart_rx_tick(&h);

    TEST_ASSERT_EQUAL_INT(UART_RX_RECEIVING, h.rx_state,
                          "permanece RECEIVING sem dados (rx_index=0 → sem timeout)");
    TEST_ASSERT_EQUAL_INT(0, h.rx_index, "nenhum byte recebido");
}

/** Segunda chamada a recv_async ignorada se já está a receber. */
static void test_usart_rx_busy_ignores_new_recv(void)
{
    mock_usart_reset();
    mock_usart.rx_avail_default = 0; /* Mantém em RECEIVING sem completar */

    usart_handle_t h;
    init_tx_handle(&h);
    h.rx_state = UART_RX_RECEIVING; /* simula FSM ocupada */
    h.rx_len   = 5U;
    h.rx_index = 2U;

    uint8_t buf[1] = {0};
    usart_recv_async(&h, buf, 1U, NULL); /* deve ser ignorada */

    TEST_ASSERT_EQUAL_INT(UART_RX_RECEIVING, h.rx_state, "estado mantido");
    TEST_ASSERT_EQUAL_INT(5U, h.rx_len,  "rx_len não alterado");
    TEST_ASSERT_EQUAL_INT(2U, h.rx_index,"rx_index não alterado");
}

/** Timeout após receber primeiro byte: callback(0). */
static void test_usart_rx_timeout_after_first_byte(void)
{
    mock_usart_reset();
    /* 1 byte disponível para leitura, depois sem mais dados */
    uint8_t avail_seq[]   = {1, 0}; /* 1 tick lê byte, depois avail=0 */
    uint8_t rx_data_seq[] = {0x55U};
    mock_usart_set_rx_avail_seq(avail_seq, 2);
    mock_usart_set_rx_data_seq(rx_data_seq, 1);
    mock_usart.rx_avail_default = 0;

    usart_handle_t h;
    init_tx_handle(&h);

    uint8_t buf[4] = {0};
    cb_called = 0; cb_result = 99;
    usart_recv_async(&h, buf, 4U, test_callback); /* espera 4 bytes */

    usart_rx_tick(&h); /* IDLE → RECEIVING */
    usart_rx_tick(&h); /* lê 1º byte (0x55), index=1 */

    TEST_ASSERT_EQUAL_INT(1, h.rx_index, "1 byte recebido");

    /* Pre-seta timeout para forçar expiração */
    h.timeout = USART_TIMEOUT_MAX - 1U;
    usart_rx_tick(&h); /* timeout++ → ERROR */

    TEST_ASSERT_EQUAL_INT(UART_RX_ERROR, h.rx_state, "ERROR após timeout");

    usart_rx_tick(&h); /* ERROR → IDLE, chama callback(0) */
    TEST_ASSERT_EQUAL_INT(UART_RX_IDLE, h.rx_state, "IDLE após ERROR");
    TEST_ASSERT_EQUAL_INT(1, cb_called,              "callback chamado");
    TEST_ASSERT_EQUAL_INT(0, cb_result,              "callback(0) em timeout");
}

/** Sem timeout se rx_index=0 (nenhum byte recebido ainda — espera infinita segura). */
static void test_usart_rx_no_timeout_before_first_byte(void)
{
    mock_usart_reset();
    mock_usart.rx_avail_default = 0;

    usart_handle_t h;
    init_tx_handle(&h);
    h.rx_state = UART_RX_RECEIVING;
    h.rx_index = 0U;
    h.rx_len   = 4U;
    h.timeout  = 0U;
    uint8_t buf[4] = {0};
    h.rx_buf   = buf;

    /* Muitos ticks sem dados disponíveis */
    for (int i = 0; i < 100; i++)
        usart_rx_tick(&h);

    /* Não deve ter chegado a ERROR (rx_index ainda 0 → timeout não conta) */
    TEST_ASSERT_EQUAL_INT(UART_RX_RECEIVING, h.rx_state,
                          "permanece RECEIVING sem primeiro byte");
    TEST_ASSERT_EQUAL_INT(0, h.timeout, "timeout não incrementa sem primeiro byte");
}

/** Estado ERROR: chama callback(0), limpa e volta a IDLE. */
static void test_usart_rx_error_state_clears_and_calls_callback(void)
{
    mock_usart_reset();

    usart_handle_t h;
    init_tx_handle(&h);
    h.rx_state  = UART_RX_ERROR;
    cb_called = 0; cb_result = 99;
    h.callback  = test_callback;

    usart_rx_tick(&h);

    TEST_ASSERT_EQUAL_INT(UART_RX_IDLE, h.rx_state, "IDLE após ERROR");
    TEST_ASSERT_EQUAL_INT(1, cb_called,              "callback chamado");
    TEST_ASSERT_EQUAL_INT(0, cb_result,              "callback(0)");
    TEST_ASSERT_EQUAL_INT(0, h.rx_index,             "rx_index limpo");
    TEST_ASSERT_EQUAL_INT(0, h.timeout,              "timeout limpo");
}

/** Default/estado inválido na FSM RX volta a IDLE. */
static void test_usart_rx_default_state_to_idle(void)
{
    mock_usart_reset();
    usart_handle_t h;
    init_tx_handle(&h);
    h.rx_state = (uart_rx_state_t)0xFFU;

    usart_rx_tick(&h);
    TEST_ASSERT_EQUAL_INT(UART_RX_IDLE, h.rx_state, "DEFAULT → IDLE");
}

/** Callback NULL em RX não causa crash em sucesso. */
static void test_usart_rx_null_callback_no_crash(void)
{
    mock_usart_reset();
    mock_usart.rx_avail_default = 1;
    uint8_t rx_data[] = {0x11U};
    mock_usart_set_rx_data_seq(rx_data, 1);

    usart_handle_t h;
    init_tx_handle(&h);

    uint8_t buf[1] = {0};
    usart_recv_async(&h, buf, 1U, NULL);

    for (int i = 0; i < 5; i++)
        usart_rx_tick(&h);

    TEST_ASSERT_EQUAL_INT(UART_RX_IDLE, h.rx_state, "IDLE sem callback");
}

/** Callback NULL em RX não causa crash em timeout/ERROR. */
static void test_usart_rx_null_callback_no_crash_on_error(void)
{
    mock_usart_reset();

    usart_handle_t h;
    init_tx_handle(&h);
    h.rx_state = UART_RX_ERROR;
    h.callback = NULL;

    usart_rx_tick(&h); /* não deve crashar */
    TEST_ASSERT_EQUAL_INT(UART_RX_IDLE, h.rx_state, "IDLE sem callback em ERROR");
}

/* =========================================================================
 * main
 * ========================================================================= */

int main(void)
{
    TEST_BEGIN("USART Driver — Sistema Espacial OBC");

    /* Grupo 1: TX */
    RUN_TEST(test_usart_tx_single_byte);
    RUN_TEST(test_usart_tx_multiple_bytes_in_order);
    RUN_TEST(test_usart_tx_busy_ignores_new_send);
    RUN_TEST(test_usart_tx_idle_does_nothing);
    RUN_TEST(test_usart_tx_timeout_goes_to_error_then_idle);
    RUN_TEST(test_usart_tx_default_state_to_idle);

    /* Grupo 2: RX */
    RUN_TEST(test_usart_rx_single_byte_success);
    RUN_TEST(test_usart_rx_multiple_bytes_success);
    RUN_TEST(test_usart_rx_waits_for_data_in_receiving);
    RUN_TEST(test_usart_rx_busy_ignores_new_recv);
    RUN_TEST(test_usart_rx_timeout_after_first_byte);
    RUN_TEST(test_usart_rx_no_timeout_before_first_byte);
    RUN_TEST(test_usart_rx_error_state_clears_and_calls_callback);
    RUN_TEST(test_usart_rx_default_state_to_idle);
    RUN_TEST(test_usart_rx_null_callback_no_crash);
    RUN_TEST(test_usart_rx_null_callback_no_crash_on_error);

    TEST_END();
}
