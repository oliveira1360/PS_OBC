/**
 * @file wcet_test.c
 * @brief Medição de WCET (Worst-Case Execution Time) na placa SAMV71.
 *
 * Método: measurement-based WCET com o contador de ciclos do DWT
 * (Cortex-M7). Cada função é executada WCET_N_RUNS vezes e regista-se
 * o MÁXIMO de ciclos observado. Ao máximo aplica-se uma margem de 30%
 * e compara-se com o deadline do super-loop (1 tick = 1 ms).
 *
 * Notas importantes:
 *   - A 1ª execução é medida em separado (cache/branch predictor frios)
 *     e conta para o máximo — é normalmente o pior caso real.
 *   - Com irq_off=1 mede-se o WCET "puro" da função; com irq_off=0
 *     inclui-se a interferência do SysTick — reportam-se os dois.
 *   - Medição dá um LIMITE INFERIOR do WCET teórico: só é válida se as
 *     execuções cobrirem o pior caminho lógico (sensores todos activos,
 *     ramos de erro, buffers máximos). Garantia formal exigiria análise
 *     estática (ex.: aiT), fora do âmbito deste projecto.
 *
 * Este ficheiro NÃO usa o framework de testes unitários.
 * É uma função autónoma chamada directamente da main() do firmware.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include "testsForBoard/wcet_test.h"
#include "app/states.h"     /* stateCheck() */

/* Protótipos declarados directamente (em vez de app/sensors.h e app/seu.h)
 * para este ficheiro não depender da cadeia de includes ttc.h -> modes.h. */
void sensors_tick(void);
void mission_lifecycle(void);
void seu_scrub(void);

/* ---------------------------------------------------------------------------
 * DWT — Data Watchpoint and Trace unit (Cortex-M7)
 * Contador de ciclos de CPU com resolução de 1 ciclo (160 ns @ 6.25 MHz).
 * --------------------------------------------------------------------------- */
#define DEMCR        (*(volatile uint32_t *)0xE000EDFCUL)
#define DWT_CTRL     (*(volatile uint32_t *)0xE0001000UL)
#define DWT_CYCCNT   (*(volatile uint32_t *)0xE0001004UL)
#define DWT_LAR      (*(volatile uint32_t *)0xE0001FB0UL)

#define DEMCR_TRCENA     (1UL << 24)
#define DWT_CTRL_CYCEN   (1UL << 0)

static void dwt_init(void)
{
    DEMCR   |= DEMCR_TRCENA;     /* liga o bloco de trace/debug          */
    DWT_LAR  = 0xC5ACCE55UL;     /* unlock (necessário nalguns Cortex-M7) */
    DWT_CYCCNT = 0UL;
    DWT_CTRL |= DWT_CTRL_CYCEN;  /* arranca o contador de ciclos          */
}

static inline uint32_t cycles_now(void)
{
    return DWT_CYCCNT;
}

static inline void irq_disable(void) { __asm volatile ("cpsid i" ::: "memory"); }
static inline void irq_enable(void)  { __asm volatile ("cpsie i" ::: "memory"); }

/* ---------------------------------------------------------------------------
 * Medição genérica de uma função void(void)
 * --------------------------------------------------------------------------- */
wcet_result_t wcet_measure(void (*fn)(void), uint32_t n_runs, int irq_off)
{
    wcet_result_t r;
    uint64_t sum = 0U;
    uint32_t t0, dt;

    r.min_cycles   = 0xFFFFFFFFUL;
    r.max_cycles   = 0UL;
    r.first_cycles = 0UL;

    for (uint32_t i = 0U; i < n_runs; i++) {

        if (irq_off) { irq_disable(); }

        t0 = cycles_now();
        fn();
        dt = cycles_now() - t0;

        if (irq_off) { irq_enable(); }

        if (i == 0U)           { r.first_cycles = dt; }
        if (dt > r.max_cycles) { r.max_cycles = dt; }
        if (dt < r.min_cycles) { r.min_cycles = dt; }
        sum += dt;
    }

    r.avg_cycles = (uint32_t)(sum / n_runs);
    return r;
}

/* ---------------------------------------------------------------------------
 * Helpers de relatório
 * --------------------------------------------------------------------------- */
static uint32_t cycles_to_us(uint32_t c)
{
    return (uint32_t)(((uint64_t)c * 1000000UL) / WCET_CPU_HZ);
}

static int s_all_ok = 1;

/* Largura total da tabela: 89 colunas (87 interiores) */
static void box_edge(void)
{
    printf("+---------------------------------------------------------------------------------------+\r\n");
}

static void box_text(const char *fmt, ...)
{
    char buf[96];
    va_list ap;

    va_start(ap, fmt);
    (void)vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    printf("| %-85s |\r\n", buf);
}

static void print_row(const char *name, const wcet_result_t *r, int irq_off)
{
    uint32_t wcet_margin = (r->max_cycles * WCET_MARGIN_NUM) / WCET_MARGIN_DEN;
    int ok = (wcet_margin < WCET_DEADLINE_CYCLES);

    if (!ok) { s_all_ok = 0; }

    printf("| %-19s | %-3s | %7lu | %7lu | %9lu | %7lu | %6lu | %-6s |\r\n",
           name,
           irq_off ? "OFF" : "ON",
           (unsigned long)r->min_cycles,
           (unsigned long)r->avg_cycles,
           (unsigned long)r->max_cycles,
           (unsigned long)wcet_margin,
           (unsigned long)cycles_to_us(wcet_margin),
           ok ? "OK" : "VIOLA");
}

static void measure_and_print(const char *name, void (*fn)(void))
{
    wcet_result_t r;

    /* 1) WCET puro: interrupções desligadas durante cada execução */
    r = wcet_measure(fn, WCET_N_RUNS, 1);
    print_row(name, &r, 1);

    /* 2) Tempo real observado: inclui interferência do SysTick */
    r = wcet_measure(fn, WCET_N_RUNS, 0);
    print_row(name, &r, 0);
}

/* ---------------------------------------------------------------------------
 * Teste principal — chamar directamente na main(), depois de system_init()
 * ---------------------------------------------------------------------------
 * NOTA sobre o pior caso: para o WCET ser representativo, correr este
 * teste com o sistema no estado mais "carregado" possível — sensores
 * todos ligados e leituras assíncronas em curso (ttc_read_async()
 * chamado antes), para que sensors_tick()/mission_lifecycle() percorram
 * os ramos mais longos da máquina de estados.
 * --------------------------------------------------------------------------- */
void test_wcet(void)
{
    printf("\r\n");
    box_edge();
    box_text("                TESTE WCET - Worst-Case Execution Time (DWT/CYCCNT)");
    box_text("MCU: ATSAMV71Q21 (Cortex-M7) @ %lu Hz  |  %u execucoes por funcao",
             (unsigned long)WCET_CPU_HZ, (unsigned)WCET_N_RUNS);
    box_text("Deadline do super-loop: 1 ms = %lu ciclos  |  Margem de seguranca: +30%%",
             (unsigned long)WCET_DEADLINE_CYCLES);
    box_edge();

    dwt_init();

    if ((DWT_CTRL & DWT_CTRL_CYCEN) == 0UL) {
        printf("  ERRO: DWT_CYCCNT nao disponivel neste core.\r\n");
        return;
    }

    /* Sanity check: o contador tem de avançar */
    {
        uint32_t a = cycles_now();
        for (volatile int i = 0; i < 100; i++) { }
        if (cycles_now() == a) {
            printf("  ERRO: DWT_CYCCNT nao avanca.\r\n");
            return;
        }
    }

    printf("| Funcao              | IRQ |     min |   media | MAX(WCET) |    +30%% | us(*)  | Estado |\r\n");
    printf("+---------------------+-----+---------+---------+-----------+---------+--------+--------+\r\n");

    measure_and_print("sensors_tick()",      sensors_tick);
    measure_and_print("stateCheck()",        stateCheck);
    measure_and_print("seu_scrub()",         seu_scrub);
    measure_and_print("mission_lifecycle()", mission_lifecycle);

    printf("+---------------------+-----+---------+---------+-----------+---------+--------+--------+\r\n");
    box_text("Valores em ciclos de CPU. (*) us = (MAX+30%%) convertido a %lu Hz.",
             (unsigned long)WCET_CPU_HZ);
    box_text("IRQ OFF = WCET puro da funcao; IRQ ON = inclui interferencia do SysTick.");
    box_edge();
    box_text("RESULTADO GLOBAL: %s",
             s_all_ok ? "OK - todas as funcoes cumprem o deadline de 1 ms"
                      : "FALHA - pelo menos uma funcao viola o deadline");
    box_edge();
    printf("\r\n");
}
