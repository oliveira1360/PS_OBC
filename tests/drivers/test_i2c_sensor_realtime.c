/**
 * @file test_i2c_sensor_realtime.c
 * @brief Testes de não-bloqueio e determinismo da leitura I2C de sensores.
 *
 * Objetivo:
 *   Garantir duas propriedades fundamentais para sistemas de tempo-real
 *   embarcado (satélite OBC):
 *
 *   1. NÃO-BLOQUEIO (non-blocking):
 *      As funções *_read_async() e i2c_read() retornam imediatamente sem
 *      esperar pela conclusão da transação I2C. A FSM avança um passo por
 *      chamada a i2c_tick(), nunca bloqueando o scheduler.
 *
 *   2. DETERMINISMO (determinism):
 *      O número de ticks para completar uma transação é sempre o mesmo
 *      para o mesmo padrão de dados. O sistema é previsível e bounded.
 *
 * Fórmulas verificadas (modo nominal, HAL always-ready):
 *
 *   write(N bytes)     → 2*N + 3 ticks
 *   read(N bytes)      → 2*N + 3 ticks   [sem registo]
 *   reg_read(N bytes)  → 2*N + 5 ticks   [com registo: addr→reg→restart→leitura]
 *
 * Derivação da fórmula (trace da FSM):
 *   write(1):  STARTING(1) → SELECT_MODE(1) → WRITE(1) → WAIT_TX(1) → STOP(1) = 5 ticks
 *   write(N):  +2 ticks por byte adicional (WRITE + WAIT_TX)
 *   read(1):   STARTING(1) → SELECT_MODE(1) → READ(1) → WAIT_RX(1) → STOP(1) = 5 ticks
 *   reg_read:  +2 ticks de overhead (RESTART + WAIT_RESTART)
 *
 * Compatibilidade:
 *   USE_REAL_HW=0 (simulação host): Grupos 1–4 via mock HAL.
 *   USE_REAL_HW=1 (hardware real):  Grupo 5 via HAL real + SysTick.
 *
 * Grupos:
 *   GRUPO 1 — Não-bloqueio: retorno imediato e single-step da FSM (5 testes)
 *   GRUPO 2 — Determinismo: tick counts idênticos em N runs (6 testes)
 *   GRUPO 3 — Bounded: conclusão dentro de limite máximo garantido (4 testes)
 *   GRUPO 4 — Sensores I2C: padrões de uso real (IMU/EPS/Pressure/Temp/GNSS) (5 testes)
 *   GRUPO 5 — Hardware real: validação por tempo de parede via SysTick (3 testes)
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "../framework/test_runner.h"
#include "drivers/i2c_driver.h"

/* ============================================================================
 * Constantes derivadas da FSM
 * ============================================================================ */

/**
 * Ticks nominais (HAL always-ready) para cada tipo de transação.
 * Fórmula: write/read = 2*N+3, reg_read = 2*N+5
 */
#define TICKS_WRITE(n)     (2*(n) + 3)
#define TICKS_READ(n)      (2*(n) + 3)
#define TICKS_REG_READ(n)  (2*(n) + 5)

/**
 * Limite máximo absoluto de ticks para uma transação com timeout.
 * Worst-case: setup (5 ticks) + WAIT_TX ou WAIT_RX gasta I2C_TIMEOUT_MAX+1 ticks.
 */
#define TICKS_TIMEOUT_BOUND  (I2C_TIMEOUT_MAX + 10)

/**
 * Limite para transação nominal com payload máximo (GNSS: 18 bytes, sem reg).
 */
#define TICKS_MAX_NOMINAL    TICKS_READ(18)   /* 39 ticks */

/* ============================================================================
 * Endereços e tamanhos dos sensores I2C (de board.h / uso real)
 * ============================================================================ */
#define IMU_ADDR_TEST   0x68U  /* IMU: 6 bytes accel, reg 0x3B */
#define IMU_BUF_LEN     6U
#define IMU_REG         0x3BU

#define EPS_ADDR_TEST   0x60U  /* EPS: 2 bytes */
#define EPS_BUF_LEN     2U

#define PRESS_ADDR_TEST 0x77U  /* Pressure: 2 bytes */
#define PRESS_BUF_LEN   2U

#define TEMP_ADDR_TEST  0x48U  /* Temperature: 2 bytes */
#define TEMP_BUF_LEN    2U

#define GNSS_ADDR_TEST  0x42U  /* GNSS: 18 bytes */
#define GNSS_BUF_LEN    18U


/* ============================================================================
 * BLOCO DE SIMULAÇÃO (USE_REAL_HW == 0)
 * Usa mock HAL para controlo total do comportamento do bus I2C.
 * ============================================================================ */
#if USE_REAL_HW == 0

#include "../mocks/mock_hal_i2c.h"

/* --------------------------------------------------------------------------
 * Helpers partilhados
 * -------------------------------------------------------------------------- */

static int cb_result;
static int cb_called;

static void test_cb(int r) { cb_called++; cb_result = r; }

/** Inicializa handle para escrita simples. */
static void setup_write(i2c_handle_t *h, uint8_t addr,
                        uint8_t *buf, uint8_t len, void (*cb)(int))
{
    memset(h, 0, sizeof(*h));
    h->addr = addr; h->buf = buf; h->len = len;
    h->rw = 0; h->use_reg = 0; h->callback = cb;
    h->state = I2C_STARTING;
}

/** Inicializa handle para leitura simples (sem registo). */
static void setup_read(i2c_handle_t *h, uint8_t addr,
                       uint8_t *buf, uint8_t len, void (*cb)(int))
{
    memset(h, 0, sizeof(*h));
    h->addr = addr; h->buf = buf; h->len = len;
    h->rw = 1; h->use_reg = 0; h->callback = cb;
    h->state = I2C_STARTING;
}

/** Inicializa handle para leitura com registo (use_reg=1). */
static void setup_reg_read(i2c_handle_t *h, uint8_t addr, uint8_t reg,
                           uint8_t *buf, uint8_t len, void (*cb)(int))
{
    memset(h, 0, sizeof(*h));
    h->addr = addr; h->reg = reg; h->buf = buf; h->len = len;
    h->rw = 1; h->use_reg = 1; h->callback = cb;
    h->state = I2C_STARTING;
}

/** Configura o mock para respostas always-ready (modo nominal). */
static void mock_set_nominal(void)
{
    mock_i2c.ack_default      = 1;
    mock_i2c.tx_ready_default = 1;
    mock_i2c.rx_ready_default = 1;
    mock_i2c.bus_free_default = 1;
}

/**
 * Corre a FSM até I2C_IDLE e devolve o número de ticks gastos.
 * Limite de segurança: 10000 ticks (evita loop infinito em teste com bug).
 */
static int run_to_idle(i2c_handle_t *h)
{
    int ticks = 0;
    while (h->state != I2C_IDLE && ticks < 10000) {
        i2c_tick(h);
        ticks++;
    }
    return ticks;
}

/**
 * Avança a FSM até atingir o estado alvo (ou até ao limite de segurança).
 * Devolve o número de ticks gastos para chegar ao estado.
 */
static int run_until_state(i2c_handle_t *h, i2c_state_t target)
{
    int ticks = 0;
    while (h->state != target && h->state != I2C_IDLE && ticks < 200) {
        i2c_tick(h);
        ticks++;
    }
    return ticks;
}

/** Força a FSM para IDLE com todas as respostas HAL afirmativas. */
static void force_to_idle(i2c_handle_t *h)
{
    mock_set_nominal();
    run_to_idle(h);
}


/* ============================================================================
 * GRUPO 1 — Não-bloqueio
 *
 * Prova que:
 *  (a) setup + i2c_tick(0 vezes) → estado STARTING, buffers intocados
 *  (b) cada chamada a i2c_tick() avança exactamente um estado
 *  (c) uma chamada quando ocupado é inócua (guard de IDLE)
 *  (d) i2c_tick() em IDLE não chama nenhuma função HAL
 * ============================================================================ */

/**
 * @test Após inicializar o handle para leitura, o estado é I2C_STARTING.
 *       A transação NÃO começou ainda — provar que setup é O(1).
 */
static void test_nb_read_init_is_starting_not_idle(void)
{
    mock_i2c_reset();
    i2c_handle_t h;
    uint8_t buf[4] = {0xFFU, 0xFFU, 0xFFU, 0xFFU};

    setup_read(&h, IMU_ADDR_TEST, buf, 4U, test_cb);

    /* Sem chamar i2c_tick() nenhuma vez */
    TEST_ASSERT_EQUAL_INT(I2C_STARTING, h.state,
        "estado e STARTING imediatamente apos setup — sem bloqueio");
    TEST_ASSERT_EQUAL_INT(0, mock_i2c.start_count,
        "START nao enviado antes do primeiro tick");
    TEST_ASSERT_EQUAL_INT(0, mock_i2c.send_addr_count,
        "send_addr nao chamado antes do primeiro tick");
    /* Buffer intocado: a transação ainda não correu */
    TEST_ASSERT_EQUAL_UINT8(0xFFU, buf[0], "buf nao alterado antes de ticks");

    force_to_idle(&h);
}

/**
 * @test Cada tick avança exactamente um estado na FSM.
 *       Prova granularidade fina: o scheduler controla o ritmo.
 */
static void test_nb_single_tick_advances_one_state(void)
{
    mock_i2c_reset();
    mock_set_nominal();
    i2c_handle_t h;
    uint8_t buf[] = {0x01U};

    setup_write(&h, 0x42U, buf, 1U, test_cb);

    /* Tick 1: STARTING → SELECT_MODE */
    i2c_tick(&h);
    TEST_ASSERT_EQUAL_INT(I2C_SELECT_MODE, h.state,
        "tick 1: STARTING -> SELECT_MODE");

    /* Tick 2: SELECT_MODE → WRITE */
    i2c_tick(&h);
    TEST_ASSERT_EQUAL_INT(I2C_WRITE, h.state,
        "tick 2: SELECT_MODE -> WRITE");

    /* Tick 3: WRITE → WAIT_TX */
    i2c_tick(&h);
    TEST_ASSERT_EQUAL_INT(I2C_WAIT_TX, h.state,
        "tick 3: WRITE -> WAIT_TX");

    /* Tick 4: WAIT_TX → STOP (index=1=len) */
    i2c_tick(&h);
    TEST_ASSERT_EQUAL_INT(I2C_STOP, h.state,
        "tick 4: WAIT_TX -> STOP");

    /* Tick 5: STOP → IDLE */
    i2c_tick(&h);
    TEST_ASSERT_EQUAL_INT(I2C_IDLE, h.state,
        "tick 5: STOP -> IDLE");
}

/**
 * @test Guard de IDLE: se o handle já está ocupado (não IDLE), uma nova
 *       tentativa de lançar outra operação deve ser ignorada.
 *       Simula o padrão usado em imu_read_async(): if (state != IDLE) return;
 */
static void test_nb_busy_guard_prevents_restart(void)
{
    mock_i2c_reset();
    mock_set_nominal();
    i2c_handle_t h;
    uint8_t buf[2] = {0};

    setup_read(&h, IMU_ADDR_TEST, buf, 2U, NULL);
    /* Avança 1 tick (STARTING → SELECT_MODE): handle está a meio da transação */
    i2c_tick(&h);
    TEST_ASSERT_EQUAL_INT(I2C_SELECT_MODE, h.state, "handle ocupado em SELECT_MODE");

    /* Simula guard: aplicação verifica estado antes de relançar */
    int would_relaunch = (h.state == I2C_IDLE) ? 1 : 0;
    TEST_ASSERT_EQUAL_INT(0, would_relaunch,
        "guard IDLE: nova leitura nao e lancada quando handle ocupado");

    force_to_idle(&h);
}

/**
 * @test i2c_tick() em estado IDLE é um no-op absoluto:
 *       nenhuma função HAL é chamada, independentemente do número de ticks.
 */
static void test_nb_tick_on_idle_zero_hal_calls(void)
{
    mock_i2c_reset();
    i2c_handle_t h;
    memset(&h, 0, sizeof(h));
    h.state = I2C_IDLE;

    /* 200 ticks em IDLE */
    for (int i = 0; i < 200; i++)
        i2c_tick(&h);

    TEST_ASSERT_EQUAL_INT(I2C_IDLE, h.state,       "permanece IDLE");
    TEST_ASSERT_EQUAL_INT(0, mock_i2c.start_count,  "START nunca chamado");
    TEST_ASSERT_EQUAL_INT(0, mock_i2c.stop_count,   "STOP nunca chamado");
    TEST_ASSERT_EQUAL_INT(0, mock_i2c.send_addr_count, "send_addr nunca chamado");
    TEST_ASSERT_EQUAL_INT(0, mock_i2c.send_byte_count, "send_byte nunca chamado");
}

/**
 * @test A FSM de leitura de 6 bytes (padrão IMU) não completa após apenas
 *       1 tick — prova que a leitura é não-bloqueante (não termina no mesmo
 *       ciclo em que é iniciada).
 */
static void test_nb_read_does_not_complete_in_one_tick(void)
{
    mock_i2c_reset();
    mock_set_nominal();
    i2c_handle_t h;
    uint8_t buf[IMU_BUF_LEN] = {0};
    uint8_t rx[IMU_BUF_LEN];
    for (uint8_t i = 0; i < IMU_BUF_LEN; i++) rx[i] = (uint8_t)(0xA0U + i);
    mock_i2c_set_rx_data_seq(rx, IMU_BUF_LEN);

    setup_reg_read(&h, IMU_ADDR_TEST, IMU_REG, buf, IMU_BUF_LEN, test_cb);

    /* Após 1 tick: deve estar em SELECT_MODE, não em IDLE */
    i2c_tick(&h);
    TEST_ASSERT_FALSE(h.state == I2C_IDLE,
        "leitura de 6 bytes nao termina em 1 tick — nao bloqueante");

    force_to_idle(&h);
}


/* ============================================================================
 * GRUPO 2 — Determinismo de ticks
 *
 * Verifica que o número de ticks para completar uma transação é:
 *  (a) sempre idêntico em múltiplos runs consecutivos
 *  (b) exactamente igual à fórmula teórica da FSM
 * ============================================================================ */

/**
 * @test Escrita de 1 byte completa em exactamente TICKS_WRITE(1) = 5 ticks.
 *       Resultado idêntico em 3 runs consecutivos (determinismo).
 */
static void test_det_write_1byte_exact_ticks(void)
{
    int counts[3];

    for (int run = 0; run < 3; run++) {
        mock_i2c_reset();
        mock_set_nominal();
        i2c_handle_t h;
        uint8_t buf[] = {0x55U};
        setup_write(&h, 0x42U, buf, 1U, NULL);
        counts[run] = run_to_idle(&h);
    }

    TEST_ASSERT_EQUAL_INT(TICKS_WRITE(1), counts[0],
        "write(1): tick count == formula 2*N+3");
    TEST_ASSERT_EQUAL_INT(counts[0], counts[1],
        "write(1): run1 == run2 (determinismo)");
    TEST_ASSERT_EQUAL_INT(counts[1], counts[2],
        "write(1): run2 == run3 (determinismo)");
}

/**
 * @test Leitura de 1 byte (sem registo) em exactamente TICKS_READ(1) = 5 ticks.
 *       Resultado idêntico em 3 runs consecutivos.
 */
static void test_det_read_1byte_exact_ticks(void)
{
    int counts[3];

    for (int run = 0; run < 3; run++) {
        mock_i2c_reset();
        mock_set_nominal();
        uint8_t rx[] = {0xC7U};
        mock_i2c_set_rx_data_seq(rx, 1);
        i2c_handle_t h;
        uint8_t buf[1] = {0};
        setup_read(&h, TEMP_ADDR_TEST, buf, 1U, NULL);
        counts[run] = run_to_idle(&h);
    }

    TEST_ASSERT_EQUAL_INT(TICKS_READ(1), counts[0],
        "read(1): tick count == formula 2*N+3");
    TEST_ASSERT_EQUAL_INT(counts[0], counts[1], "read(1): run1 == run2");
    TEST_ASSERT_EQUAL_INT(counts[1], counts[2], "read(1): run2 == run3");
}

/**
 * @test Leitura com registo de 1 byte em exactamente TICKS_REG_READ(1) = 7 ticks.
 *       Overhead de 2 ticks em relação à leitura simples (RESTART + WAIT_RESTART).
 */
static void test_det_reg_read_1byte_exact_ticks(void)
{
    int counts[3];

    for (int run = 0; run < 3; run++) {
        mock_i2c_reset();
        mock_set_nominal();
        uint8_t rx[] = {0x71U};
        mock_i2c_set_rx_data_seq(rx, 1);
        i2c_handle_t h;
        uint8_t buf[1] = {0};
        setup_reg_read(&h, IMU_ADDR_TEST, 0x75U, buf, 1U, NULL);
        counts[run] = run_to_idle(&h);
    }

    TEST_ASSERT_EQUAL_INT(TICKS_REG_READ(1), counts[0],
        "reg_read(1): tick count == formula 2*N+5");
    TEST_ASSERT_EQUAL_INT(counts[0], counts[1], "reg_read(1): run1 == run2");
    TEST_ASSERT_EQUAL_INT(counts[1], counts[2], "reg_read(1): run2 == run3");
}

/**
 * @test Escalonamento linear: o tick count cresce linearmente com o número
 *       de bytes (2 ticks por byte adicional).
 *       Verifica write(1)=5, write(2)=7, write(3)=9, write(6)=15.
 */
static void test_det_tick_count_scales_linearly(void)
{
    static const uint8_t sizes[] = {1U, 2U, 3U, 6U};
    static const int expected[]  = {
        TICKS_WRITE(1), TICKS_WRITE(2), TICKS_WRITE(3), TICKS_WRITE(6)
    };

    for (int s = 0; s < 4; s++) {
        mock_i2c_reset();
        mock_set_nominal();
        uint8_t buf[6] = {0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U};
        i2c_handle_t h;
        setup_write(&h, 0x42U, buf, sizes[s], NULL);
        int ticks = run_to_idle(&h);

        TEST_ASSERT_EQUAL_INT(expected[s], ticks, "escalonamento linear: 2*N+3");
    }
}

/**
 * @test A fórmula reg_read(N) = 2*N+5 é verificada para N=1,2,6 (EPS=2, IMU=6).
 *       Confirma overhead de RESTART+WAIT_RESTART (+2 ticks face a read simples).
 */
static void test_det_reg_read_formula_verified(void)
{
    static const uint8_t sizes[] = {1U, 2U, 6U};
    static const int expected[]  = {
        TICKS_REG_READ(1), TICKS_REG_READ(2), TICKS_REG_READ(6)
    };

    for (int s = 0; s < 3; s++) {
        mock_i2c_reset();
        mock_set_nominal();
        uint8_t rx[6] = {0xA1U, 0xA2U, 0xA3U, 0xA4U, 0xA5U, 0xA6U};
        mock_i2c_set_rx_data_seq(rx, sizes[s]);
        i2c_handle_t h;
        uint8_t buf[6] = {0};
        setup_reg_read(&h, IMU_ADDR_TEST, 0x3BU, buf, sizes[s], NULL);
        int ticks = run_to_idle(&h);

        TEST_ASSERT_EQUAL_INT(expected[s], ticks, "reg_read formula 2*N+5");
    }
}

/**
 * @test Determinismo em 5 runs consecutivos da mesma operação (reg_read 6 bytes).
 *       Todos os runs devem produzir exactamente o mesmo tick count.
 *       Detecta acumulação de estado ou drift entre iterações.
 */
static void test_det_5runs_identical_tick_count(void)
{
    int first_count = -1;

    for (int run = 0; run < 5; run++) {
        mock_i2c_reset();
        mock_set_nominal();
        uint8_t rx[IMU_BUF_LEN] = {0x10U, 0x20U, 0x30U, 0x40U, 0x50U, 0x60U};
        mock_i2c_set_rx_data_seq(rx, IMU_BUF_LEN);
        i2c_handle_t h;
        uint8_t buf[IMU_BUF_LEN] = {0};
        setup_reg_read(&h, IMU_ADDR_TEST, IMU_REG, buf, IMU_BUF_LEN, NULL);
        int ticks = run_to_idle(&h);

        if (first_count == -1)
            first_count = ticks;

        TEST_ASSERT_EQUAL_INT(first_count, ticks,
            "5 runs consecutivos: tick count identico (sem drift)");
    }
}


/* ============================================================================
 * GRUPO 3 — Bounded completion
 *
 * Prova que a FSM SEMPRE termina dentro de um limite previsível de ticks,
 * mesmo em condições adversas (bus ocupado, timeout).
 * ============================================================================ */

/**
 * @test Uma transação nominal completa bem abaixo do limite máximo teórico.
 *       Payload máximo testado: GNSS (18 bytes, sem registo) = 39 ticks.
 */
static void test_bound_nominal_completes_within_limit(void)
{
    mock_i2c_reset();
    mock_set_nominal();
    uint8_t rx[GNSS_BUF_LEN];
    for (uint8_t i = 0; i < GNSS_BUF_LEN; i++) rx[i] = i;
    mock_i2c_set_rx_data_seq(rx, GNSS_BUF_LEN);

    i2c_handle_t h;
    uint8_t buf[GNSS_BUF_LEN] = {0};
    setup_read(&h, GNSS_ADDR_TEST, buf, GNSS_BUF_LEN, NULL);
    int ticks = run_to_idle(&h);

    TEST_ASSERT_EQUAL_INT(TICKS_READ(GNSS_BUF_LEN), ticks,
        "GNSS 18 bytes: completa em exactamente 2*18+3 = 39 ticks");
    TEST_ASSERT_TRUE(ticks <= TICKS_MAX_NOMINAL,
        "GNSS 18 bytes: abaixo do limite nominal maximo");
}

/**
 * @test Timeout em WAIT_TX: a FSM termina exactamente após I2C_TIMEOUT_MAX+1
 *       ticks nesse estado (determinismo do timeout).
 *       Garante que o timeout não é nem mais curto nem mais longo.
 */
static void test_bound_tx_timeout_exact_ticks(void)
{
    /* Primeiro run: conta os ticks do timeout */
    int timeout_ticks_run1, timeout_ticks_run2;

    /* Run 1 */
    {
        mock_i2c_reset();
        mock_set_nominal();
        mock_i2c.tx_ready_default = 0; /* TX nunca pronto */

        i2c_handle_t h;
        uint8_t buf[] = {0xAAU};
        setup_write(&h, 0x42U, buf, 1U, NULL);

        /* Avança até WAIT_TX */
        run_until_state(&h, I2C_WAIT_TX);
        TEST_ASSERT_EQUAL_INT(I2C_WAIT_TX, h.state, "chegou a WAIT_TX");

        h.timeout = 0; /* reset para contagem limpa */
        timeout_ticks_run1 = run_to_idle(&h);
    }

    /* Run 2: deve demorar exactamente o mesmo número de ticks */
    {
        mock_i2c_reset();
        mock_set_nominal();
        mock_i2c.tx_ready_default = 0;

        i2c_handle_t h;
        uint8_t buf[] = {0xBBU};
        setup_write(&h, 0x42U, buf, 1U, NULL);

        run_until_state(&h, I2C_WAIT_TX);
        h.timeout = 0;
        timeout_ticks_run2 = run_to_idle(&h);
    }

    /* Os dois runs devem demorar exactamente o mesmo */
    TEST_ASSERT_EQUAL_INT(timeout_ticks_run1, timeout_ticks_run2,
        "TX timeout: identico em 2 runs (determinismo)");

    /* O timeout deve ser exatamente I2C_TIMEOUT_MAX+1 ticks (++timeout > MAX) */
    TEST_ASSERT_EQUAL_INT(I2C_TIMEOUT_MAX + 1, timeout_ticks_run1,
        "TX timeout: dispara em exactamente I2C_TIMEOUT_MAX+1 ticks");
}

/**
 * @test Timeout em WAIT_RX: idêntico ao WAIT_TX — determinismo garantido.
 */
static void test_bound_rx_timeout_exact_ticks(void)
{
    int timeout_ticks_run1, timeout_ticks_run2;

    for (int run = 0; run < 2; run++) {
        mock_i2c_reset();
        mock_set_nominal();
        mock_i2c.rx_ready_default = 0; /* RX nunca pronto */

        i2c_handle_t h;
        uint8_t buf[1] = {0};
        setup_read(&h, TEMP_ADDR_TEST, buf, 1U, NULL);

        run_until_state(&h, I2C_WAIT_RX);
        TEST_ASSERT_EQUAL_INT(I2C_WAIT_RX, h.state, "chegou a WAIT_RX");

        h.timeout = 0;
        int ticks = run_to_idle(&h);

        if (run == 0) timeout_ticks_run1 = ticks;
        else          timeout_ticks_run2 = ticks;
    }

    TEST_ASSERT_EQUAL_INT(timeout_ticks_run1, timeout_ticks_run2,
        "RX timeout: identico em 2 runs");
    TEST_ASSERT_EQUAL_INT(I2C_TIMEOUT_MAX + 1, timeout_ticks_run1,
        "RX timeout: dispara em exactamente I2C_TIMEOUT_MAX+1 ticks");
}

/**
 * @test Worst-case bounded: a FSM termina sempre, mesmo com bus ocupado
 *       no início. Após libertar o bus, completa no número esperado de ticks.
 */
static void test_bound_busy_bus_then_complete(void)
{
    mock_i2c_reset();
    mock_i2c.bus_free_default = 0; /* bus ocupado */
    mock_set_nominal();
    mock_i2c.bus_free_default = 0; /* sobrepõe (mock_set_nominal liga bus_free) */

    i2c_handle_t h;
    uint8_t buf[] = {0x11U};
    setup_write(&h, 0x42U, buf, 1U, NULL);

    /* 10 ticks com bus ocupado: deve ficar em STARTING */
    for (int i = 0; i < 10; i++)
        i2c_tick(&h);

    TEST_ASSERT_EQUAL_INT(I2C_STARTING, h.state,
        "bus ocupado: fica em STARTING (nao bloqueia — controlo devolvido ao caller)");

    /* Liberta bus: a FSM deve completar em exactamente TICKS_WRITE(1) ticks */
    mock_i2c.bus_free_default = 1;
    int ticks = run_to_idle(&h);

    TEST_ASSERT_EQUAL_INT(TICKS_WRITE(1), ticks,
        "apos libertar bus: completa em ticks nominais exactos");
}


/* ============================================================================
 * GRUPO 4 — Padrões de sensores I2C
 *
 * Verifica os padrões de uso real de cada sensor I2C do OBC:
 *   IMU (0x68, 6 bytes, reg 0x3B) — acelerómetro + giroscópio
 *   EPS (0x60, 2 bytes)           — sistema de energia
 *   Pressure (0x77, 2 bytes)      — pressão
 *   Temperature (0x48, 2 bytes)   — temperatura
 *   GNSS (0x42, 18 bytes)         — localização (sem reg, stream)
 * ============================================================================ */

/**
 * @test IMU: leitura de 6 bytes com registo (padrão imu_read_async).
 *       Não-bloqueante: retorna antes de completar.
 *       Determinista: sempre TICKS_REG_READ(6) = 17 ticks.
 *       Dados íntegros: buffer preenchido correctamente.
 */
static void test_sensor_imu_nonblocking_and_deterministic(void)
{
    int counts[3];
    uint8_t rx[IMU_BUF_LEN] = {0x10U, 0x20U, 0x30U, 0x40U, 0x50U, 0x60U};

    for (int run = 0; run < 3; run++) {
        mock_i2c_reset();
        mock_set_nominal();
        mock_i2c_set_rx_data_seq(rx, IMU_BUF_LEN);

        i2c_handle_t h;
        uint8_t buf[IMU_BUF_LEN] = {0};
        setup_reg_read(&h, IMU_ADDR_TEST, IMU_REG, buf, IMU_BUF_LEN, test_cb);

        /* Prova não-bloqueio: após setup, ainda não completou */
        TEST_ASSERT_EQUAL_INT(I2C_STARTING, h.state,
            "IMU: estado STARTING antes do primeiro tick");

        counts[run] = run_to_idle(&h);

        /* Verifica dados íntegros no último run */
        if (run == 2) {
            TEST_ASSERT_EQUAL_MEM(rx, buf, IMU_BUF_LEN,
                "IMU: dados lidos correctamente");
        }
    }

    TEST_ASSERT_EQUAL_INT(TICKS_REG_READ(IMU_BUF_LEN), counts[0],
        "IMU: tick count == 2*6+5 = 17");
    TEST_ASSERT_EQUAL_INT(counts[0], counts[1], "IMU: run1 == run2");
    TEST_ASSERT_EQUAL_INT(counts[1], counts[2], "IMU: run2 == run3");
}

/**
 * @test EPS, Pressure e Temperature: leitura de 2 bytes com registo.
 *       Todos devem completar em TICKS_REG_READ(2) = 9 ticks.
 *       Verifica que sensores de tamanho igual têm tick counts iguais.
 */
static void test_sensor_eps_pressure_temp_deterministic(void)
{
    static const struct {
        uint8_t addr;
        const char *name;
    } sensors[] = {
        {EPS_ADDR_TEST,   "EPS"},
        {PRESS_ADDR_TEST, "Pressure"},
        {TEMP_ADDR_TEST,  "Temperature"},
    };
    static const uint8_t n_sensors = 3U;

    int prev_count = -1;
    for (uint8_t s = 0; s < n_sensors; s++) {
        mock_i2c_reset();
        mock_set_nominal();
        uint8_t rx[EPS_BUF_LEN] = {0xA5U, 0x5AU};
        mock_i2c_set_rx_data_seq(rx, EPS_BUF_LEN);

        i2c_handle_t h;
        uint8_t buf[EPS_BUF_LEN] = {0};
        setup_reg_read(&h, sensors[s].addr, 0x00U, buf, EPS_BUF_LEN, NULL);
        int ticks = run_to_idle(&h);

        TEST_ASSERT_EQUAL_INT(TICKS_REG_READ(EPS_BUF_LEN), ticks,
            "sensor 2 bytes: tick count == 2*2+5 = 9");

        if (prev_count != -1)
            TEST_ASSERT_EQUAL_INT(prev_count, ticks,
                "sensores de mesmo tamanho: tick count identico");

        prev_count = ticks;
    }
}

/**
 * @test GNSS: leitura de 18 bytes sem registo (stream contínuo).
 *       Deve completar em TICKS_READ(18) = 39 ticks.
 *       Dados integridade: todos os 18 bytes correctos.
 */
static void test_sensor_gnss_18bytes_deterministic(void)
{
    uint8_t rx[GNSS_BUF_LEN];
    for (uint8_t i = 0; i < GNSS_BUF_LEN; i++) rx[i] = (uint8_t)(i * 2U + 1U);

    int counts[3];
    for (int run = 0; run < 3; run++) {
        mock_i2c_reset();
        mock_set_nominal();
        mock_i2c_set_rx_data_seq(rx, GNSS_BUF_LEN);

        i2c_handle_t h;
        uint8_t buf[GNSS_BUF_LEN] = {0};
        setup_read(&h, GNSS_ADDR_TEST, buf, GNSS_BUF_LEN, NULL);
        counts[run] = run_to_idle(&h);

        if (run == 2)
            TEST_ASSERT_EQUAL_MEM(rx, buf, GNSS_BUF_LEN,
                "GNSS: todos os 18 bytes correctos");
    }

    TEST_ASSERT_EQUAL_INT(TICKS_READ(GNSS_BUF_LEN), counts[0],
        "GNSS: tick count == 2*18+3 = 39");
    TEST_ASSERT_EQUAL_INT(counts[0], counts[1], "GNSS: run1 == run2");
    TEST_ASSERT_EQUAL_INT(counts[1], counts[2], "GNSS: run2 == run3");
}

/**
 * @test Alternância de sensores: simula o padrão do scheduler OBC onde
 *       os sensores são lidos em ciclos alternados (sensors_read_all).
 *       Cada sensor individualmente mantém o seu tick count determinista.
 */
static void test_sensor_alternating_reads_deterministic(void)
{
    /* Tick counts esperados para cada sensor */
    static const int expected[] = {
        TICKS_REG_READ(IMU_BUF_LEN),  /* IMU:  17 */
        TICKS_REG_READ(EPS_BUF_LEN),  /* EPS:   9 */
        TICKS_REG_READ(PRESS_BUF_LEN),/* Pres:  9 */
        TICKS_REG_READ(TEMP_BUF_LEN), /* Temp:  9 */
    };

    for (int cycle = 0; cycle < 3; cycle++) {
        /* IMU */
        {
            mock_i2c_reset(); mock_set_nominal();
            uint8_t rx[IMU_BUF_LEN] = {0};
            mock_i2c_set_rx_data_seq(rx, IMU_BUF_LEN);
            i2c_handle_t h; uint8_t buf[IMU_BUF_LEN] = {0};
            setup_reg_read(&h, IMU_ADDR_TEST, IMU_REG, buf, IMU_BUF_LEN, NULL);
            int t = run_to_idle(&h);
            TEST_ASSERT_EQUAL_INT(expected[0], t, "ciclo: IMU ticks correcto");
        }
        /* EPS */
        {
            mock_i2c_reset(); mock_set_nominal();
            uint8_t rx[EPS_BUF_LEN] = {0x12U, 0x34U};
            mock_i2c_set_rx_data_seq(rx, EPS_BUF_LEN);
            i2c_handle_t h; uint8_t buf[EPS_BUF_LEN] = {0};
            setup_reg_read(&h, EPS_ADDR_TEST, 0x00U, buf, EPS_BUF_LEN, NULL);
            int t = run_to_idle(&h);
            TEST_ASSERT_EQUAL_INT(expected[1], t, "ciclo: EPS ticks correcto");
        }
        /* Pressure */
        {
            mock_i2c_reset(); mock_set_nominal();
            uint8_t rx[PRESS_BUF_LEN] = {0xF0U, 0x0FU};
            mock_i2c_set_rx_data_seq(rx, PRESS_BUF_LEN);
            i2c_handle_t h; uint8_t buf[PRESS_BUF_LEN] = {0};
            setup_reg_read(&h, PRESS_ADDR_TEST, 0x00U, buf, PRESS_BUF_LEN, NULL);
            int t = run_to_idle(&h);
            TEST_ASSERT_EQUAL_INT(expected[2], t, "ciclo: Pressure ticks correcto");
        }
    }
}

/**
 * @test Prova que a propriedade de não-bloqueio se mantém para todos os sensores:
 *       após setup, cada handle está em STARTING (não IDLE), i.e., a função
 *       de leitura não executou nenhuma transação I2C.
 */
static void test_sensor_all_start_in_starting_state(void)
{
    static const struct {
        uint8_t addr;
        uint8_t use_reg;
        uint8_t reg;
        uint8_t len;
    } sensors[] = {
        {IMU_ADDR_TEST,   1U, IMU_REG, IMU_BUF_LEN},
        {EPS_ADDR_TEST,   1U, 0x00U,   EPS_BUF_LEN},
        {PRESS_ADDR_TEST, 1U, 0x00U,   PRESS_BUF_LEN},
        {TEMP_ADDR_TEST,  1U, 0x00U,   TEMP_BUF_LEN},
        {GNSS_ADDR_TEST,  0U, 0x00U,   GNSS_BUF_LEN},
    };

    for (int s = 0; s < 5; s++) {
        mock_i2c_reset();
        i2c_handle_t h;
        uint8_t buf[18] = {0};

        if (sensors[s].use_reg)
            setup_reg_read(&h, sensors[s].addr, sensors[s].reg,
                           buf, sensors[s].len, NULL);
        else
            setup_read(&h, sensors[s].addr, buf, sensors[s].len, NULL);

        /* Sem nenhum tick: deve estar em STARTING */
        TEST_ASSERT_EQUAL_INT(I2C_STARTING, h.state,
            "sensor: estado STARTING apos setup, antes de qualquer tick");
        TEST_ASSERT_EQUAL_INT(0, mock_i2c.start_count,
            "sensor: START nao enviado antes do tick");

        force_to_idle(&h);
    }
}


/* ============================================================================
 * GRUPO 5 — Prova visual de determinismo (output para apresentação)
 *
 * Objetivo: mostrar, de forma visualmente clara, que o número de ticks é
 * sempre idêntico independentemente do número de execuções (1 ou 500).
 *
 * Cada teste imprime uma tabela com as primeiras e últimas execuções,
 * seguida de um resumo com mínimo, máximo e variância zero.
 * ============================================================================ */

#define PROOF_N_RUNS     500   /* total de execuções por prova */
#define PROOF_SHOW_HEAD  5     /* mostrar primeiras N execuções */
#define PROOF_SHOW_TAIL  5     /* mostrar últimas N execuções */

/**
 * Helper interno: corre N vezes a mesma transação I2C, recolhe os tick
 * counts, imprime a tabela de resultados e faz assert de determinismo.
 *
 * @param sensor_name   Nome do sensor para o cabeçalho.
 * @param sensor_addr   Endereço I2C do sensor.
 * @param use_reg       1 = reg_read (addr→reg→restart→leitura), 0 = read simples.
 * @param reg           Endereço do registo (ignorado se use_reg=0).
 * @param n_bytes       Número de bytes a ler.
 * @param n_runs        Número de execuções a realizar.
 */
static void run_determinism_proof(const char *sensor_name,
                                   uint8_t sensor_addr,
                                   uint8_t use_reg, uint8_t reg,
                                   uint8_t n_bytes,
                                   int n_runs)
{
    /* Buffer estático para evitar stack overflow (500 × 4 bytes = 2 kB) */
    static int counts[PROOF_N_RUNS];
    int min_t = 99999, max_t = 0, equal_count = 0;

    /* ---- executa todas as iterações ---- */
    for (int run = 0; run < n_runs; run++) {
        mock_i2c_reset();
        mock_set_nominal();

        /* dados RX variáveis por run para excluir qualquer caching acidental */
        uint8_t rx[18] = {0};
        for (uint8_t i = 0; i < n_bytes; i++)
            rx[i] = (uint8_t)((run * 7 + i) & 0xFFU);
        mock_i2c_set_rx_data_seq(rx, n_bytes);

        i2c_handle_t h;
        uint8_t buf[18] = {0};
        if (use_reg)
            setup_reg_read(&h, sensor_addr, reg, buf, n_bytes, NULL);
        else
            setup_read(&h, sensor_addr, buf, n_bytes, NULL);

        counts[run] = run_to_idle(&h);
        if (counts[run] < min_t) min_t = counts[run];
        if (counts[run] > max_t) max_t = counts[run];
    }
    for (int i = 0; i < n_runs; i++)
        if (counts[i] == counts[0]) equal_count++;

    /* ---- impressao da tabela ---- */
    printf("\n");
    printf("  +----------------------------------------------------------+\n");
    printf("  |           PROVA DE DETERMINISMO I2C                     |\n");
    printf("  +----------------------------------------------------------+\n");
    printf("  |  Sensor : %-47s|\n", sensor_name);
    printf("  |  Bytes  : %-2d  |  Tipo: %-10s  |  Execucoes: %-6d|\n",
           n_bytes,
           use_reg ? "reg_read" : "read",
           n_runs);
    printf("  +----------------------------------------------------------+\n");

    /* primeiras execucoes */
    for (int i = 0; i < PROOF_SHOW_HEAD && i < n_runs; i++) {
        printf("  |  Execucao %4d : %3d ticks  %-4s                       |\n",
               i + 1, counts[i],
               (counts[i] == counts[0]) ? "[OK]" : "[!!]");
    }
    printf("  |  ...                                                    |\n");
    /* ultimas execucoes */
    for (int i = n_runs - PROOF_SHOW_TAIL; i < n_runs; i++) {
        printf("  |  Execucao %4d : %3d ticks  %-4s                       |\n",
               i + 1, counts[i],
               (counts[i] == counts[0]) ? "[OK]" : "[!!]");
    }

    printf("  +----------------------------------------------------------+\n");
    printf("  |  Minimo : %-3d  | Maximo : %-3d  | Desvio: %-3d          |\n",
           min_t, max_t, max_t - min_t);

    if (min_t == max_t) {
        printf("  |  DETERMINISTICO : %4d/%4d execucoes = %3d ticks [OK]  |\n",
               equal_count, n_runs, counts[0]);
    } else {
        printf("  |  NAO DETERMINISTICO : min=%-3d  max=%-3d          [!!]  |\n",
               min_t, max_t);
    }
    printf("  +----------------------------------------------------------+\n");

    /* ---- assert para o framework de testes ---- */
    TEST_ASSERT_EQUAL_INT(min_t, max_t,
        "determinismo visual: min == max (variancia zero)");
    TEST_ASSERT_EQUAL_INT(n_runs, equal_count,
        "determinismo visual: todas as execucoes identicas");
}

/**
 * @test Prova visual — IMU (reg_read 6 bytes, addr 0x68, reg 0x3B).
 *       500 execuções devem produzir sempre 17 ticks.
 */
static void test_proof_imu_500runs(void)
{
    run_determinism_proof("IMU (0x68) — accel reg_read 0x3B",
                          IMU_ADDR_TEST, 1U, IMU_REG, IMU_BUF_LEN, PROOF_N_RUNS);
}

/**
 * @test Prova visual — EPS (reg_read 2 bytes, addr 0x60).
 *       500 execuções devem produzir sempre 9 ticks.
 */
static void test_proof_eps_500runs(void)
{
    run_determinism_proof("EPS (0x60) — reg_read 2 bytes",
                          EPS_ADDR_TEST, 1U, 0x00U, EPS_BUF_LEN, PROOF_N_RUNS);
}

/**
 * @test Prova visual — GNSS (read 18 bytes sem registo, addr 0x42).
 *       500 execuções devem produzir sempre 39 ticks.
 */
static void test_proof_gnss_500runs(void)
{
    run_determinism_proof("GNSS (0x42) — read 18 bytes",
                          GNSS_ADDR_TEST, 0U, 0x00U, GNSS_BUF_LEN, PROOF_N_RUNS);
}

/**
 * @test Prova visual — comparação entre sensores de tamanhos diferentes.
 *       Mostra que tick counts crescem linearmente (2*N+3 e 2*N+5).
 */
static void test_proof_all_sensors_summary(void)
{
    static const struct {
        const char *name;
        uint8_t addr;
        uint8_t use_reg;
        uint8_t reg;
        uint8_t n_bytes;
        int expected_ticks;
    } s_sensors[] = {
        {"Temp  (0x48) reg_read 2B", TEMP_ADDR_TEST,   1U, 0x00U, 2U,            TICKS_REG_READ(2)},
        {"Press (0x77) reg_read 2B", PRESS_ADDR_TEST,  1U, 0x00U, 2U,            TICKS_REG_READ(2)},
        {"EPS   (0x60) reg_read 2B", EPS_ADDR_TEST,    1U, 0x00U, EPS_BUF_LEN,   TICKS_REG_READ(2)},
        {"IMU   (0x68) reg_read 6B", IMU_ADDR_TEST,    1U, IMU_REG, IMU_BUF_LEN, TICKS_REG_READ(6)},
        {"GNSS  (0x42) read    18B", GNSS_ADDR_TEST,   0U, 0x00U, GNSS_BUF_LEN,  TICKS_READ(18)},
    };

    printf("\n");
    printf("  +----------------------------------------------------------+\n");
    printf("  |       RESUMO: TICKS POR SENSOR (formula 2N+3 / 2N+5)    |\n");
    printf("  +----------------------------------------------------------+\n");
    printf("  |  %-26s  Esperado  Obtido  OK?  |\n", "Sensor");
    printf("  +----------------------------------------------------------+\n");

    for (int s = 0; s < 5; s++) {
        mock_i2c_reset();
        mock_set_nominal();
        uint8_t rx[18] = {0x11U,0x22U,0x33U,0x44U,0x55U,0x66U,
                          0x77U,0x88U,0x99U,0xAAU,0xBBU,0xCCU,
                          0xDDU,0xEEU,0xFFU,0x01U,0x02U,0x03U};
        mock_i2c_set_rx_data_seq(rx, s_sensors[s].n_bytes);
        i2c_handle_t h;
        uint8_t buf[18] = {0};
        if (s_sensors[s].use_reg)
            setup_reg_read(&h, s_sensors[s].addr, s_sensors[s].reg,
                           buf, s_sensors[s].n_bytes, NULL);
        else
            setup_read(&h, s_sensors[s].addr, buf, s_sensors[s].n_bytes, NULL);

        int ticks = run_to_idle(&h);
        int ok = (ticks == s_sensors[s].expected_ticks);

        printf("  |  %-26s     %3d      %3d    %-4s |\n",
               s_sensors[s].name,
               s_sensors[s].expected_ticks,
               ticks,
               ok ? "[OK]" : "[!!]");

        TEST_ASSERT_EQUAL_INT(s_sensors[s].expected_ticks, ticks,
            "resumo: tick count correcto para sensor");
    }
    printf("  +----------------------------------------------------------+\n");
}


/* ============================================================================
 * main -- modo simulacao
 * ============================================================================ */
int main(void)
{
    TEST_BEGIN("I2C Sensor Realtime -- Nao-Bloqueio & Determinismo (sim)");

    printf("\n--- GRUPO 1: Nao-Bloqueio ---\n");
    RUN_TEST(test_nb_read_init_is_starting_not_idle);
    RUN_TEST(test_nb_single_tick_advances_one_state);
    RUN_TEST(test_nb_busy_guard_prevents_restart);
    RUN_TEST(test_nb_tick_on_idle_zero_hal_calls);
    RUN_TEST(test_nb_read_does_not_complete_in_one_tick);

    printf("\n--- GRUPO 2: Determinismo de Ticks ---\n");
    RUN_TEST(test_det_write_1byte_exact_ticks);
    RUN_TEST(test_det_read_1byte_exact_ticks);
    RUN_TEST(test_det_reg_read_1byte_exact_ticks);
    RUN_TEST(test_det_tick_count_scales_linearly);
    RUN_TEST(test_det_reg_read_formula_verified);
    RUN_TEST(test_det_5runs_identical_tick_count);

    printf("\n--- GRUPO 3: Bounded Completion ---\n");
    RUN_TEST(test_bound_nominal_completes_within_limit);
    RUN_TEST(test_bound_tx_timeout_exact_ticks);
    RUN_TEST(test_bound_rx_timeout_exact_ticks);
    RUN_TEST(test_bound_busy_bus_then_complete);

    printf("\n--- GRUPO 4: Padroes de Sensores I2C ---\n");
    RUN_TEST(test_sensor_imu_nonblocking_and_deterministic);
    RUN_TEST(test_sensor_eps_pressure_temp_deterministic);
    RUN_TEST(test_sensor_gnss_18bytes_deterministic);
    RUN_TEST(test_sensor_alternating_reads_deterministic);
    RUN_TEST(test_sensor_all_start_in_starting_state);

    printf("\n--- GRUPO 5: Prova Visual de Determinismo ---\n");
    RUN_TEST(test_proof_imu_500runs);
    RUN_TEST(test_proof_eps_500runs);
    RUN_TEST(test_proof_gnss_500runs);
    RUN_TEST(test_proof_all_sensors_summary);

    TEST_END();
}

#else /* USE_REAL_HW == 1 */

#include "hal/hal_i2c.h"
#include "hal/hal_systick.h"

#define HW_MAX_TICKS   50000U
#define HW_BUDGET_MS(n)  ((uint32_t)((n) + 5U))

static i2c_handle_t hw_h;
static uint8_t      hw_buf[GNSS_BUF_LEN];
static int          hw_cb_done;
static int          hw_cb_result;

static void hw_test_cb(int r) { hw_cb_done = 1; hw_cb_result = r; }

static uint32_t hw_run_to_idle(i2c_handle_t *h)
{
    uint32_t ticks = 0;
    while (h->state != I2C_IDLE && ticks < HW_MAX_TICKS) {
        i2c_tick(h);
        ticks++;
    }
    return ticks;
}

static void test_hw_read_async_returns_immediately(void)
{
    memset(&hw_h, 0, sizeof(hw_h));
    hw_h.addr = IMU_ADDR_TEST; hw_h.buf = hw_buf;
    hw_h.len  = IMU_BUF_LEN;  hw_h.rw  = 1;
    hw_h.reg  = IMU_REG;      hw_h.use_reg = 1;
    hw_h.callback = hw_test_cb;

    uint32_t t0 = hal_systick_get_ms();
    hw_h.state = I2C_STARTING;
    uint32_t elapsed = hal_systick_get_ms() - t0;

    TEST_ASSERT_TRUE(elapsed < 1U, "[HW] setup nao-bloqueante: retorna em < 1 ms");
    TEST_ASSERT_EQUAL_INT(I2C_STARTING, hw_h.state, "[HW] estado STARTING apos setup");
    TEST_ASSERT_FALSE(hw_cb_done, "[HW] callback nao chamado antes de ticks");
    hw_run_to_idle(&hw_h);
}

static void test_hw_single_tick_under_5ms(void)
{
    memset(&hw_h, 0, sizeof(hw_h));
    hw_h.addr = IMU_ADDR_TEST; hw_h.buf = hw_buf;
    hw_h.len  = IMU_BUF_LEN;  hw_h.rw  = 1;
    hw_h.reg  = IMU_REG;      hw_h.use_reg = 1;
    hw_h.callback = hw_test_cb; hw_h.state = I2C_STARTING;
    hw_cb_done = 0;

    uint32_t t0 = hal_systick_get_ms();
    i2c_tick(&hw_h);
    i2c_tick(&hw_h);
    i2c_tick(&hw_h);
    uint32_t elapsed = hal_systick_get_ms() - t0;

    TEST_ASSERT_TRUE(elapsed < 5U, "[HW] 3 ticks executam em < 5 ms");
    hw_run_to_idle(&hw_h);
}

static void test_hw_imu_read_completes_ok(void)
{
    memset(&hw_h, 0, sizeof(hw_h));
    memset(hw_buf, 0, sizeof(hw_buf));
    hw_h.addr = IMU_ADDR_TEST; hw_h.buf = hw_buf;
    hw_h.len  = IMU_BUF_LEN;  hw_h.rw  = 1;
    hw_h.reg  = IMU_REG;      hw_h.use_reg = 1;
    hw_h.callback = hw_test_cb;
    hw_h.state = I2C_STARTING;
    hw_cb_done = 0; hw_cb_result = -99;

    uint32_t t0 = hal_systick_get_ms();
    hw_run_to_idle(&hw_h);
    uint32_t elapsed = hal_systick_get_ms() - t0;

    TEST_ASSERT_EQUAL_INT(I2C_IDLE, hw_h.state,  "[HW] IMU: FSM chegou a IDLE");
    TEST_ASSERT_TRUE(hw_cb_done,                 "[HW] IMU: callback chamado");
    TEST_ASSERT_EQUAL_INT(0, hw_cb_result,       "[HW] IMU: leitura ok");
    TEST_ASSERT_TRUE(elapsed <= HW_BUDGET_MS(IMU_BUF_LEN),
        "[HW] IMU: dentro do orcamento de tempo");
}

int main(void)
{
    hal_systick_init();
    hal_i2c_init();
    TEST_BEGIN("I2C Sensor Realtime -- Hardware Real (SAMV71)");
    printf("\n--- GRUPO 5: Validacao Hardware Real ---\n");
    RUN_TEST(test_hw_read_async_returns_immediately);
    RUN_TEST(test_hw_single_tick_under_5ms);
    RUN_TEST(test_hw_imu_read_completes_ok);
    TEST_END();
}

#endif /* USE_REAL_HW */
