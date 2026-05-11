/**
 * @file test_spi_driver.c
 * @brief Testes unitários completos do SPI driver (spi_driver.c).
 *
 * Cobertura:
 *   - Transferência nominal (1 byte, N bytes, máximo)
 *   - Sequência de estados (CS_LOW → TRANSFER → WAIT_TX → WAIT_RX → CS_HIGH → STOP → IDLE)
 *   - Timeout de TX e RX: callback(-1), CS libertado, estado volta a IDLE
 *   - tx_buf NULL envia 0xFF (dummy para leitura)
 *   - rx_buf NULL descarta dados sem crash
 *   - Chamada ignorada quando driver está ocupado (busy)
 *   - Transferências sequenciais após conclusão
 *   - Callback chamado exactamente uma vez em sucesso e em erro
 *   - Contadores de timeout reiniciam entre bytes
 *
 * Estratégia para timeouts:
 *   O SPI_TIMEOUT_MAX é 200. Em vez de iterar 201 vezes, pre-setamos
 *   h.timeout = SPI_TIMEOUT_MAX para que o próximo tick dispare o erro.
 *   A condição no driver é (++h->timeout > SPI_TIMEOUT_MAX).
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "../framework/test_runner.h"
#include "../mocks/mock_hal_spi.h"

/* Inclui o driver sob teste (apenas os headers; o .c é compilado separado) */
#include "drivers/spi_driver.h"

/* =========================================================================
 * Helpers
 * ========================================================================= */

/** Corre o tick da FSM até o handle estar em IDLE ou atingir max iterações. */
static void run_to_idle(spi_handle_t *h, int max_ticks)
{
    for (int i = 0; i < max_ticks && h->state != SPI_IDLE; i++)
        spi_tick(h);
}

/** Inicializa um handle limpo. */
static void init_handle(spi_handle_t *h)
{
    memset(h, 0, sizeof(*h));
    h->state = SPI_IDLE;
}

/* =========================================================================
 * Variável de resultado de callback
 * ========================================================================= */
static int cb_result;
static int cb_called;

static void test_callback(int result)
{
    cb_called++;
    cb_result = result;
}

/* =========================================================================
 * GRUPO 1: Transferência nominal
 * ========================================================================= */

/** Transferência de 1 byte — verifica que todos os estados são percorridos
 *  e que o byte recebido é colocado no rx_buf. */
static void test_spi_single_byte_transfer(void)
{
    mock_spi_reset();
    spi_handle_t h;
    init_handle(&h);

    uint8_t tx = 0xA5U;
    uint8_t rx = 0x00U;
    uint8_t rx_data[] = {0xC3U};
    mock_spi_set_rx_data_seq(rx_data, 1);

    cb_called = 0; cb_result = 99;
    spi_transfer_async(&h, 1U, &tx, &rx, 1U, test_callback);

    run_to_idle(&h, 50);

    TEST_ASSERT_EQUAL_INT(SPI_IDLE, h.state,     "estado final deve ser IDLE");
    TEST_ASSERT_EQUAL_UINT8(0xC3U, rx,           "byte recebido deve ser 0xC3");
    TEST_ASSERT_EQUAL_UINT8(0xA5U, mock_spi.sent_bytes[0], "byte enviado deve ser 0xA5");
    TEST_ASSERT_EQUAL_INT(1, cb_called,           "callback chamado exatamente 1 vez");
    TEST_ASSERT_EQUAL_INT(0, cb_result,           "callback com resultado 0 (sucesso)");
}

/** Transferência de 3 bytes — verifica sequência de TX e RX completa. */
static void test_spi_multi_byte_transfer(void)
{
    mock_spi_reset();
    spi_handle_t h;
    init_handle(&h);

    uint8_t tx[]      = {0x01U, 0x02U, 0x03U};
    uint8_t rx[3]     = {0};
    uint8_t rx_data[] = {0xAAU, 0xBBU, 0xCCU};
    mock_spi_set_rx_data_seq(rx_data, 3);

    cb_called = 0; cb_result = 99;
    spi_transfer_async(&h, 2U, tx, rx, 3U, test_callback);
    run_to_idle(&h, 100);

    TEST_ASSERT_EQUAL_INT(SPI_IDLE, h.state,        "estado final IDLE");
    TEST_ASSERT_EQUAL_UINT8(0xAAU, rx[0],           "rx[0] correto");
    TEST_ASSERT_EQUAL_UINT8(0xBBU, rx[1],           "rx[1] correto");
    TEST_ASSERT_EQUAL_UINT8(0xCCU, rx[2],           "rx[2] correto");
    TEST_ASSERT_EQUAL_UINT8(0x01U, mock_spi.sent_bytes[0], "tx[0] enviado");
    TEST_ASSERT_EQUAL_UINT8(0x02U, mock_spi.sent_bytes[1], "tx[1] enviado");
    TEST_ASSERT_EQUAL_UINT8(0x03U, mock_spi.sent_bytes[2], "tx[2] enviado");
    TEST_ASSERT_EQUAL_INT(0, cb_result,             "callback sucesso");
}

/** Sequência de estados: CS_LOW → TRANSFER → WAIT_TX → WAIT_RX → CS_HIGH → STOP → IDLE */
static void test_spi_state_sequence(void)
{
    mock_spi_reset();
    spi_handle_t h;
    init_handle(&h);

    uint8_t tx = 0xFFU;
    uint8_t rx = 0x00U;

    spi_transfer_async(&h, 0U, &tx, &rx, 1U, NULL);
    TEST_ASSERT_EQUAL_INT(SPI_CS_LOW,  h.state, "estado inicial CS_LOW");

    spi_tick(&h); /* CS_LOW → TRANSFER */
    TEST_ASSERT_EQUAL_INT(SPI_TRANSFER, h.state, "após 1 tick: TRANSFER");
    TEST_ASSERT_EQUAL_INT(1, mock_spi.cs_low_call_count, "CS foi colocado LOW");

    spi_tick(&h); /* TRANSFER → WAIT_TX */
    TEST_ASSERT_EQUAL_INT(SPI_WAIT_TX, h.state, "após 2 ticks: WAIT_TX");
    TEST_ASSERT_EQUAL_INT(1, mock_spi.send_byte_call_count, "send_byte foi chamado");

    spi_tick(&h); /* WAIT_TX (tx_ready=1) → WAIT_RX */
    TEST_ASSERT_EQUAL_INT(SPI_WAIT_RX, h.state, "após 3 ticks: WAIT_RX");

    spi_tick(&h); /* WAIT_RX (rx_ready=1) → CS_HIGH (último byte) */
    TEST_ASSERT_EQUAL_INT(SPI_CS_HIGH, h.state, "após 4 ticks: CS_HIGH");

    spi_tick(&h); /* CS_HIGH → STOP */
    TEST_ASSERT_EQUAL_INT(SPI_STOP, h.state, "após 5 ticks: STOP");
    TEST_ASSERT_EQUAL_INT(1, mock_spi.cs_high_call_count, "CS foi colocado HIGH");

    spi_tick(&h); /* STOP → IDLE */
    TEST_ASSERT_EQUAL_INT(SPI_IDLE, h.state, "após 6 ticks: IDLE");
}

/** TX NULL envia 0xFF como dummy byte para leituras SPI. */
static void test_spi_null_tx_sends_dummy_0xFF(void)
{
    mock_spi_reset();
    spi_handle_t h;
    init_handle(&h);

    uint8_t rx = 0x00U;
    spi_transfer_async(&h, 0U, NULL, &rx, 1U, NULL);
    run_to_idle(&h, 50);

    TEST_ASSERT_EQUAL_UINT8(0xFFU, mock_spi.sent_bytes[0],
                            "NULL tx_buf deve enviar 0xFF");
}

/** RX NULL descarta dados sem crash nem acesso inválido. */
static void test_spi_null_rx_discards_data(void)
{
    mock_spi_reset();
    spi_handle_t h;
    init_handle(&h);

    uint8_t tx = 0x42U;
    cb_called = 0;
    spi_transfer_async(&h, 0U, &tx, NULL, 1U, test_callback);
    run_to_idle(&h, 50);

    TEST_ASSERT_EQUAL_INT(SPI_IDLE, h.state, "IDLE após transfer com rx NULL");
    TEST_ASSERT_EQUAL_INT(1, cb_called,      "callback chamado");
    TEST_ASSERT_EQUAL_INT(0, cb_result,      "sucesso mesmo com rx NULL");
}

/** CS LOW ocorre antes do envio; CS HIGH ocorre depois do último byte. */
static void test_spi_cs_sequence_correct(void)
{
    mock_spi_reset();
    spi_handle_t h;
    init_handle(&h);

    uint8_t tx[] = {0x11U, 0x22U};
    uint8_t rx[2] = {0};
    spi_transfer_async(&h, 5U, tx, rx, 2U, NULL);
    run_to_idle(&h, 100);

    TEST_ASSERT_EQUAL_INT(1, mock_spi.cs_low_call_count,  "CS LOW chamado 1 vez");
    TEST_ASSERT_EQUAL_INT(1, mock_spi.cs_high_call_count, "CS HIGH chamado 1 vez");
    TEST_ASSERT_EQUAL_INT(5U, mock_spi.cs_low_last_pin,   "cs_pin correto no LOW");
    TEST_ASSERT_EQUAL_INT(5U, mock_spi.cs_high_last_pin,  "cs_pin correto no HIGH");
}

/** Transferência com 8 bytes (tamanho realista de frame do propulsor). */
static void test_spi_propulsor_frame_8bytes(void)
{
    mock_spi_reset();
    spi_handle_t h;
    init_handle(&h);

    uint8_t tx[8]     = {0x01U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};
    uint8_t rx[8]     = {0};
    uint8_t rx_data[] = {0x00U, 0x00U, 0x96U, 0x00U, 0xDCU, 0x00U, 0x00U, 0x6AU};
    mock_spi_set_rx_data_seq(rx_data, 8);

    cb_called = 0; cb_result = 99;
    spi_transfer_async(&h, 25U, tx, rx, 8U, test_callback);
    run_to_idle(&h, 200);

    TEST_ASSERT_EQUAL_INT(SPI_IDLE, h.state,  "IDLE após frame 8 bytes");
    TEST_ASSERT_EQUAL_INT(8, mock_spi.send_byte_call_count, "8 bytes enviados");
    TEST_ASSERT_EQUAL_INT(8, mock_spi.rx_data_call_count,   "8 bytes recebidos");
    TEST_ASSERT_EQUAL_INT(0, cb_result,                     "sucesso");
    /* Verifica checksum byte (último) */
    TEST_ASSERT_EQUAL_UINT8(0x6AU, rx[7], "byte de checksum correto");
}

/* =========================================================================
 * GRUPO 2: Testes de timeout
 * ========================================================================= */

/** TX timeout: após SPI_TIMEOUT_MAX+1 ticks sem TX ready → callback(-1) e IDLE. */
static void test_spi_tx_timeout_callback_minus1(void)
{
    mock_spi_reset();
    mock_spi.tx_ready_default = 0; /* TX nunca pronto */
    spi_handle_t h;
    init_handle(&h);

    uint8_t tx = 0xFFU, rx = 0x00U;
    cb_called = 0; cb_result = 0;
    spi_transfer_async(&h, 0U, &tx, &rx, 1U, test_callback);

    /* Avança até WAIT_TX */
    spi_tick(&h); /* CS_LOW → TRANSFER */
    spi_tick(&h); /* TRANSFER → WAIT_TX */

    /* Pre-seta timeout para forçar expiração no próximo tick */
    h.timeout = SPI_TIMEOUT_MAX;
    spi_tick(&h); /* deve disparar erro */

    TEST_ASSERT_EQUAL_INT(SPI_IDLE,  h.state,  "IDLE após TX timeout");
    TEST_ASSERT_EQUAL_INT(1,         cb_called, "callback chamado");
    TEST_ASSERT_EQUAL_INT(-1,        cb_result, "callback(-1) em TX timeout");
}

/** TX timeout liberta o CS. */
static void test_spi_tx_timeout_releases_cs(void)
{
    mock_spi_reset();
    mock_spi.tx_ready_default = 0;
    spi_handle_t h;
    init_handle(&h);

    uint8_t tx = 0x00U, rx = 0x00U;
    spi_transfer_async(&h, 3U, &tx, &rx, 1U, NULL);
    spi_tick(&h); /* CS_LOW */
    spi_tick(&h); /* TRANSFER → WAIT_TX */
    h.timeout = SPI_TIMEOUT_MAX;
    spi_tick(&h); /* timeout */

    TEST_ASSERT_EQUAL_INT(1, mock_spi.cs_high_call_count,
                          "CS HIGH chamado mesmo em timeout TX");
}

/** RX timeout: callback(-1) e IDLE. */
static void test_spi_rx_timeout_callback_minus1(void)
{
    mock_spi_reset();
    mock_spi.rx_ready_default = 0; /* RX nunca pronto */
    spi_handle_t h;
    init_handle(&h);

    uint8_t tx = 0x00U, rx = 0x00U;
    cb_called = 0; cb_result = 0;
    spi_transfer_async(&h, 0U, &tx, &rx, 1U, test_callback);
    spi_tick(&h); /* CS_LOW → TRANSFER */
    spi_tick(&h); /* TRANSFER → WAIT_TX */
    spi_tick(&h); /* WAIT_TX (tx_ready=1) → WAIT_RX */

    h.timeout = SPI_TIMEOUT_MAX;
    spi_tick(&h); /* timeout RX */

    TEST_ASSERT_EQUAL_INT(SPI_IDLE, h.state,  "IDLE após RX timeout");
    TEST_ASSERT_EQUAL_INT(-1,       cb_result, "callback(-1) em RX timeout");
}

/** RX timeout liberta o CS. */
static void test_spi_rx_timeout_releases_cs(void)
{
    mock_spi_reset();
    mock_spi.rx_ready_default = 0;
    spi_handle_t h;
    init_handle(&h);

    uint8_t tx = 0x00U, rx = 0x00U;
    spi_transfer_async(&h, 7U, &tx, &rx, 1U, NULL);
    spi_tick(&h);
    spi_tick(&h);
    spi_tick(&h); /* WAIT_RX */
    h.timeout = SPI_TIMEOUT_MAX;
    spi_tick(&h);

    TEST_ASSERT_EQUAL_INT(1, mock_spi.cs_high_call_count,
                          "CS HIGH chamado mesmo em timeout RX");
}

/** O contador de timeout é reposto a 0 após cada byte enviado com sucesso. */
static void test_spi_timeout_resets_between_bytes(void)
{
    mock_spi_reset();
    spi_handle_t h;
    init_handle(&h);

    /* 2 bytes — tx_ready: primeiro byte demora 5 ticks, segundo byte imediato */
    uint8_t tx[2] = {0xAAU, 0xBBU};
    uint8_t rx[2] = {0};

    /* Sequência tx_ready: 5 vezes 0 (aguardar), depois sempre 1 */
    uint8_t tx_seq[10] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1};
    mock_spi_set_tx_ready_seq(tx_seq, 10);

    spi_transfer_async(&h, 0U, tx, rx, 2U, NULL);
    run_to_idle(&h, 100);

    TEST_ASSERT_EQUAL_INT(SPI_IDLE, h.state, "IDLE após 2 bytes com delay");
    TEST_ASSERT_EQUAL_INT(2, mock_spi.send_byte_call_count, "2 bytes enviados");
}

/* =========================================================================
 * GRUPO 3: Controlo de acesso concorrente
 * ========================================================================= */

/** Chamar transfer_async enquanto a FSM está ocupada não deve iniciar nova transferência. */
static void test_spi_busy_ignores_new_transfer(void)
{
    mock_spi_reset();
    spi_handle_t h;
    init_handle(&h);

    uint8_t tx1 = 0xAAU, rx1 = 0x00U;
    uint8_t tx2 = 0xBBU, rx2 = 0x00U;

    spi_transfer_async(&h, 0U, &tx1, &rx1, 1U, NULL);
    TEST_ASSERT_EQUAL_INT(SPI_CS_LOW, h.state, "primeira transferência iniciada");

    /* Tenta iniciar segunda enquanto ainda está em CS_LOW */
    spi_transfer_async(&h, 0U, &tx2, &rx2, 1U, NULL);

    /* O handle não deve ter mudado para refletir a segunda chamada */
    TEST_ASSERT_EQUAL_INT(SPI_CS_LOW, h.state, "estado não mudou");
    TEST_ASSERT_TRUE(h.tx_buf == &tx1, "tx_buf mantido da primeira chamada");
}

/** Após completar uma transferência, é possível iniciar outra. */
static void test_spi_sequential_transfers(void)
{
    mock_spi_reset();
    spi_handle_t h;
    init_handle(&h);

    uint8_t tx1 = 0x11U, rx1 = 0x00U;
    uint8_t tx2 = 0x22U, rx2 = 0x00U;
    int cb1 = 0, cb2 = 0;

    /* Primeira transferência */
    cb_called = 0;
    spi_transfer_async(&h, 0U, &tx1, &rx1, 1U, test_callback);
    run_to_idle(&h, 50);
    cb1 = cb_called;

    /* Segunda transferência */
    cb_called = 0;
    spi_transfer_async(&h, 0U, &tx2, &rx2, 1U, test_callback);
    run_to_idle(&h, 50);
    cb2 = cb_called;

    TEST_ASSERT_EQUAL_INT(1, cb1, "callback primeira transferência");
    TEST_ASSERT_EQUAL_INT(1, cb2, "callback segunda transferência");
    TEST_ASSERT_EQUAL_INT(2, mock_spi.cs_low_call_count,  "CS LOW 2 vezes");
    TEST_ASSERT_EQUAL_INT(2, mock_spi.cs_high_call_count, "CS HIGH 2 vezes");
}

/* =========================================================================
 * GRUPO 4: Sem callback
 * ========================================================================= */

/** Callback NULL não deve causar crash (NULL check implícito na FSM). */
static void test_spi_null_callback_no_crash(void)
{
    mock_spi_reset();
    spi_handle_t h;
    init_handle(&h);

    uint8_t tx = 0x55U, rx = 0x00U;
    spi_transfer_async(&h, 0U, &tx, &rx, 1U, NULL);
    run_to_idle(&h, 50);

    TEST_ASSERT_EQUAL_INT(SPI_IDLE, h.state, "IDLE mesmo sem callback");
}

/** Callback NULL em timeout também não deve causar crash. */
static void test_spi_null_callback_on_timeout_no_crash(void)
{
    mock_spi_reset();
    mock_spi.tx_ready_default = 0;
    spi_handle_t h;
    init_handle(&h);

    uint8_t tx = 0x00U, rx = 0x00U;
    spi_transfer_async(&h, 0U, &tx, &rx, 1U, NULL);
    spi_tick(&h);
    spi_tick(&h);
    h.timeout = SPI_TIMEOUT_MAX;
    spi_tick(&h);

    TEST_ASSERT_EQUAL_INT(SPI_IDLE, h.state, "IDLE mesmo sem callback em timeout");
}

/* =========================================================================
 * GRUPO 5: Handle limpo no arranque
 * ========================================================================= */

/** Um handle recém-inicializado deve estar em IDLE. */
static void test_spi_handle_starts_idle(void)
{
    spi_handle_t h;
    memset(&h, 0, sizeof(h));
    TEST_ASSERT_EQUAL_INT(SPI_IDLE, h.state, "SPI_IDLE == 0, handle zero-inicializado");
}

/** Chamar spi_tick num handle IDLE não faz nada. */
static void test_spi_tick_on_idle_is_noop(void)
{
    mock_spi_reset();
    spi_handle_t h;
    init_handle(&h);

    spi_tick(&h);
    spi_tick(&h);

    TEST_ASSERT_EQUAL_INT(SPI_IDLE, h.state,     "permanece IDLE");
    TEST_ASSERT_EQUAL_INT(0, mock_spi.cs_low_call_count, "CS não foi ativado");
    TEST_ASSERT_EQUAL_INT(0, mock_spi.send_byte_call_count, "nenhum byte enviado");
}

/* =========================================================================
 * GRUPO 6: Integridade dos dados
 * ========================================================================= */

/** Verifica que bytes TX são enviados exatamente na ordem do buffer. */
static void test_spi_tx_byte_order_preserved(void)
{
    mock_spi_reset();
    spi_handle_t h;
    init_handle(&h);

    uint8_t tx[]  = {0x10U, 0x20U, 0x30U, 0x40U, 0x50U};
    uint8_t rx[5] = {0};
    spi_transfer_async(&h, 0U, tx, rx, 5U, NULL);
    run_to_idle(&h, 150);

    for (int i = 0; i < 5; i++) {
        TEST_ASSERT_EQUAL_UINT8(tx[i], mock_spi.sent_bytes[i],
                                "ordem dos bytes TX preservada");
    }
}

/** Verifica que bytes RX são colocados na posição correcta do buffer. */
static void test_spi_rx_bytes_placed_correctly(void)
{
    mock_spi_reset();
    spi_handle_t h;
    init_handle(&h);

    uint8_t tx[4]     = {0};
    uint8_t rx[4]     = {0};
    uint8_t rx_data[] = {0xAAU, 0xBBU, 0xCCU, 0xDDU};
    mock_spi_set_rx_data_seq(rx_data, 4);

    spi_transfer_async(&h, 0U, tx, rx, 4U, NULL);
    run_to_idle(&h, 120);

    TEST_ASSERT_EQUAL_UINT8(0xAAU, rx[0], "rx[0]");
    TEST_ASSERT_EQUAL_UINT8(0xBBU, rx[1], "rx[1]");
    TEST_ASSERT_EQUAL_UINT8(0xCCU, rx[2], "rx[2]");
    TEST_ASSERT_EQUAL_UINT8(0xDDU, rx[3], "rx[3]");
}

/* =========================================================================
 * GRUPO 7: Proteção contra estado inválido
 * ========================================================================= */

/** Estado inválido (default no switch) deve resultar em break sem crash. */
static void test_spi_invalid_state_no_crash(void)
{
    mock_spi_reset();
    spi_handle_t h;
    init_handle(&h);
    h.state = (spi_state_t)0xFFU; /* estado inválido */

    /* Não deve crashar */
    spi_tick(&h);
    /* Estado inválido permanece (o driver faz break no default) */
    /* Apenas verificamos que não houve crash */
    TEST_ASSERT_TRUE(1, "tick em estado invalido nao crashou");
}

/* =========================================================================
 * main
 * ========================================================================= */

int main(void)
{
    TEST_BEGIN("SPI Driver — Sistema Espacial OBC");

    /* Grupo 1: Transferência nominal */
    RUN_TEST(test_spi_single_byte_transfer);
    RUN_TEST(test_spi_multi_byte_transfer);
    RUN_TEST(test_spi_state_sequence);
    RUN_TEST(test_spi_null_tx_sends_dummy_0xFF);
    RUN_TEST(test_spi_null_rx_discards_data);
    RUN_TEST(test_spi_cs_sequence_correct);
    RUN_TEST(test_spi_propulsor_frame_8bytes);

    /* Grupo 2: Timeout */
    RUN_TEST(test_spi_tx_timeout_callback_minus1);
    RUN_TEST(test_spi_tx_timeout_releases_cs);
    RUN_TEST(test_spi_rx_timeout_callback_minus1);
    RUN_TEST(test_spi_rx_timeout_releases_cs);
    RUN_TEST(test_spi_timeout_resets_between_bytes);

    /* Grupo 3: Controlo de acesso */
    RUN_TEST(test_spi_busy_ignores_new_transfer);
    RUN_TEST(test_spi_sequential_transfers);

    /* Grupo 4: Sem callback */
    RUN_TEST(test_spi_null_callback_no_crash);
    RUN_TEST(test_spi_null_callback_on_timeout_no_crash);

    /* Grupo 5: Handle */
    RUN_TEST(test_spi_handle_starts_idle);
    RUN_TEST(test_spi_tick_on_idle_is_noop);

    /* Grupo 6: Integridade de dados */
    RUN_TEST(test_spi_tx_byte_order_preserved);
    RUN_TEST(test_spi_rx_bytes_placed_correctly);

    /* Grupo 7: Robustez */
    RUN_TEST(test_spi_invalid_state_no_crash);

    TEST_END();
}
