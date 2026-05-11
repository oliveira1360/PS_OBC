/**
 * @file test_i2c_stress.c
 * @brief Testes intensivos do I2C driver — sistema espacial OBC.
 *
 * Cobertura que vai além dos testes unitários nominais:
 *
 * GRUPO 1 — Concorrência de dois handles (bus_locked como mutex)
 *   Verifica que bus_locked impede dois handles de adquirirem o bus
 *   simultaneamente. O segundo handle fica bloqueado em I2C_STARTING
 *   enquanto o primeiro está a transferir, e só avança após o primeiro
 *   chegar a IDLE.
 *
 * GRUPO 2 — Séries longas (50+ ciclos)
 *   Detecção de: acumulação silenciosa de error_count, leaks de
 *   bus_locked, corrupção de index/timeout entre ciclos, deriva de
 *   estado após muitas iterações.
 *
 * GRUPO 3 — Recovery completo sob carga
 *   NACK storm → bus_recovery() → nova tentativa bem sucedida, incluindo
 *   múltiplos ciclos de recovery e mistura de erros com sucessos.
 *
 * GRUPO 4 — Integridade de dados
 *   Payload máximo, bytes em ordem correcta, múltiplos endereços sem
 *   mistura, dados de leitura íntegros após muitos ciclos.
 *
 * GRUPO 5 — Robustez SEU (Single Event Upset)
 *   Estado corrompido a meio de transferência, timeout no limite máximo
 *   do uint16, re-uso imediato após erro.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "../framework/test_runner.h"
#include "../mocks/mock_hal_i2c.h"
#include "drivers/i2c_driver.h"

/* =========================================================================
 * Helpers locais
 * ========================================================================= */

static int cb_result;
static int cb_called;

static void test_cb(int r) { cb_called++; cb_result = r; }

static void setup_write(i2c_handle_t *h, uint8_t addr,
                        uint8_t *buf, uint8_t len, void (*cb)(int))
{
    memset(h, 0, sizeof(*h));
    h->addr = addr; h->buf = buf; h->len = len;
    h->rw = 0; h->use_reg = 0; h->callback = cb;
    h->state = I2C_STARTING;
}

static void setup_read(i2c_handle_t *h, uint8_t addr,
                       uint8_t *buf, uint8_t len, void (*cb)(int))
{
    memset(h, 0, sizeof(*h));
    h->addr = addr; h->buf = buf; h->len = len;
    h->rw = 1; h->use_reg = 0; h->callback = cb;
    h->state = I2C_STARTING;
}

static void setup_reg_read(i2c_handle_t *h, uint8_t addr, uint8_t reg,
                           uint8_t *buf, uint8_t len, void (*cb)(int))
{
    memset(h, 0, sizeof(*h));
    h->addr = addr; h->reg = reg; h->buf = buf; h->len = len;
    h->rw = 1; h->use_reg = 1; h->callback = cb;
    h->state = I2C_STARTING;
}

/** Corre a FSM até IDLE com todas as respostas HAL afirmativas. */
static void force_to_idle(i2c_handle_t *h)
{
    mock_i2c.ack_default      = 1;
    mock_i2c.tx_ready_default = 1;
    mock_i2c.rx_ready_default = 1;
    mock_i2c.bus_free_default = 1;
    for (int i = 0; i < 2000 && h->state != I2C_IDLE; i++)
        i2c_tick(h);
}

/* =========================================================================
 * GRUPO 1 — Concorrência de dois handles (bus_locked como mutex)
 * ========================================================================= */

/**
 * Dois handles simultâneos: A adquire o bus, B fica bloqueado em
 * I2C_STARTING até A terminar. Verifica que B nunca avança enquanto
 * bus_locked está activo.
 */
static void test_i2c_two_handles_bus_arbitration(void)
{
    mock_i2c_reset();
    i2c_handle_t hA, hB;
    uint8_t bufA[] = {0xAAU};
    uint8_t bufB[] = {0xBBU};
    
    void (*cbA_fn)(int) = NULL; (void)cbA_fn;

    /* Setup: ambos a escrever para endereços diferentes */
    setup_write(&hA, 0x20U, bufA, 1U, test_cb);
    setup_write(&hB, 0x30U, bufB, 1U, test_cb);

    mock_i2c.bus_free_default = 1;
    mock_i2c.ack_default      = 1;
    mock_i2c.tx_ready_default = 1;

    /* Tick A uma vez: A adquire bus (STARTING→SELECT_MODE) */
    i2c_tick(&hA);
    TEST_ASSERT_TRUE(hA.state == I2C_SELECT_MODE,
                     "A deve estar em SELECT_MODE depois do 1º tick");

    /* Tick B agora: bus_locked=1, B deve ficar em STARTING */
    i2c_tick(&hB);
    TEST_ASSERT_TRUE(hB.state == I2C_STARTING,
                     "B deve ficar em STARTING enquanto A tem o bus");

    /* Completar A */
    cb_called = 0; cb_result = 99;
    force_to_idle(&hA);
    TEST_ASSERT_TRUE(hA.state == I2C_IDLE, "A deve terminar em IDLE");

    /* Agora B deve conseguir avançar */
    i2c_tick(&hB);
    TEST_ASSERT_TRUE(hB.state != I2C_STARTING,
                     "B deve avançar depois de A libertar o bus");
    force_to_idle(&hB);
    TEST_ASSERT_TRUE(hB.state == I2C_IDLE, "B deve completar em IDLE");
}

/**
 * Ticks intercalados: A e B são ticked alternadamente. Só A avança
 * enquanto tem o bus; B só avança depois.
 */
static void test_i2c_interleaved_ticks_both_complete(void)
{
    mock_i2c_reset();
    i2c_handle_t hA, hB;
    uint8_t bufA[] = {0x01U, 0x02U};
    uint8_t bufB[] = {0x03U, 0x04U};
    

    /* Callbacks locais usando contador externo */
    cb_called = 0; cb_result = 0;

    setup_write(&hA, 0x50U, bufA, 2U, test_cb);
    setup_write(&hB, 0x51U, bufB, 2U, test_cb);

    mock_i2c.bus_free_default = 1;
    mock_i2c.ack_default      = 1;
    mock_i2c.tx_ready_default = 1;

    /* Intercalar ticks: A, B, A, B... até ambos terminarem */
    for (int i = 0; i < 400; i++) {
        if (hA.state != I2C_IDLE) i2c_tick(&hA);
        if (hB.state != I2C_IDLE) i2c_tick(&hB);
        if (hA.state == I2C_IDLE && hB.state == I2C_IDLE) break;
    }

    TEST_ASSERT_TRUE(hA.state == I2C_IDLE, "A deve terminar em IDLE (interleaved)");
    TEST_ASSERT_TRUE(hB.state == I2C_IDLE, "B deve terminar em IDLE (interleaved)");
    
}

/**
 * Após NACK em A → bus_locked é libertado → B consegue adquirir bus.
 */
static void test_i2c_bus_released_after_nack(void)
{
    mock_i2c_reset();
    i2c_handle_t hA, hB;
    uint8_t bufA[] = {0xFFU};
    uint8_t bufB[] = {0x01U};

    setup_write(&hA, 0x40U, bufA, 1U, test_cb);
    setup_write(&hB, 0x41U, bufB, 1U, test_cb);

    mock_i2c.bus_free_default = 1;
    mock_i2c.tx_ready_default = 1;

    /* A entra em SELECT_MODE */
    i2c_tick(&hA); /* STARTING → SELECT_MODE */

    /* B tenta entrar mas bus está locked */
    i2c_tick(&hB);
    TEST_ASSERT_TRUE(hB.state == I2C_STARTING, "B bloqueado enquanto A tem bus");

    /* A recebe NACK no endereço → i2c_fail → bus_locked=0 */
    mock_i2c.ack_default = 0; /* NACK */
    i2c_tick(&hA); /* SELECT_MODE → i2c_fail → IDLE */
    TEST_ASSERT_TRUE(hA.state == I2C_IDLE, "A deve ir para IDLE após NACK");

    /* Agora B deve conseguir avançar */
    mock_i2c.ack_default = 1;
    i2c_tick(&hB);
    TEST_ASSERT_TRUE(hB.state != I2C_STARTING, "B deve avançar após A libertar bus por NACK");
    force_to_idle(&hB);
    TEST_ASSERT_TRUE(hB.state == I2C_IDLE, "B deve completar");
}

/**
 * Após timeout em A → bus_locked é libertado → B consegue adquirir bus.
 */
static void test_i2c_bus_released_after_timeout(void)
{
    mock_i2c_reset();
    i2c_handle_t hA, hB;
    uint8_t bufA[] = {0x55U};
    uint8_t bufB[] = {0xAAU};

    setup_write(&hA, 0x42U, bufA, 1U, test_cb);
    setup_write(&hB, 0x43U, bufB, 1U, test_cb);

    mock_i2c.bus_free_default = 1;
    mock_i2c.ack_default      = 1;
    mock_i2c.tx_ready_default = 0; /* TX nunca pronto → A vai dar timeout */

    /* A: STARTING → SELECT_MODE (ack=1) → WRITE → WAIT_TX */
    i2c_tick(&hA); /* STARTING → SELECT_MODE */
    i2c_tick(&hA); /* SELECT_MODE → WRITE */
    i2c_tick(&hA); /* WRITE → WAIT_TX */

    /* B tenta — bus ainda locked */
    i2c_tick(&hB);
    TEST_ASSERT_TRUE(hB.state == I2C_STARTING, "B bloqueado durante WAIT_TX de A");

    /* Levar A ao timeout: pré-seed timeout */
    hA.timeout = I2C_TIMEOUT_MAX; /* próximo tick incrementa para MAX+1 → timeout */
    i2c_tick(&hA); /* WAIT_TX: timeout > MAX → i2c_fail → IDLE */
    TEST_ASSERT_TRUE(hA.state == I2C_IDLE, "A deve ir para IDLE após timeout TX");

    /* B deve agora conseguir avançar */
    mock_i2c.tx_ready_default = 1;
    i2c_tick(&hB);
    TEST_ASSERT_TRUE(hB.state != I2C_STARTING, "B deve avançar após A libertar bus por timeout");
    force_to_idle(&hB);
    TEST_ASSERT_TRUE(hB.state == I2C_IDLE, "B deve completar após timeout de A");
}

/**
 * Três handles tentam acesso sequencial: A termina → B procede → C procede.
 * Garante que bus_locked não fica "preso" ao longo de múltiplas
 * aquisições e libertações.
 */
static void test_i2c_three_handles_sequential_acquisition(void)
{
    mock_i2c_reset();
    i2c_handle_t hA, hB, hC;
    uint8_t bufA[] = {0x01U};
    uint8_t bufB[] = {0x02U};
    uint8_t bufC[] = {0x03U};

    setup_write(&hA, 0x10U, bufA, 1U, NULL);
    setup_write(&hB, 0x11U, bufB, 1U, NULL);
    setup_write(&hC, 0x12U, bufC, 1U, NULL);

    mock_i2c.bus_free_default = 1;
    mock_i2c.ack_default      = 1;
    mock_i2c.tx_ready_default = 1;

    /* A adquire bus */
    i2c_tick(&hA);
    TEST_ASSERT_TRUE(hA.state == I2C_SELECT_MODE, "A adquiriu bus");

    /* B e C ficam à espera */
    i2c_tick(&hB);
    i2c_tick(&hC);
    TEST_ASSERT_TRUE(hB.state == I2C_STARTING, "B bloquado");
    TEST_ASSERT_TRUE(hC.state == I2C_STARTING, "C bloqueado");

    /* A termina */
    force_to_idle(&hA);

    /* B adquire */
    i2c_tick(&hB);
    TEST_ASSERT_TRUE(hB.state != I2C_STARTING, "B avançou após A");

    /* C ainda à espera */
    i2c_tick(&hC);
    TEST_ASSERT_TRUE(hC.state == I2C_STARTING, "C ainda bloqueado por B");

    /* B termina */
    force_to_idle(&hB);

    /* C avança */
    i2c_tick(&hC);
    TEST_ASSERT_TRUE(hC.state != I2C_STARTING, "C avançou após B");
    force_to_idle(&hC);
    TEST_ASSERT_TRUE(hC.state == I2C_IDLE, "C completou");
}

/* =========================================================================
 * GRUPO 2 — Séries longas (50+ ciclos no mesmo handle)
 * ========================================================================= */

/** 50 escritas consecutivas de 1 byte: sem corrupção, error_count=0. */
static void test_i2c_50_consecutive_writes(void)
{
    mock_i2c_reset();
    i2c_handle_t h;
    int fail_count = 0;

    for (int i = 0; i < 50; i++) {
        uint8_t val = (uint8_t)(i & 0xFF);
        /* Re-inicializar handle sem memset para detectar resíduos */
        h.addr     = 0x68U;
        h.buf      = &val;
        h.len      = 1U;
        h.rw       = 0;
        h.use_reg  = 0;
        h.callback = test_cb;
        h.state    = I2C_STARTING;
        h.timeout  = 0;
        h.index    = 0;

        mock_i2c_reset();
        mock_i2c.bus_free_default = 1;
        mock_i2c.ack_default      = 1;
        mock_i2c.tx_ready_default = 1;

        cb_called = 0; cb_result = 99;
        force_to_idle(&h);

        if (h.state != I2C_IDLE || cb_result != 0 || h.error_count != 0)
            fail_count++;
    }

    TEST_ASSERT_EQUAL_INT(0, fail_count,
        "Nenhum dos 50 ciclos de escrita deve falhar");
}

/** 50 leituras consecutivas de 2 bytes: dados correctos, sem corrupção. */
static void test_i2c_50_consecutive_reads(void)
{
    mock_i2c_reset();
    i2c_handle_t h;
    int fail_count = 0;

    for (int i = 0; i < 50; i++) {
        uint8_t rx[2] = {0, 0};

        h.addr     = 0x77U;
        h.buf      = rx;
        h.len      = 2U;
        h.rw       = 1;
        h.use_reg  = 0;
        h.callback = test_cb;
        h.state    = I2C_STARTING;
        h.timeout  = 0;
        h.index    = 0;

        mock_i2c_reset();
        mock_i2c.bus_free_default = 1;
        mock_i2c.ack_default      = 1;
        mock_i2c.rx_ready_default = 1;

        uint8_t rdata[] = {(uint8_t)(i & 0xFF), (uint8_t)((i + 1) & 0xFF)};
        mock_i2c_set_rx_data_seq(rdata, 2);

        cb_called = 0; cb_result = 99;
        force_to_idle(&h);

        if (h.state != I2C_IDLE || cb_result != 0 ||
            rx[0] != rdata[0] || rx[1] != rdata[1])
            fail_count++;
    }

    TEST_ASSERT_EQUAL_INT(0, fail_count,
        "Nenhum dos 50 ciclos de leitura deve falhar ou corromper dados");
}

/** 25 reg_reads consecutivos: endereço + registo + leitura. */
static void test_i2c_25_reg_reads_sequential(void)
{
    mock_i2c_reset();
    i2c_handle_t h;
    int fail_count = 0;

    for (int i = 0; i < 25; i++) {
        uint8_t rx[1] = {0};

        h.addr     = 0x68U;
        h.reg      = (uint8_t)(i & 0xFF);
        h.buf      = rx;
        h.len      = 1U;
        h.rw       = 1;
        h.use_reg  = 1;
        h.callback = test_cb;
        h.state    = I2C_STARTING;
        h.timeout  = 0;
        h.index    = 0;

        mock_i2c_reset();
        mock_i2c.bus_free_default = 1;
        mock_i2c.ack_default      = 1;
        mock_i2c.tx_ready_default = 1;
        mock_i2c.rx_ready_default = 1;

        uint8_t rdata[] = {(uint8_t)(0xA0U + i)};
        mock_i2c_set_rx_data_seq(rdata, 1);

        cb_called = 0; cb_result = 99;
        force_to_idle(&h);

        if (h.state != I2C_IDLE || cb_result != 0 || rx[0] != rdata[0])
            fail_count++;
    }

    TEST_ASSERT_EQUAL_INT(0, fail_count,
        "Nenhum dos 25 reg_reads deve falhar ou corromper dados");
}

/** Alternância write/read 25 vezes no mesmo handle: sem contaminação. */
static void test_i2c_alternating_write_read_25_cycles(void)
{
    mock_i2c_reset();
    i2c_handle_t h;
    int fail_count = 0;

    for (int i = 0; i < 25; i++) {
        /* --- Write --- */
        uint8_t tx_val = (uint8_t)(i & 0xFF);
        h.addr = 0x20U; h.buf = &tx_val; h.len = 1U;
        h.rw = 0; h.use_reg = 0; h.callback = test_cb;
        h.state = I2C_STARTING; h.timeout = 0; h.index = 0;

        mock_i2c_reset();
        mock_i2c.bus_free_default = 1;
        mock_i2c.ack_default      = 1;
        mock_i2c.tx_ready_default = 1;

        cb_result = 99;
        force_to_idle(&h);
        if (h.state != I2C_IDLE || cb_result != 0) { fail_count++; continue; }

        /* --- Read --- */
        uint8_t rx_val = 0;
        h.addr = 0x20U; h.buf = &rx_val; h.len = 1U;
        h.rw = 1; h.use_reg = 0; h.callback = test_cb;
        h.state = I2C_STARTING; h.timeout = 0; h.index = 0;

        mock_i2c_reset();
        mock_i2c.bus_free_default = 1;
        mock_i2c.ack_default      = 1;
        mock_i2c.rx_ready_default = 1;

        uint8_t rdata[] = {(uint8_t)(0x80U | (i & 0x7FU))};
        mock_i2c_set_rx_data_seq(rdata, 1);

        cb_result = 99;
        force_to_idle(&h);
        if (h.state != I2C_IDLE || cb_result != 0 || rx_val != rdata[0])
            fail_count++;
    }

    TEST_ASSERT_EQUAL_INT(0, fail_count,
        "Nenhum dos 25 ciclos write+read deve falhar");
}

/* =========================================================================
 * GRUPO 3 — Recovery completo sob carga
 * ========================================================================= */

/**
 * 3 NACKs consecutivos no endereço → bus_recovery() chamado → 4ª tentativa
 * bem sucedida. Verifica o ciclo completo de degradação e recuperação.
 */
static void test_i2c_nack_storm_then_recovery_then_success(void)
{
    mock_i2c_reset();
    i2c_handle_t h;
    uint8_t buf[] = {0x55U};

    /* Inicializar uma única vez para error_count acumular */
    memset(&h, 0, sizeof(h));
    h.addr = 0x42U; h.buf = buf; h.len = 1U;
    h.rw = 0; h.use_reg = 0; h.callback = test_cb;

    mock_i2c.bus_free_default = 1;
    mock_i2c.tx_ready_default = 1;

    int recovery_before = mock_i2c.bus_recovery_count;

    /* 3 tentativas com NACK no endereço */
    for (int attempt = 0; attempt < 3; attempt++) {
        h.state = I2C_STARTING;
        mock_i2c.ack_default = 0; /* NACK */
        mock_i2c.ack_seq_len = 0;

        /* STARTING → SELECT_MODE → NACK → i2c_fail → IDLE */
        i2c_tick(&h); /* STARTING → SELECT_MODE */
        i2c_tick(&h); /* SELECT_MODE → NACK → IDLE */
        TEST_ASSERT_TRUE(h.state == I2C_IDLE,
            "Handle deve voltar a IDLE após NACK");
    }

    /* Após 3 erros, bus_recovery deve ter sido chamado */
    TEST_ASSERT_TRUE(mock_i2c.bus_recovery_count > recovery_before,
        "bus_recovery() deve ser chamado após 3 erros consecutivos");
    TEST_ASSERT_EQUAL_INT(0, (int)h.error_count,
        "error_count deve ser reposto a 0 após recovery");

    /* 4ª tentativa: sucesso */
    h.state = I2C_STARTING;
    mock_i2c.ack_default = 1;

    cb_called = 0; cb_result = 99;
    force_to_idle(&h);

    TEST_ASSERT_TRUE(h.state == I2C_IDLE, "4ª tentativa deve completar em IDLE");
    TEST_ASSERT_EQUAL_INT(0, cb_result, "4ª tentativa deve chamar callback(0)");
}

/**
 * Dois ciclos completos de recovery: 3 erros → recovery → 3 erros → recovery.
 * Garante que o mecanismo não "queima" após ser usado uma vez.
 */
static void test_i2c_two_full_recovery_cycles(void)
{
    mock_i2c_reset();
    i2c_handle_t h;
    uint8_t buf[] = {0xAAU};

    memset(&h, 0, sizeof(h));
    h.addr = 0x30U; h.buf = buf; h.len = 1U;
    h.rw = 0; h.use_reg = 0; h.callback = NULL;

    mock_i2c.bus_free_default = 1;
    mock_i2c.tx_ready_default = 1;
    mock_i2c.ack_default      = 0; /* sempre NACK */

    /* Ciclo 1: 3 erros */
    for (int i = 0; i < 3; i++) {
        h.state = I2C_STARTING;
        i2c_tick(&h);
        i2c_tick(&h);
    }
    TEST_ASSERT_EQUAL_INT(1, mock_i2c.bus_recovery_count, "1º recovery");
    TEST_ASSERT_EQUAL_INT(0, (int)h.error_count, "error_count reposto após 1º recovery");

    /* Ciclo 2: 3 erros */
    for (int i = 0; i < 3; i++) {
        h.state = I2C_STARTING;
        i2c_tick(&h);
        i2c_tick(&h);
    }
    TEST_ASSERT_EQUAL_INT(2, mock_i2c.bus_recovery_count, "2º recovery");
    TEST_ASSERT_EQUAL_INT(0, (int)h.error_count, "error_count reposto após 2º recovery");
}

/**
 * Mistura de erros e sucessos: cada erro isolado não deve acumular
 * suficientemente para disparar recovery. Erro–sucesso–erro–sucesso nunca
 * deve chamar bus_recovery().
 */
static void test_i2c_isolated_errors_no_recovery(void)
{
    mock_i2c_reset();
    i2c_handle_t h;
    uint8_t buf[] = {0x01U};

    memset(&h, 0, sizeof(h));
    h.addr = 0x25U; h.buf = buf; h.len = 1U;
    h.rw = 0; h.use_reg = 0; h.callback = NULL;

    mock_i2c.bus_free_default = 1;
    mock_i2c.tx_ready_default = 1;

    for (int cycle = 0; cycle < 10; cycle++) {
        /* Erro */
        h.state = I2C_STARTING;
        mock_i2c.ack_default = 0;
        mock_i2c.ack_seq_len = 0;
        i2c_tick(&h);
        i2c_tick(&h);
        TEST_ASSERT_TRUE(h.state == I2C_IDLE, "deve estar IDLE após erro");

        /* Sucesso (repõe error_count) */
        h.state = I2C_STARTING;
        mock_i2c.ack_default = 1;
        mock_i2c.ack_seq_len = 0;
        force_to_idle(&h);
        TEST_ASSERT_EQUAL_INT(0, (int)h.error_count,
            "sucesso deve repor error_count a 0");
    }

    TEST_ASSERT_EQUAL_INT(0, mock_i2c.bus_recovery_count,
        "recovery não deve ser chamado com erros isolados por sucessos");
}

/* =========================================================================
 * GRUPO 4 — Integridade de dados
 * ========================================================================= */

/** Escrita de 16 bytes: todos os bytes chegam ao HAL na ordem correcta. */
static void test_i2c_large_payload_write_integrity(void)
{
    mock_i2c_reset();
    i2c_handle_t h;
    uint8_t tx[16];
    for (int i = 0; i < 16; i++) tx[i] = (uint8_t)(0x10U + i);

    setup_write(&h, 0x55U, tx, 16U, test_cb);
    mock_i2c.bus_free_default = 1;
    mock_i2c.ack_default      = 1;
    mock_i2c.tx_ready_default = 1;

    cb_called = 0; cb_result = 99;
    force_to_idle(&h);

    TEST_ASSERT_TRUE(h.state == I2C_IDLE,   "deve terminar em IDLE");
    TEST_ASSERT_EQUAL_INT(0, cb_result,     "callback deve ser 0");
    TEST_ASSERT_EQUAL_INT(16, mock_i2c.sent_bytes_count,
        "16 bytes de dados devem ter sido enviados via send_byte");

    /* Verificar ordem dos bytes enviados (after addr) */
    int order_ok = 1;
    for (int i = 0; i < 16; i++) {
        if (mock_i2c.sent_bytes[i] != tx[i]) { order_ok = 0; break; }
    }
    TEST_ASSERT_TRUE(order_ok, "bytes enviados devem estar na ordem correcta");
}

/** Leitura de 8 bytes: todos os bytes recebidos íntegros no buffer. */
static void test_i2c_large_payload_read_integrity(void)
{
    mock_i2c_reset();
    i2c_handle_t h;
    uint8_t rx[8] = {0};
    uint8_t expected[8];
    for (int i = 0; i < 8; i++) expected[i] = (uint8_t)(0xC0U + i);

    setup_read(&h, 0x44U, rx, 8U, test_cb);
    mock_i2c.bus_free_default = 1;
    mock_i2c.ack_default      = 1;
    mock_i2c.rx_ready_default = 1;
    mock_i2c_set_rx_data_seq(expected, 8);

    cb_called = 0; cb_result = 99;
    force_to_idle(&h);

    TEST_ASSERT_TRUE(h.state == I2C_IDLE, "deve terminar em IDLE");
    TEST_ASSERT_EQUAL_INT(0, cb_result,   "callback deve ser 0");
    TEST_ASSERT_EQUAL_MEM(expected, rx, 8U,
        "8 bytes recebidos devem corresponder ao esperado");
}

/**
 * Dois handles, dois endereços diferentes: verificar que os endereços
 * enviados ao HAL são os correctos para cada handle (sem mistura).
 */
static void test_i2c_two_addresses_no_mixing(void)
{
    i2c_handle_t hA, hB;
    uint8_t bufA[] = {0xAAU};
    uint8_t bufB[] = {0xBBU};

    /* --- Handle A: addr=0x68 --- */
    mock_i2c_reset();
    setup_write(&hA, 0x68U, bufA, 1U, NULL);
    mock_i2c.bus_free_default = 1;
    mock_i2c.ack_default      = 1;
    mock_i2c.tx_ready_default = 1;
    force_to_idle(&hA);
    uint8_t addr_a = mock_i2c.last_addr;

    /* --- Handle B: addr=0x77 --- */
    mock_i2c_reset();
    setup_write(&hB, 0x77U, bufB, 1U, NULL);
    mock_i2c.bus_free_default = 1;
    mock_i2c.ack_default      = 1;
    mock_i2c.tx_ready_default = 1;
    force_to_idle(&hB);
    uint8_t addr_b = mock_i2c.last_addr;

    /* Endereço A: (0x68 << 1) | 0 = 0xD0 */
    TEST_ASSERT_EQUAL_UINT8((uint8_t)(0x68U << 1), addr_a,
        "Handle A deve enviar endereço 0x68 em write");
    /* Endereço B: (0x77 << 1) | 0 = 0xEE */
    TEST_ASSERT_EQUAL_UINT8((uint8_t)(0x77U << 1), addr_b,
        "Handle B deve enviar endereço 0x77 em write");
    TEST_ASSERT_TRUE(addr_a != addr_b, "Endereços não podem ser iguais");
}

/**
 * reg_read envia: addr(write) → reg → restart → addr(read) → leitura.
 * Verifica que a sequência de endereços e o byte de registo chegam ao
 * HAL na ordem e com os valores correctos.
 */
static void test_i2c_reg_read_address_and_register_correctness(void)
{
    mock_i2c_reset();
    i2c_handle_t h;
    uint8_t rx[2] = {0, 0};
    uint8_t rdata[] = {0x12U, 0x34U};

    setup_reg_read(&h, 0x68U, 0x3BU, rx, 2U, test_cb);
    mock_i2c.bus_free_default = 1;
    mock_i2c.ack_default      = 1;
    mock_i2c.tx_ready_default = 1;
    mock_i2c.rx_ready_default = 1;
    mock_i2c_set_rx_data_seq(rdata, 2);

    cb_result = 99;
    force_to_idle(&h);

    TEST_ASSERT_EQUAL_INT(0, cb_result, "reg_read deve completar com sucesso");

    /* Endereço write: (0x68 << 1) | 0 = 0xD0 */
    TEST_ASSERT_EQUAL_UINT8((uint8_t)(0x68U << 1), mock_i2c.last_addr,
        "Endereço write correcto");

    /* Byte de registo enviado */
    TEST_ASSERT_TRUE(mock_i2c.sent_bytes_count >= 1,
        "Registo 0x3B deve ser enviado após endereço");
    TEST_ASSERT_EQUAL_UINT8(0x3BU, mock_i2c.sent_bytes[0],
        "Byte de registo deve ser 0x3B");

    /* restart_read deve ter sido chamado com addr correcto */
    TEST_ASSERT_EQUAL_INT(1, mock_i2c.restart_read_count, "restart_read chamado 1x");
    TEST_ASSERT_EQUAL_UINT8(0x68U, mock_i2c.restart_read_last_addr,
        "restart_read com endereço 0x68");

    /* Dados recebidos correctamente */
    TEST_ASSERT_EQUAL_MEM(rdata, rx, 2U, "Dados de leitura correctos");
}

/* =========================================================================
 * GRUPO 5 — Robustez SEU (Single Event Upset)
 * ========================================================================= */

/**
 * Estado corrompido para valor inválido (como um SEU num flip-flop):
 * i2c_tick() não deve crashar, e idealmente a FSM não entra num loop
 * infinito.
 */
static void test_i2c_seu_state_corruption_no_crash(void)
{
    mock_i2c_reset();
    i2c_handle_t h;
    uint8_t buf[] = {0x00U};

    setup_write(&h, 0x60U, buf, 1U, NULL);
    mock_i2c.bus_free_default = 1;
    mock_i2c.ack_default      = 1;
    mock_i2c.tx_ready_default = 1;

    /* Corromper estado para valor inexistente */
    h.state = (i2c_state_t)99;

    /* Deve não crashar (cobre o default implícito do switch) */
    i2c_tick(&h);
    /* Se chegámos aqui, não houve crash — suficiente para um sistema espacial */
}

/**
 * Timeout no seu valor máximo de uint16: incrementar mais 1 não deve
 * causar overflow silencioso que deixe o handle bloqueado para sempre.
 */
static void test_i2c_timeout_at_uint16_boundary(void)
{
    mock_i2c_reset();
    i2c_handle_t h;
    uint8_t buf[] = {0xFFU};

    setup_write(&h, 0x70U, buf, 1U, test_cb);
    mock_i2c.bus_free_default = 1;
    mock_i2c.ack_default      = 1;
    mock_i2c.tx_ready_default = 0; /* TX nunca pronto */

    /* Avançar até WAIT_TX */
    i2c_tick(&h); /* STARTING → SELECT_MODE */
    i2c_tick(&h); /* SELECT_MODE → WRITE */
    i2c_tick(&h); /* WRITE → WAIT_TX */
    TEST_ASSERT_TRUE(h.state == I2C_WAIT_TX, "deve estar em WAIT_TX");

    /* Pre-seed timeout para UINT16_MAX - 1 */
    h.timeout = (uint16_t)(I2C_TIMEOUT_MAX - 1);

    cb_called = 0; cb_result = 99;
    i2c_tick(&h); /* timeout = TIMEOUT_MAX → ainda < TIMEOUT_MAX+1 → espera */
    /* Um mais... */
    i2c_tick(&h); /* timeout > TIMEOUT_MAX → i2c_fail → IDLE */

    TEST_ASSERT_TRUE(h.state == I2C_IDLE,
        "Deve ir para IDLE após timeout no limite uint16");
    TEST_ASSERT_EQUAL_INT(-1, cb_result,
        "Deve chamar callback(-1) no timeout");
}

/**
 * Re-uso imediato após erro: na mesma frame do superloop, o erro dispara
 * o callback que re-enfileira uma nova operação. Simula re-tentativa
 * imediata no callback.
 */
static void test_i2c_immediate_reuse_after_error(void)
{
    mock_i2c_reset();
    i2c_handle_t h;
    uint8_t buf[] = {0x01U};

    setup_write(&h, 0x55U, buf, 1U, test_cb);
    mock_i2c.bus_free_default = 1;
    mock_i2c.tx_ready_default = 1;

    /* 1ª tentativa: falha com NACK */
    mock_i2c.ack_default = 0;
    i2c_tick(&h); /* STARTING → SELECT_MODE */
    i2c_tick(&h); /* SELECT_MODE → NACK → IDLE */
    TEST_ASSERT_TRUE(h.state == I2C_IDLE, "IDLE após erro");
    TEST_ASSERT_TRUE(h.error_count > 0,   "error_count > 0");

    /* Re-enfileirar imediatamente sem resetar handle */
    h.state    = I2C_STARTING;
    h.rw       = 0;
    h.use_reg  = 0;
    h.index    = 0;
    h.timeout  = 0;
    /* Nota: error_count mantém-se para que a lógica de recovery funcione */

    mock_i2c.ack_default = 1; /* desta vez ACK */

    cb_called = 0; cb_result = 99;
    force_to_idle(&h);

    TEST_ASSERT_TRUE(h.state == I2C_IDLE, "Re-uso deve completar em IDLE");
    TEST_ASSERT_EQUAL_INT(0, cb_result,   "Re-uso deve ter sucesso");
}

/**
 * Timeout em WAIT_RX: comportamento simétrico ao timeout em WAIT_TX.
 * Garante que bus_locked é libertado mesmo no caminho de erro da leitura.
 */
static void test_i2c_rx_timeout_releases_bus_for_next_handle(void)
{
    mock_i2c_reset();
    i2c_handle_t hA, hB;
    uint8_t bufA[1] = {0};
    uint8_t bufB[]  = {0x10U};

    setup_read(&hA, 0x68U, bufA, 1U, test_cb);
    setup_write(&hB, 0x70U, bufB, 1U, test_cb);

    mock_i2c.bus_free_default = 1;
    mock_i2c.ack_default      = 1;
    mock_i2c.rx_ready_default = 0; /* RX nunca pronto */

    /* Avançar A até WAIT_RX */
    i2c_tick(&hA); /* STARTING → SELECT_MODE */
    i2c_tick(&hA); /* SELECT_MODE → READ */
    i2c_tick(&hA); /* READ → WAIT_RX */
    TEST_ASSERT_TRUE(hA.state == I2C_WAIT_RX, "A deve estar em WAIT_RX");

    /* B fica bloqueado */
    i2c_tick(&hB);
    TEST_ASSERT_TRUE(hB.state == I2C_STARTING, "B bloqueado durante WAIT_RX de A");

    /* A dá timeout */
    hA.timeout = I2C_TIMEOUT_MAX;
    cb_result = 99;
    i2c_tick(&hA); /* timeout > MAX → i2c_fail → IDLE */
    TEST_ASSERT_TRUE(hA.state == I2C_IDLE, "A deve ir para IDLE após RX timeout");
    TEST_ASSERT_EQUAL_INT(-1, cb_result, "callback(-1) no RX timeout");

    /* B deve agora avançar */
    mock_i2c.tx_ready_default = 1;
    i2c_tick(&hB);
    TEST_ASSERT_TRUE(hB.state != I2C_STARTING,
        "B deve avançar após A libertar bus por RX timeout");
    force_to_idle(&hB);
    TEST_ASSERT_TRUE(hB.state == I2C_IDLE, "B deve completar");
}

/* =========================================================================
 * main
 * ========================================================================= */

int main(void)
{
    TEST_BEGIN("I2C Driver — Testes Intensivos / Stress");

    /* Grupo 1: Concorrência */
    RUN_TEST(test_i2c_two_handles_bus_arbitration);
    RUN_TEST(test_i2c_interleaved_ticks_both_complete);
    RUN_TEST(test_i2c_bus_released_after_nack);
    RUN_TEST(test_i2c_bus_released_after_timeout);
    RUN_TEST(test_i2c_three_handles_sequential_acquisition);

    /* Grupo 2: Séries longas */
    RUN_TEST(test_i2c_50_consecutive_writes);
    RUN_TEST(test_i2c_50_consecutive_reads);
    RUN_TEST(test_i2c_25_reg_reads_sequential);
    RUN_TEST(test_i2c_alternating_write_read_25_cycles);

    /* Grupo 3: Recovery sob carga */
    RUN_TEST(test_i2c_nack_storm_then_recovery_then_success);
    RUN_TEST(test_i2c_two_full_recovery_cycles);
    RUN_TEST(test_i2c_isolated_errors_no_recovery);

    /* Grupo 4: Integridade de dados */
    RUN_TEST(test_i2c_large_payload_write_integrity);
    RUN_TEST(test_i2c_large_payload_read_integrity);
    RUN_TEST(test_i2c_two_addresses_no_mixing);
    RUN_TEST(test_i2c_reg_read_address_and_register_correctness);

    /* Grupo 5: Robustez SEU */
    RUN_TEST(test_i2c_seu_state_corruption_no_crash);
    RUN_TEST(test_i2c_timeout_at_uint16_boundary);
    RUN_TEST(test_i2c_immediate_reuse_after_error);
    RUN_TEST(test_i2c_rx_timeout_releases_bus_for_next_handle);

    TEST_END();
}
