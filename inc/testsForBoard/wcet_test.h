#ifndef WCET_TEST_H
#define WCET_TEST_H

#include <stdint.h>

/* Nº de execuções por função (quanto mais, melhor a cobertura do pior caso) */
#define WCET_N_RUNS      1000U

/* Frequência do core (SysTick com CLKSOURCE=1 usa HCLK = 300 MHz) */
#define WCET_CPU_HZ      300000000UL

/* Deadline do super-loop: 1 tick = 1 ms */
#define WCET_DEADLINE_CYCLES   (WCET_CPU_HZ / 1000UL)   /* 6250 ciclos */

/* Margem de segurança aplicada ao máximo observado (30%) */
#define WCET_MARGIN_NUM  13U
#define WCET_MARGIN_DEN  10U

/* Resultado de uma medição */
typedef struct {
    uint32_t min_cycles;
    uint32_t max_cycles;   /* WCET observado (sem margem) */
    uint32_t avg_cycles;
    uint32_t first_cycles; /* 1ª execução (cache fria) */
} wcet_result_t;

/* Mede uma função arbitrária void(void), N execuções.
 * irq_off = 1 -> mede com interrupções desligadas (WCET "puro" da função)
 * irq_off = 0 -> inclui interferência do SysTick/IRQs (tempo real observado) */
wcet_result_t wcet_measure(void (*fn)(void), uint32_t n_runs, int irq_off);

/* Teste completo: mede as funções principais do OBC e imprime relatório.
 * Chamar directamente da main(), depois de system_init(). */
void test_wcet(void);

#endif
