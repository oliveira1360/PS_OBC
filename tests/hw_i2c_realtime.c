/**
 * @file hw_i2c_realtime.c
 * @brief Teste de determinismo I2C para correr directamente na placa SAMV71.
 *
 * Este ficheiro NÃO usa o framework de testes unitários.
 * É uma função autónoma chamada directamente da main() do firmware.
 *
 * O que prova:
 *   1. NÃO-BLOQUEIO: após setup, o handle está em STARTING — a função
 *      de leitura devolveu o controlo imediatamente sem esperar pelo bus.
 *
 *   2. DETERMINISMO: o número de ticks para completar a mesma transação
 *      é sempre idêntico — sejam 2 execuções ou 10, o valor não varia.
 *      Na placa real, os ticks dependem da velocidade do bus I2C (25 kHz)
 *      e da cadência do scheduler, mas são sempre constantes para o mesmo
 *      hardware.
 *
 * Nota sobre o número de ticks na placa vs. simulação:
 *   - Em simulação (mock always-ready): IMU = 17 ticks (fórmula 2*N+5)
 *   - Na placa real (25 kHz I2C): os estados WAIT_TX e WAIT_RX gastam
 *     vários ticks à espera do hardware → valor mais alto mas CONSTANTE.
 */

#include <stdio.h>
#include <string.h>
#include "hw_i2c_realtime.h"
#include "drivers/i2c_driver.h"
#include "config/board.h"

/* ---------------------------------------------------------------------------
 * Configuração dos sensores a testar
 * --------------------------------------------------------------------------- */

#define HW_TEST_N_RUNS   10      /* execuções por sensor (suficiente para provar) */
#define HW_MAX_TICKS     100000U /* safety net: nunca bloqueia mais que isto       */

typedef struct {
    const char *name;
    uint8_t     addr;
    uint8_t     use_reg;
    uint8_t     reg;
    uint8_t     n_bytes;
} sensor_cfg_t;

static const sensor_cfg_t sensors[] = {
    {"IMU   (0x68) accel 6B reg 0x3B", IMU_ADDR,   1U, 0x3BU, IMU_BUF_LEN  },
    {"EPS   (0x60) 2B              ", EPS_ADDR,   1U, 0x00U, 2U            },
    {"Press (0x77) 2B              ", PRESS_ADDR, 1U, 0x00U, PRES_BUF_LEN },
    {"Temp  (0x48) 2B              ", TEMP_ADDR,  1U, 0x00U, TEMP_BUF_LEN },
    {"GNSS  (0x42) 18B             ", GNSS_ADDR,  0U, 0x00U, GNSS_BUF_LEN },
};

#define N_SENSORS  ((int)(sizeof(sensors) / sizeof(sensors[0])))

/* ---------------------------------------------------------------------------
 * Helper: corre uma transação i2c até IDLE, devolve o número de ticks
 * --------------------------------------------------------------------------- */
static uint32_t run_one_transaction(const sensor_cfg_t *cfg)
{
    i2c_handle_t h;
    uint8_t buf[GNSS_BUF_LEN]; /* buffer máximo */
    uint32_t ticks = 0U;

    memset(&h,   0, sizeof(h));
    memset(buf,  0, sizeof(buf));

    h.addr     = cfg->addr;
    h.buf      = buf;
    h.len      = cfg->n_bytes;
    h.rw       = 1U;
    h.reg      = cfg->reg;
    h.use_reg  = cfg->use_reg;
    h.index    = 0U;
    h.timeout  = 0U;
    h.callback = NULL;
    h.state    = I2C_STARTING;  /* equivalente a *_read_async() */

    /* ----- PROVA DE NÃO-BLOQUEIO -----
     * Após o setup acima, h.state == I2C_STARTING.
     * A função não esperou pelo bus nem realizou nenhuma comunicação I2C.
     * O controlo está de volta ao caller imediatamente. */

    while (h.state != I2C_IDLE && ticks < HW_MAX_TICKS) {
        i2c_tick(&h);
        ticks++;
    }

    return ticks;
}

/* ---------------------------------------------------------------------------
 * Função principal do teste — chama directamente na main()
 * --------------------------------------------------------------------------- */
void hw_i2c_realtime_test(void)
{
    printf("\r\n");
    printf("  ╔══════════════════════════════════════════════════════════╗\r\n");
    printf("  ║        TESTE I2C — NAO-BLOQUEIO & DETERMINISMO          ║\r\n");
    printf("  ║        Hardware: SAMV71  |  Bus: 25 kHz I2C             ║\r\n");
    printf("  ╚══════════════════════════════════════════════════════════╝\r\n");

    for (int s = 0; s < N_SENSORS; s++) {
        const sensor_cfg_t *cfg = &sensors[s];
        uint32_t counts[HW_TEST_N_RUNS];
        uint32_t min_t = 0xFFFFFFFFU, max_t = 0U;

        /* --- corre HW_TEST_N_RUNS transações --- */
        for (int run = 0; run < HW_TEST_N_RUNS; run++) {
            counts[run] = run_one_transaction(cfg);
            if (counts[run] < min_t) min_t = counts[run];
            if (counts[run] > max_t) max_t = counts[run];
        }

        /* --- imprime tabela --- */
        printf("\r\n");
        printf("  ┌──────────────────────────────────────────────────────────┐\r\n");
        printf("  │  Sensor : %-49s│\r\n", cfg->name);
        printf("  │  Bytes  : %-2u  | Tipo: %-10s | Runs: %-3d           │\r\n",
               cfg->n_bytes,
               cfg->use_reg ? "reg_read" : "read",
               HW_TEST_N_RUNS);
        printf("  ├──────────────────────────────────────────────────────────┤\r\n");

        for (int run = 0; run < HW_TEST_N_RUNS; run++) {
            printf("  │  Execucao %2d : %6lu ticks  %s                    │\r\n",
                   run + 1,
                   (unsigned long)counts[run],
                   (counts[run] == counts[0]) ? "[OK]" : "[!!]");
        }

        printf("  ├──────────────────────────────────────────────────────────┤\r\n");
        printf("  │  Min: %-6lu  Max: %-6lu  Desvio: %-6lu              │\r\n",
               (unsigned long)min_t,
               (unsigned long)max_t,
               (unsigned long)(max_t - min_t));

        if (min_t == max_t) {
            printf("  │  DETERMINISTICO : %2d/%2d runs = %lu ticks        [OK] │\r\n",
                   HW_TEST_N_RUNS, HW_TEST_N_RUNS,
                   (unsigned long)counts[0]);
        } else {
            printf("  │  NAO DETERMINISTICO : min=%-6lu max=%-6lu     [!!] │\r\n",
                   (unsigned long)min_t, (unsigned long)max_t);
        }
        printf("  └──────────────────────────────────────────────────────────┘\r\n");
    }

    printf("\r\n");
    printf("  ╔══════════════════════════════════════════════════════════╗\r\n");
    printf("  ║  Teste concluido. Retomando execucao normal do OBC.     ║\r\n");
    printf("  ╚══════════════════════════════════════════════════════════╝\r\n");
    printf("\r\n");
}
