/**
 * @file seu_injection_test.c
 * @brief Teste de injeção de SEU — validação do TMR das estruturas globais.
 *
 * Simula Single Event Upsets escrevendo directamente nas cópias (live,
 * shadow B, shadow C) e verifica o comportamento da votação 2-de-3 do
 * módulo seu_data:
 *
 *   T1  Upset na cópia live      → maioria B/C repara o valor original.
 *   T2  Bit-flip num float (IMU) → idem, ao nível do bit.
 *   T3  Upset numa shadow (B)    → reparada a partir de live/C; o valor
 *                                  visível pela aplicação nunca é afectado.
 *   T4  Upset duplo divergente   → irrecuperável por maioria: o sistema
 *                                  adopta um valor determinístico e as 3
 *                                  cópias voltam a ficar coerentes
 *                                  (uncorrectable++).
 *   T5  Escrita legítima+commit  → o scrub NÃO altera dados válidos
 *                                  (ausência de falsos positivos).
 *
 * Este ficheiro NÃO usa o framework de testes unitários.
 * É uma função autónoma chamada directamente da main() do firmware.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "testsForBoard/seu_injection_test.h"
#include "app/seu_data.h"
#include "app/sensors.h"

static int s_pass = 0;
static int s_fail = 0;

static void report(const char *id, const char *desc, int ok)
{
    printf("| %-4s | %-58s | %-4s |\r\n", id, desc, ok ? "PASS" : "FAIL");
    if (ok) { s_pass++; } else { s_fail++; }
}

static void edge(void)
{
    printf("+------+------------------------------------------------------------+------+\r\n");
}

static int f_eq(float a, float b)
{
    float d = a - b;
    if (d < 0.0f) { d = -d; }
    return d < 0.0001f;
}

void test_seu_injection(void)
{
    const seu_data_stats_t *st;
    uint32_t corr0, unc0;

    printf("\r\n");
    printf("+--------------------------------------------------------------------------+\r\n");
    printf("|        TESTE SEU - Injecao de falhas no TMR das estruturas globais       |\r\n");
    printf("|        Modulo: seu_data (votacao 2-de-3, byte a byte)                    |\r\n");
    printf("+--------------------------------------------------------------------------+\r\n");

    /* Regista as regiões protegidas (1ª chamada a sensors_tick faz o
     * registo e o commit inicial). */
    sensors_tick();

    st = seu_data_get_stats();
    seu_data_reset_stats();

    edge();
    printf("| ID   | Cenario                                                    | Res. |\r\n");
    edge();

    /* ── T1: upset na cópia live — maioria B/C repara ─────────────────── */
    {
        gnss.latitude = 38.7223f;          /* valor legítimo (Lisboa)      */
        seu_data_commit();                 /* consolidado nas 3 cópias     */

        gnss.latitude = 999.9f;            /* SEU simulado na cópia live   */
        (void)seu_data_scrub();            /* votação: B==C ganham         */

        report("T1", "Upset na copia live: reparado por maioria B/C",
               f_eq(gnss.latitude, 38.7223f));
    }

    /* ── T2: bit-flip num float da IMU ────────────────────────────────── */
    {
        uint8_t *p = (uint8_t *)&imu.ax;

        imu.ax = 9.81f;
        seu_data_commit();

        p[1] ^= 0x10U;                     /* flip de 1 bit (SEU típico)   */
        (void)seu_data_scrub();

        report("T2", "Bit-flip em imu.ax: reparado por maioria B/C",
               f_eq(imu.ax, 9.81f));
    }

    /* ── T3: upset numa shadow — aplicação nunca vê o erro ────────────── */
    {
        volatile uint8_t *shadow_b = seu_data_dbg_shadow(&eps, 1U);
        float before;

        eps.voltage = 3.7f;
        seu_data_commit();
        before = eps.voltage;

        if (shadow_b != 0)
        {
            shadow_b[0] ^= 0xFFU;          /* SEU simulado na shadow B     */
        }
        (void)seu_data_scrub();            /* live==C reparam B            */

        report("T3", "Upset na shadow B de eps: reparado, live intacta",
               (shadow_b != 0) && f_eq(eps.voltage, before) &&
               (shadow_b[0] == ((const volatile uint8_t *)&eps)[0]));
    }

    /* ── T4: upset duplo divergente — falha tripla ────────────────────── */
    {
        volatile uint8_t *shadow_b = seu_data_dbg_shadow(&BATTERY_STATUS, 1U);
        volatile uint8_t *shadow_c = seu_data_dbg_shadow(&BATTERY_STATUS, 2U);
        volatile uint8_t *live     = (volatile uint8_t *)&BATTERY_STATUS;
        uint32_t unc_before;
        int coherent;

        BATTERY_STATUS = 77.0f;
        seu_data_commit();
        unc_before = seu_data_get_stats()->uncorrectable;

        live[3]     = 0xAAU;               /* 3 cópias distintas no mesmo  */
        shadow_b[3] = 0x55U;               /* byte -> maioria impossivel   */

        (void)seu_data_scrub();

        coherent = (live[3] == shadow_b[3]) && (shadow_b[3] == shadow_c[3]);

        report("T4", "Upset duplo divergente: detectado e estado re-coerente",
               (seu_data_get_stats()->uncorrectable == unc_before + 1U) &&
               coherent);

        BATTERY_STATUS = 77.0f;            /* restaura valor de teste      */
        seu_data_commit();
    }

    /* ── T5: escrita legítima não é revertida (sem falsos positivos) ──── */
    {
        corr0 = seu_data_get_stats()->corrected;

        temperature.temperature = 21.5f;   /* escrita legítima             */
        seu_data_commit();                 /* consolidada                  */
        (void)seu_data_scrub();            /* não deve tocar em nada       */

        report("T5", "Escrita legitima + commit: scrub nao altera dados",
               f_eq(temperature.temperature, 21.5f) &&
               (seu_data_get_stats()->corrected == corr0));
    }

    edge();

    st    = seu_data_get_stats();
    corr0 = st->corrected;
    unc0  = st->uncorrectable;

    {
        char buf[80];

        (void)snprintf(buf, sizeof buf,
                       "Estatisticas: %lu bytes corrigidos, %lu falhas triplas, %lu scrubs",
                       (unsigned long)corr0, (unsigned long)unc0,
                       (unsigned long)st->scrub_runs);
        printf("| %-72s |\r\n", buf);
        edge();

        (void)snprintf(buf, sizeof buf, "RESULTADO GLOBAL: %d/%d PASS %s",
                       s_pass, s_pass + s_fail,
                       (s_fail == 0) ? "- protecao TMR operacional"
                                     : "- ATENCAO: falhas");
        printf("| %-72s |\r\n", buf);
        edge();
    }
    printf("\r\n");

    seu_data_reset_stats();
}
