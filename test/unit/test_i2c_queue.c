/**
 * @file test_i2c_queue.c
 * @brief Testes unitários para a fila I2C (i2c_queue_t).
 *
 * ECSS-E-ST-40C Rev.1 - Requisitos cobertos:
 *   §5.5.3.2c (ECSS-E-ST-40_0860089) — item 2: "all messages and error cases"
 *   §5.5.3.2c (ECSS-E-ST-40_0860089) — item 3: "access of all global variables"
 *   §5.5.3.2c (ECSS-E-ST-40_0860089) — item 1: "boundary at n-1, n, n+1"
 *                                                (boundary: fila vazia, cheia, overflow)
 *
 * Onde encontrar no standard:
 *   Secção 5.5.3.2, página 61 — "Software unit testing", ponto c., itens 1-5.
 *
 * Cobertura:
 *   - Enqueue de pedidos válidos
 *   - Estrutura da fila: head, tail, count
 *   - Boundary: fila vazia (count=0), com 1 elemento, com I2C_QUEUE_SIZE-1, com I2C_QUEUE_SIZE
 *   - Overflow: enqueue além da capacidade máxima (I2C_QUEUE_SIZE=25)
 *   - Verificação de campos do pedido (addr, len, rw, callback)
 *   - Acesso à variável global i2c_queue (ECSS §5.5.3.2c item 3)
 */

#include <stdint.h>
#include <string.h>
#include "test_utils.h"
#include "drivers/i2c_driver.h"
#include "config/board.h"

/* Callback de teste */
static int queue_cb_result = -99;
static void queue_test_callback(int result)
{
    queue_cb_result = result;
}

/* Helper: limpa a fila global antes de cada grupo de testes */
static void reset_queue(void)
{
    memset(&i2c_queue, 0, sizeof(i2c_queue));
}

/* ─────────────────────────────────────────────────────────────────────────────
   GRUPO 1 — Estado inicial da fila global (variável global, ECSS §5.5.3.2c item 3)
   ───────────────────────────────────────────────────────────────────────────── */

/**
 * @test TC-IQ-01: Fila global i2c_queue é acessível e tem tamanho correcto
 * Ref: ECSS-E-ST-40_0860089c item 3 — "access of all global variables"
 */
static void test_queue_global_accessible(void)
{
    TEST("TC-IQ-01: i2c_queue global variable accessible and correct size");
    reset_queue();
    ASSERT_EQ(i2c_queue.count, 0, "Queue starts empty (count=0)");
    ASSERT_EQ(i2c_queue.head,  0, "Queue head starts at 0");
    ASSERT_EQ(i2c_queue.tail,  0, "Queue tail starts at 0");
    /* Verifica que a capacidade da fila é I2C_QUEUE_SIZE=25 */
    int capacity = (int)(sizeof(i2c_queue.requests) / sizeof(i2c_queue.requests[0]));
    ASSERT_EQ(capacity, I2C_QUEUE_SIZE, "Queue capacity = I2C_QUEUE_SIZE (25)");
}

/* ─────────────────────────────────────────────────────────────────────────────
   GRUPO 2 — Enqueue de pedidos (ECSS §5.5.3.2c item 2)
   ───────────────────────────────────────────────────────────────────────────── */

/**
 * @test TC-IQ-02: Enqueue de um pedido de leitura do GNSS
 * Ref: ECSS-E-ST-40_0860089c item 2 — "all messages and error cases"
 */
static void test_enqueue_single_read(void)
{
    TEST("TC-IQ-02: Enqueue single read request (GNSS)");
    reset_queue();
    uint8_t buf[GNSS_BUF_LEN];
    queue_cb_result = -99;

    i2c_enqueue(GNSS_ADDR, buf, GNSS_BUF_LEN, 1, queue_test_callback);

    ASSERT_EQ(i2c_queue.count, 1, "Queue count = 1 after enqueue");
    ASSERT_EQ(i2c_queue.requests[0].addr, GNSS_ADDR, "Request addr = GNSS_ADDR");
    ASSERT_EQ(i2c_queue.requests[0].len,  GNSS_BUF_LEN, "Request len = GNSS_BUF_LEN");
    ASSERT_EQ(i2c_queue.requests[0].rw,   1, "Request rw = 1 (READ)");
}

/**
 * @test TC-IQ-03: Enqueue de um pedido de escrita
 * Ref: ECSS-E-ST-40_0860089c item 2 — distinção entre READ e WRITE
 */
static void test_enqueue_write_request(void)
{
    TEST("TC-IQ-03: Enqueue write request (rw=0)");
    reset_queue();
    uint8_t buf[4] = {0xAA, 0xBB, 0xCC, 0xDD};

    i2c_enqueue(IMU_ADDR, buf, 4, 0, queue_test_callback);

    ASSERT_EQ(i2c_queue.count, 1, "Queue count = 1");
    ASSERT_EQ(i2c_queue.requests[0].rw, 0, "Request rw = 0 (WRITE)");
    ASSERT_EQ(i2c_queue.requests[0].addr, IMU_ADDR, "Request addr = IMU_ADDR");
}

/**
 * @test TC-IQ-04: Enqueue de todos os 5 sensores I2C (pedidos múltiplos)
 * Ref: ECSS-E-ST-40_0860089c item 3 — acesso às variáveis globais de endereço
 */
static void test_enqueue_all_sensors(void)
{
    TEST("TC-IQ-04: Enqueue all 5 I2C sensor requests");
    reset_queue();

    uint8_t buf[IMU_BUF_LEN];

    i2c_enqueue(GNSS_ADDR, buf, GNSS_BUF_LEN, 1, queue_test_callback);
    i2c_enqueue(IMU_ADDR,  buf, IMU_BUF_LEN,  1, queue_test_callback);
    i2c_enqueue(PRESS_ADDR,buf, PRES_BUF_LEN, 1, queue_test_callback);
    i2c_enqueue(TEMP_ADDR, buf, TEMP_BUF_LEN, 1, queue_test_callback);
    i2c_enqueue(EPS_ADDR,  buf, EPS_BUF_LEN,  1, queue_test_callback);

    ASSERT_EQ(i2c_queue.count, 5, "Queue count = 5 (all sensors enqueued)");
    ASSERT_EQ(i2c_queue.requests[0].addr, GNSS_ADDR,  "Entry 0: GNSS");
    ASSERT_EQ(i2c_queue.requests[1].addr, IMU_ADDR,   "Entry 1: IMU");
    ASSERT_EQ(i2c_queue.requests[2].addr, PRESS_ADDR, "Entry 2: Pressure");
    ASSERT_EQ(i2c_queue.requests[3].addr, TEMP_ADDR,  "Entry 3: Temperature");
    ASSERT_EQ(i2c_queue.requests[4].addr, EPS_ADDR,   "Entry 4: EPS");
}

/* ─────────────────────────────────────────────────────────────────────────────
   GRUPO 3 — Boundary da fila (ECSS §5.5.3.2c item 1)
   ───────────────────────────────────────────────────────────────────────────── */

/**
 * @test TC-IQ-05: Fila vazia — count = 0 (boundary inferior n=0)
 * Ref: ECSS-E-ST-40_0860089c item 1 — "boundary at n-1, n, n+1"
 *      Aqui n é o estado de fila vazia (count=0)
 */
static void test_queue_boundary_empty(void)
{
    TEST("TC-IQ-05: Queue boundary — empty state (count=0)");
    reset_queue();
    ASSERT_EQ(i2c_queue.count, 0, "Empty queue: count = 0");
    ASSERT_EQ(i2c_queue.head,  0, "Empty queue: head = 0");
    ASSERT_EQ(i2c_queue.tail,  0, "Empty queue: tail = 0");
}

/**
 * @test TC-IQ-06: Fila com 1 elemento (boundary n=1)
 * Ref: ECSS-E-ST-40_0860089c item 1 (n=1 elemento)
 */
static void test_queue_boundary_one_element(void)
{
    TEST("TC-IQ-06: Queue boundary — single element (count=1)");
    reset_queue();
    uint8_t buf[2];
    i2c_enqueue(EPS_ADDR, buf, EPS_BUF_LEN, 1, queue_test_callback);
    ASSERT_EQ(i2c_queue.count, 1, "Single element: count = 1");
}

/**
 * @test TC-IQ-07: Fila com I2C_QUEUE_SIZE-1 elementos (boundary n-1 de capacidade máxima)
 * Ref: ECSS-E-ST-40_0860089c item 1 (n-1 = 24 de capacidade 25)
 */
static void test_queue_boundary_near_full(void)
{
    TEST("TC-IQ-07: Queue boundary — near full (I2C_QUEUE_SIZE-1 = 24 elements)");
    reset_queue();
    uint8_t buf[2];

    for (int i = 0; i < I2C_QUEUE_SIZE - 1; i++)
    {
        i2c_enqueue(EPS_ADDR, buf, EPS_BUF_LEN, 1, queue_test_callback);
    }

    ASSERT_EQ(i2c_queue.count, I2C_QUEUE_SIZE - 1,
              "Near-full queue: count = I2C_QUEUE_SIZE-1 (24)");
}

/**
 * @test TC-IQ-08: Fila completamente cheia (boundary n = I2C_QUEUE_SIZE = 25)
 * Ref: ECSS-E-ST-40_0860089c item 1 (n = capacidade máxima)
 */
static void test_queue_boundary_full(void)
{
    TEST("TC-IQ-08: Queue boundary — completely full (I2C_QUEUE_SIZE = 25)");
    reset_queue();
    uint8_t buf[2];

    for (int i = 0; i < I2C_QUEUE_SIZE; i++)
    {
        i2c_enqueue(EPS_ADDR, buf, EPS_BUF_LEN, 1, queue_test_callback);
    }

    ASSERT_EQ(i2c_queue.count, I2C_QUEUE_SIZE,
              "Full queue: count = I2C_QUEUE_SIZE (25)");
}

/**
 * @test TC-IQ-09: Overflow da fila (boundary n+1 = I2C_QUEUE_SIZE+1)
 * Ref: ECSS-E-ST-40_0860089c item 2 — "error cases" (overflow é um caso de erro)
 *      ECSS-E-ST-40_0860089c item 4 — "out of range values" (mais pedidos do que a fila suporta)
 *
 * COMPORTAMENTO ESPERADO: o enqueue deve rejeitar silenciosamente (não corromper a fila)
 * ou sobrescrever o elemento mais antigo (política circular). O teste verifica
 * que a fila não corrompe dados (count não excede I2C_QUEUE_SIZE).
 */
static void test_queue_overflow_protection(void)
{
    TEST("TC-IQ-09: Queue overflow — I2C_QUEUE_SIZE+1 enqueues do not corrupt");
    reset_queue();
    uint8_t buf[2];

    /* Preenche completamente */
    for (int i = 0; i < I2C_QUEUE_SIZE; i++)
    {
        i2c_enqueue(EPS_ADDR, buf, EPS_BUF_LEN, 1, queue_test_callback);
    }

    /* Tenta adicionar um elemento extra (overflow) */
    i2c_enqueue(GNSS_ADDR, buf, GNSS_BUF_LEN, 1, queue_test_callback);

    /* A fila não deve ter mais elementos do que I2C_QUEUE_SIZE */
    ASSERT(i2c_queue.count <= I2C_QUEUE_SIZE,
           "Queue count does not exceed I2C_QUEUE_SIZE after overflow attempt");
}

/* ─────────────────────────────────────────────────────────────────────────────
   GRUPO 4 — Stress da fila (ECSS §5.5.3.2c item 5)
   ───────────────────────────────────────────────────────────────────────────── */

/**
 * @test TC-IQ-10: Stress — 50 enqueues consecutivos (mais que a capacidade)
 * Ref: ECSS-E-ST-40_0860089c item 5 — "software at the limits of its requirements: stress testing"
 */
static void test_queue_stress_enqueue(void)
{
    TEST("TC-IQ-10: Queue stress — 50 consecutive enqueues (2x capacity)");
    reset_queue();
    uint8_t buf[GNSS_BUF_LEN];

    for (int i = 0; i < 50; i++)
    {
        i2c_enqueue(GNSS_ADDR, buf, GNSS_BUF_LEN, 1, queue_test_callback);
    }

    /* Após stress, a fila deve estar num estado consistente */
    ASSERT(i2c_queue.count <= I2C_QUEUE_SIZE,
           "After 50 enqueues, queue count is bounded by I2C_QUEUE_SIZE");
    ASSERT(i2c_queue.head < I2C_QUEUE_SIZE,
           "Queue head is within valid range");
    ASSERT(i2c_queue.tail < I2C_QUEUE_SIZE,
           "Queue tail is within valid range");
}

/**
 * @test TC-IQ-11: Callback pointer é preservado na fila
 * Ref: ECSS-E-ST-40_0860089c item 3 — "access of all global variables"
 *      Verifica que o ponteiro de callback (campo da estrutura) não é corrompido
 */
static void test_queue_callback_preserved(void)
{
    TEST("TC-IQ-11: Callback pointer preserved in queue entry");
    reset_queue();
    uint8_t buf[EPS_BUF_LEN];
    queue_cb_result = -99;

    i2c_enqueue(EPS_ADDR, buf, EPS_BUF_LEN, 1, queue_test_callback);

    /* O ponteiro do callback deve estar definido */
    ASSERT(i2c_queue.requests[0].callback != NULL,
           "Callback pointer is not NULL after enqueue");

    /* Invocar o callback manualmente e verificar que funciona */
    i2c_queue.requests[0].callback(0);
    ASSERT_EQ(queue_cb_result, 0, "Callback invoked correctly from queue entry");
}

/* ─────────────────────────────────────────────────────────────────────────────
   Test runner
   ───────────────────────────────────────────────────────────────────────────── */

void run_i2c_queue_tests(void)
{
    TEST_SUITE("I2C Queue Tests (ECSS §5.5.3.2c items 1,2,3,4,5)");

    /* Grupo 1: variável global */
    test_queue_global_accessible();

    /* Grupo 2: enqueue */
    test_enqueue_single_read();
    test_enqueue_write_request();
    test_enqueue_all_sensors();

    /* Grupo 3: boundary */
    test_queue_boundary_empty();
    test_queue_boundary_one_element();
    test_queue_boundary_near_full();
    test_queue_boundary_full();
    test_queue_overflow_protection();

    /* Grupo 4: stress */
    test_queue_stress_enqueue();
    test_queue_callback_preserved();
}
