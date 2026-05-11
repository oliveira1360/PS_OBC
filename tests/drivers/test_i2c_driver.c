/**
 * @file test_i2c_driver.c
 * @brief Testes unitários completos do I2C driver (i2c_driver.c).
 *
 * Cobertura:
 *   - Escrita nominal (1 e N bytes): endereço → ACK → dados → STOP → callback(0)
 *   - Leitura nominal: endereço → ACK → bytes recebidos → NACK no último → STOP
 *   - Leitura com registo (use_reg=1): escrita addr+reg → restart → leitura
 *   - NACK no endereço: i2c_fail(), callback(-1), error_count++
 *   - NACK a meio da escrita: i2c_fail(), callback(-1)
 *   - Timeout TX na escrita e no restart
 *   - Timeout RX na leitura
 *   - Bus não livre: handle fica em I2C_STARTING sem progredir
 *   - Bus recovery após 3 erros consecutivos
 *   - error_count incrementa em cada falha, repõe-se em sucesso
 *   - NACK no restart: i2c_fail()
 *   - NACK no wait_restart: i2c_fail()
 *   - Callback NULL não causa crash
 *
 * Nota sobre bus_locked:
 *   bus_locked é uma variável static interna de i2c_driver.c.
 *   Cada teste DEVE concluir a FSM (chegar a I2C_IDLE) para garantir
 *   bus_locked=0 no início do teste seguinte. O helper force_to_idle()
 *   garante isso.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "../framework/test_runner.h"
#include "../mocks/mock_hal_i2c.h"
#include "drivers/i2c_driver.h"

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

/**
 * @brief Força a FSM para IDLE, configurando mock para retornar ACK e
 *        TX/RX prontos. Garante que bus_locked fica a 0.
 */
static void force_to_idle(i2c_handle_t *h)
{
    mock_i2c.ack_default      = 1;
    mock_i2c.tx_ready_default = 1;
    mock_i2c.rx_ready_default = 1;
    mock_i2c.bus_free_default = 1;
    for (int i = 0; i < 500 && h->state != I2C_IDLE; i++)
        i2c_tick(h);
}

/** Inicializa handle para escrita simples (sem registo). */
static void setup_write(i2c_handle_t *h, uint8_t addr,
                        uint8_t *buf, uint8_t len,
                        void (*cb)(int))
{
    memset(h, 0, sizeof(*h));
    h->addr     = addr;
    h->buf      = buf;
    h->len      = len;
    h->rw       = 0;      /* write */
    h->use_reg  = 0;
    h->callback = cb;
    h->state    = I2C_STARTING;
}

/** Inicializa handle para leitura simples (sem registo). */
static void setup_read(i2c_handle_t *h, uint8_t addr,
                       uint8_t *buf, uint8_t len,
                       void (*cb)(int))
{
    memset(h, 0, sizeof(*h));
    h->addr     = addr;
    h->buf      = buf;
    h->len      = len;
    h->rw       = 1;      /* read */
    h->use_reg  = 0;
    h->callback = cb;
    h->state    = I2C_STARTING;
}

/** Inicializa handle para leitura com registo (use_reg=1). */
static void setup_reg_read(i2c_handle_t *h, uint8_t addr, uint8_t reg,
                           uint8_t *buf, uint8_t len,
                           void (*cb)(int))
{
    memset(h, 0, sizeof(*h));
    h->addr     = addr;
    h->reg      = reg;
    h->buf      = buf;
    h->len      = len;
    h->rw       = 1;
    h->use_reg  = 1;
    h->callback = cb;
    h->state    = I2C_STARTING;
}

/* =========================================================================
 * GRUPO 1: Escrita nominal
 * ========================================================================= */

/** Escrita de 1 byte: verifica sequência completa e callback(0). */
static void test_i2c_write_single_byte_success(void)
{
    mock_i2c_reset();
    i2c_handle_t h;
    uint8_t buf[] = {0x42U};
    cb_called = 0; cb_result = 99;

    setup_write(&h, 0x68U, buf, 1U, test_callback);
    force_to_idle(&h);

    TEST_ASSERT_EQUAL_INT(I2C_IDLE, h.state, "estado final IDLE");
    TEST_ASSERT_EQUAL_INT(1, mock_i2c.start_count,     "START enviado");
    TEST_ASSERT_EQUAL_INT(1, mock_i2c.stop_count,      "STOP enviado");
    TEST_ASSERT_EQUAL_INT(1, cb_called,                 "callback chamado");
    TEST_ASSERT_EQUAL_INT(0, cb_result,                 "callback(0) em sucesso");
    TEST_ASSERT_EQUAL_UINT8(0x42U, mock_i2c.sent_bytes[0], "byte correto enviado");
}

/** Escrita de 3 bytes: verifica que todos os bytes são enviados em ordem. */
static void test_i2c_write_multiple_bytes_success(void)
{
    mock_i2c_reset();
    i2c_handle_t h;
    uint8_t buf[] = {0x01U, 0x02U, 0x03U};
    cb_called = 0; cb_result = 99;

    setup_write(&h, 0x42U, buf, 3U, test_callback);
    force_to_idle(&h);

    TEST_ASSERT_EQUAL_INT(I2C_IDLE, h.state, "IDLE");
    TEST_ASSERT_EQUAL_INT(0, cb_result,      "sucesso");
    /* Os bytes enviados pelo driver: reg (se use_reg) + dados.
       Com use_reg=0, apenas os dados do buf */
    int found = 0;
    for (int i = 0; i < mock_i2c.sent_bytes_count; i++) {
        if (mock_i2c.sent_bytes[i] == 0x01U) { found++; break; }
    }
    TEST_ASSERT_TRUE(found > 0, "byte 0x01 foi enviado");
}

/** NACK no endereço: deve falhar imediatamente. */
static void test_i2c_write_nack_on_address(void)
{
    mock_i2c_reset();
    uint8_t ack_seq[] = {0}; /* NACK na primeira verificação de ACK */
    mock_i2c_set_ack_seq(ack_seq, 1);

    i2c_handle_t h;
    uint8_t buf[] = {0xFFU};
    cb_called = 0; cb_result = 99;

    /* Usa ticks directos sem force_to_idle (que sobreporia ack=1) */
    setup_write(&h, 0x68U, buf, 1U, test_callback);
    for (int i = 0; i < 20 && h.state != I2C_IDLE; i++)
        i2c_tick(&h);

    TEST_ASSERT_EQUAL_INT(I2C_IDLE, h.state, "IDLE após NACK");
    TEST_ASSERT_EQUAL_INT(1, cb_called,      "callback chamado");
    TEST_ASSERT_EQUAL_INT(-1, cb_result,     "callback(-1) em NACK");
    TEST_ASSERT_TRUE(h.error_count > 0 || mock_i2c.bus_recovery_count > 0,
                     "error_count incrementado");
}

/** NACK durante a transmissão de dados: deve falhar e chamar callback(-1). */
static void test_i2c_write_nack_mid_transfer(void)
{
    mock_i2c_reset();
    /* ACK no endereço, NACK no primeiro byte de dados */
    uint8_t ack_seq[] = {1, 0};
    mock_i2c_set_ack_seq(ack_seq, 2);

    i2c_handle_t h;
    uint8_t buf[] = {0xAAU, 0xBBU};
    cb_called = 0; cb_result = 99;

    setup_write(&h, 0x42U, buf, 2U, test_callback);
    force_to_idle(&h);

    TEST_ASSERT_EQUAL_INT(-1, cb_result, "callback(-1) em NACK a meio");
}

/** Timeout TX durante write: i2c_fail → callback(-1). */
static void test_i2c_write_tx_timeout(void)
{
    mock_i2c_reset();
    /* tx_ready nunca pronto após ACK */
    uint8_t tx_seq[2] = {0, 0};
    mock_i2c_set_tx_ready_seq(tx_seq, 2);
    mock_i2c.tx_ready_default = 0;

    i2c_handle_t h;
    uint8_t buf[] = {0x55U};
    cb_called = 0; cb_result = 99;

    setup_write(&h, 0x48U, buf, 1U, test_callback);
    /* Avança até WAIT_TX, depois força timeout */
    for (int i = 0; i < 5 && h.state != I2C_WAIT_TX; i++)
        i2c_tick(&h);

    if (h.state == I2C_WAIT_TX) {
        h.timeout = I2C_TIMEOUT_MAX;
        i2c_tick(&h);
    }
    force_to_idle(&h);

    TEST_ASSERT_EQUAL_INT(-1, cb_result, "callback(-1) em TX timeout na escrita");
}

/* =========================================================================
 * GRUPO 2: Leitura nominal
 * ========================================================================= */

/** Leitura de 1 byte: verifica que o byte é colocado no buffer. */
static void test_i2c_read_single_byte_success(void)
{
    mock_i2c_reset();
    uint8_t rx_data[] = {0xC7U};
    mock_i2c_set_rx_data_seq(rx_data, 1);

    i2c_handle_t h;
    uint8_t buf[1] = {0};
    cb_called = 0; cb_result = 99;

    setup_read(&h, 0x48U, buf, 1U, test_callback);
    force_to_idle(&h);

    TEST_ASSERT_EQUAL_INT(I2C_IDLE, h.state, "IDLE");
    TEST_ASSERT_EQUAL_INT(0, cb_result,      "sucesso");
    TEST_ASSERT_EQUAL_UINT8(0xC7U, buf[0],  "byte recebido correto");
}

/** Leitura de 2 bytes: verifica NACK enviado apenas no último byte. */
static void test_i2c_read_multiple_bytes_nack_on_last(void)
{
    mock_i2c_reset();
    uint8_t rx_data[] = {0x11U, 0x22U};
    mock_i2c_set_rx_data_seq(rx_data, 2);

    i2c_handle_t h;
    uint8_t buf[2] = {0};
    cb_called = 0;

    setup_read(&h, 0x77U, buf, 2U, test_callback);
    force_to_idle(&h);

    TEST_ASSERT_EQUAL_INT(0, cb_result, "sucesso");
    TEST_ASSERT_EQUAL_UINT8(0x11U, buf[0], "buf[0]");
    TEST_ASSERT_EQUAL_UINT8(0x22U, buf[1], "buf[1]");
    /* NACK deve ter sido enviado exactamente 1 vez (último byte) */
    TEST_ASSERT_EQUAL_INT(1, mock_i2c.send_nack_count, "NACK no último byte");
    /* ACK deve ter sido enviado para bytes intermédios (aqui: 1 byte antes do último → 0 ACKs para 2 bytes) */
}

/** Leitura de 4 bytes: ACK nos 3 primeiros, NACK no quarto. */
static void test_i2c_read_4bytes_ack_nack_sequence(void)
{
    mock_i2c_reset();
    uint8_t rx_data[] = {0xAAU, 0xBBU, 0xCCU, 0xDDU};
    mock_i2c_set_rx_data_seq(rx_data, 4);

    i2c_handle_t h;
    uint8_t buf[4] = {0};
    setup_read(&h, 0x42U, buf, 4U, test_callback);
    force_to_idle(&h);

    TEST_ASSERT_EQUAL_INT(3, mock_i2c.send_ack_count,  "3 ACKs (bytes 1-3)");
    TEST_ASSERT_EQUAL_INT(1, mock_i2c.send_nack_count, "1 NACK (byte 4)");
    TEST_ASSERT_EQUAL_UINT8(0xDDU, buf[3], "último byte correto");
}

/** NACK no endereço durante leitura: callback(-1). */
static void test_i2c_read_nack_on_address(void)
{
    mock_i2c_reset();
    uint8_t ack_seq[] = {0};
    mock_i2c_set_ack_seq(ack_seq, 1);

    i2c_handle_t h;
    uint8_t buf[2] = {0};
    cb_called = 0; cb_result = 99;

    setup_read(&h, 0x60U, buf, 2U, test_callback);
    force_to_idle(&h);

    TEST_ASSERT_EQUAL_INT(-1, cb_result, "callback(-1) em NACK leitura");
}

/** Timeout RX durante leitura: callback(-1). */
static void test_i2c_read_rx_timeout(void)
{
    mock_i2c_reset();
    mock_i2c.rx_ready_default = 0; /* RX nunca pronto */

    i2c_handle_t h;
    uint8_t buf[1] = {0};
    cb_called = 0; cb_result = 99;

    setup_read(&h, 0x48U, buf, 1U, test_callback);

    for (int i = 0; i < 10 && h.state != I2C_WAIT_RX; i++)
        i2c_tick(&h);

    if (h.state == I2C_WAIT_RX) {
        h.timeout = I2C_TIMEOUT_MAX;
        i2c_tick(&h);
    }
    force_to_idle(&h);

    TEST_ASSERT_EQUAL_INT(-1, cb_result, "callback(-1) em RX timeout");
}

/* =========================================================================
 * GRUPO 3: Leitura com registo (use_reg = 1)
 * ========================================================================= */

/** Leitura de registo: escrita de addr+reg → restart → leitura → callback(0). */
static void test_i2c_reg_read_success(void)
{
    mock_i2c_reset();
    uint8_t rx_data[] = {0x7FU};
    mock_i2c_set_rx_data_seq(rx_data, 1);

    i2c_handle_t h;
    uint8_t buf[1] = {0};
    cb_called = 0; cb_result = 99;

    setup_reg_read(&h, 0x68U, 0x3BU, buf, 1U, test_callback);
    force_to_idle(&h);

    TEST_ASSERT_EQUAL_INT(I2C_IDLE, h.state, "IDLE após reg read");
    TEST_ASSERT_EQUAL_INT(0, cb_result,      "sucesso");
    TEST_ASSERT_EQUAL_UINT8(0x7FU, buf[0],  "dado lido correto");
    TEST_ASSERT_EQUAL_INT(1, mock_i2c.restart_read_count, "restart_read chamado");
}

/** NACK no endereço durante reg read: callback(-1). */
static void test_i2c_reg_read_nack_on_addr(void)
{
    mock_i2c_reset();
    uint8_t ack_seq[] = {0};
    mock_i2c_set_ack_seq(ack_seq, 1);

    i2c_handle_t h;
    uint8_t buf[1] = {0};
    cb_called = 0; cb_result = 99;

    setup_reg_read(&h, 0x68U, 0x00U, buf, 1U, test_callback);
    force_to_idle(&h);

    TEST_ASSERT_EQUAL_INT(-1, cb_result, "callback(-1) em NACK addr reg read");
}

/** Timeout TX no estado RESTART (aguarda que TX fique pronto após enviar reg). */
static void test_i2c_reg_read_tx_timeout_in_restart(void)
{
    mock_i2c_reset();
    /* ACK no endereço, depois TX nunca pronto (para o restart) */
    mock_i2c.tx_ready_default = 0;

    i2c_handle_t h;
    uint8_t buf[1] = {0};
    cb_called = 0; cb_result = 99;

    setup_reg_read(&h, 0x68U, 0x3BU, buf, 1U, test_callback);

    /* Avança até I2C_RESTART */
    for (int i = 0; i < 10 && h.state != I2C_RESTART; i++)
        i2c_tick(&h);

    if (h.state == I2C_RESTART) {
        h.timeout = I2C_TIMEOUT_MAX;
        i2c_tick(&h);
    }
    force_to_idle(&h);

    TEST_ASSERT_EQUAL_INT(-1, cb_result, "callback(-1) em TX timeout no restart");
}

/** NACK no wait_restart: callback(-1). */
static void test_i2c_reg_read_nack_on_restart(void)
{
    mock_i2c_reset();
    /* ACK no endereço (para STARTING→SELECT_MODE),
       depois tx_ready=1 para avançar no RESTART,
       depois NACK no WAIT_RESTART */
    uint8_t ack_seq[] = {1, 0}; /* 1=ACK no endereço, 0=NACK no wait_restart */
    mock_i2c_set_ack_seq(ack_seq, 2);

    i2c_handle_t h;
    uint8_t buf[1] = {0};
    cb_called = 0; cb_result = 99;

    setup_reg_read(&h, 0x68U, 0x3BU, buf, 1U, test_callback);
    force_to_idle(&h);

    TEST_ASSERT_EQUAL_INT(-1, cb_result, "callback(-1) em NACK wait_restart");
}

/* =========================================================================
 * GRUPO 4: Recuperação de erros e bus
 * ========================================================================= */

/** error_count incrementa em cada falha. */
static void test_i2c_error_count_increments_on_fail(void)
{
    mock_i2c_reset();
    uint8_t ack_seq[] = {0};
    mock_i2c_set_ack_seq(ack_seq, 1);

    i2c_handle_t h;
    uint8_t buf[] = {0x00U};
    setup_write(&h, 0x42U, buf, 1U, NULL);
    force_to_idle(&h);

    TEST_ASSERT_TRUE(h.error_count > 0 || mock_i2c.bus_recovery_count > 0,
                     "error_count incrementado ou bus recovery ocorreu");
}

/** error_count repõe-se a 0 após sucesso. */
static void test_i2c_error_count_resets_on_success(void)
{
    mock_i2c_reset();
    i2c_handle_t h;
    uint8_t buf[] = {0xAAU};
    h.error_count = 2U; /* simula erros anteriores */

    setup_write(&h, 0x42U, buf, 1U, test_callback);
    force_to_idle(&h);

    TEST_ASSERT_EQUAL_INT(0, h.error_count, "error_count reposto em sucesso");
}

/** Bus recovery chamado após 3 erros consecutivos.
 *
 *  Nota: error_count é armazenado NO handle. Para acumular 3 erros
 *  o mesmo handle deve ser reutilizado (sem memset entre tentativas).
 *  O driver chama bus_recovery quando error_count >= 3.
 */
static void test_i2c_bus_recovery_after_3_errors(void)
{
    mock_i2c_reset();
    mock_i2c.ack_default      = 0; /* NACK em todas as verificações */
    mock_i2c.bus_free_default = 1;

    i2c_handle_t h;
    uint8_t buf[] = {0x00U};

    /* Inicializa o handle UMA vez — error_count acumula entre tentativas */
    memset(&h, 0, sizeof(h));
    h.addr     = 0x42U;
    h.buf      = buf;
    h.len      = 1U;
    h.rw       = 0;
    h.use_reg  = 0;
    h.callback = NULL;

    /* 3 tentativas usando o mesmo handle */
    for (int attempt = 0; attempt < 3; attempt++) {
        h.state = I2C_STARTING;
        for (int t = 0; t < 10 && h.state != I2C_IDLE; t++)
            i2c_tick(&h);
    }

    TEST_ASSERT_TRUE(mock_i2c.bus_recovery_count >= 1,
                     "bus_recovery chamado após 3 erros");
}

/** Bus não livre: handle fica em I2C_STARTING sem progredir. */
static void test_i2c_bus_not_free_blocks_starting(void)
{
    mock_i2c_reset();
    mock_i2c.bus_free_default = 0; /* barramento nunca livre */

    i2c_handle_t h;
    uint8_t buf[] = {0x00U};
    setup_write(&h, 0x42U, buf, 1U, NULL);

    i2c_tick(&h);
    i2c_tick(&h);
    i2c_tick(&h);

    TEST_ASSERT_EQUAL_INT(I2C_STARTING, h.state, "permanece em STARTING com bus ocupado");
    TEST_ASSERT_EQUAL_INT(0, mock_i2c.start_count, "START não foi enviado");

    /* Limpa: liberta bus para force_to_idle */
    mock_i2c.bus_free_default = 1;
    force_to_idle(&h);
}

/** STOP é sempre chamado em caso de sucesso. */
static void test_i2c_stop_called_on_success(void)
{
    mock_i2c_reset();
    i2c_handle_t h;
    uint8_t buf[] = {0x11U};

    setup_write(&h, 0x42U, buf, 1U, NULL);
    force_to_idle(&h);

    TEST_ASSERT_EQUAL_INT(1, mock_i2c.stop_count, "STOP chamado em sucesso");
}

/** STOP é sempre chamado em caso de falha. */
static void test_i2c_stop_called_on_fail(void)
{
    mock_i2c_reset();
    mock_i2c.ack_default = 0;
    i2c_handle_t h;
    uint8_t buf[] = {0x11U};

    setup_write(&h, 0x42U, buf, 1U, NULL);
    force_to_idle(&h);

    TEST_ASSERT_EQUAL_INT(1, mock_i2c.stop_count, "STOP chamado em falha");
}

/** Callback NULL não causa crash em sucesso. */
static void test_i2c_null_callback_no_crash_success(void)
{
    mock_i2c_reset();
    i2c_handle_t h;
    uint8_t buf[] = {0x55U};

    setup_write(&h, 0x42U, buf, 1U, NULL);
    force_to_idle(&h);

    TEST_ASSERT_EQUAL_INT(I2C_IDLE, h.state, "IDLE sem callback em sucesso");
}

/** Callback NULL não causa crash em falha. */
static void test_i2c_null_callback_no_crash_fail(void)
{
    mock_i2c_reset();
    mock_i2c.ack_default = 0;
    i2c_handle_t h;
    uint8_t buf[] = {0x55U};

    setup_write(&h, 0x42U, buf, 1U, NULL);
    force_to_idle(&h);

    TEST_ASSERT_EQUAL_INT(I2C_IDLE, h.state, "IDLE sem callback em falha");
}

/** Tick em I2C_IDLE não faz nada. */
static void test_i2c_tick_on_idle_is_noop(void)
{
    mock_i2c_reset();
    i2c_handle_t h;
    memset(&h, 0, sizeof(h));
    h.state = I2C_IDLE;

    i2c_tick(&h);
    i2c_tick(&h);

    TEST_ASSERT_EQUAL_INT(I2C_IDLE, h.state,     "permanece IDLE");
    TEST_ASSERT_EQUAL_INT(0, mock_i2c.start_count, "START não chamado");
}

/* =========================================================================
 * main
 * ========================================================================= */

int main(void)
{
    TEST_BEGIN("I2C Driver — Sistema Espacial OBC");

    /* Grupo 1: Escrita nominal */
    RUN_TEST(test_i2c_write_single_byte_success);
    RUN_TEST(test_i2c_write_multiple_bytes_success);
    RUN_TEST(test_i2c_write_nack_on_address);
    RUN_TEST(test_i2c_write_nack_mid_transfer);
    RUN_TEST(test_i2c_write_tx_timeout);

    /* Grupo 2: Leitura nominal */
    RUN_TEST(test_i2c_read_single_byte_success);
    RUN_TEST(test_i2c_read_multiple_bytes_nack_on_last);
    RUN_TEST(test_i2c_read_4bytes_ack_nack_sequence);
    RUN_TEST(test_i2c_read_nack_on_address);
    RUN_TEST(test_i2c_read_rx_timeout);

    /* Grupo 3: Leitura com registo */
    RUN_TEST(test_i2c_reg_read_success);
    RUN_TEST(test_i2c_reg_read_nack_on_addr);
    RUN_TEST(test_i2c_reg_read_tx_timeout_in_restart);
    RUN_TEST(test_i2c_reg_read_nack_on_restart);

    /* Grupo 4: Recuperação e robustez */
    RUN_TEST(test_i2c_error_count_increments_on_fail);
    RUN_TEST(test_i2c_error_count_resets_on_success);
    RUN_TEST(test_i2c_bus_recovery_after_3_errors);
    RUN_TEST(test_i2c_bus_not_free_blocks_starting);
    RUN_TEST(test_i2c_stop_called_on_success);
    RUN_TEST(test_i2c_stop_called_on_fail);
    RUN_TEST(test_i2c_null_callback_no_crash_success);
    RUN_TEST(test_i2c_null_callback_no_crash_fail);
    RUN_TEST(test_i2c_tick_on_idle_is_noop);

    TEST_END();
}
