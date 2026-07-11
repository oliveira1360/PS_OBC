#ifndef SEU_DATA_H
#define SEU_DATA_H

#include <stdint.h>

/**
 * @file seu_data.h
 * @brief Proteção TMR (Triple Modular Redundancy) para estruturas globais.
 *
 * Complementa o módulo seu.c (que protege variáveis escalares de 32 bits,
 * como mission_state) estendendo a proteção a REGIÕES de memória — as
 * estruturas globais de sensores (gnss, imu, eps, pressure, temperature,
 * propulsor, BATTERY_STATUS).
 *
 * Modelo de cópias:
 *   Cópia A = a própria variável global (live) — o resto do código continua
 *             a ler/escrever normalmente, sem alterações.
 *   Cópias B e C = shadows em dois bancos estáticos separados (endereços
 *             de RAM afastados; um multi-bit upset contíguo não atinge
 *             as 3 cópias).
 *
 * Ciclo de vida por iteração do super-loop (integrado em sensors_tick()):
 *   1. seu_data_scrub()  — vota byte a byte (2-de-3) e repara divergências
 *                          causadas por SEU desde o último commit;
 *   2. *_tick() dos periféricos — escritas legítimas alteram apenas a
 *                          cópia live;
 *   3. seu_data_commit() — consolida o estado live nas cópias B e C.
 *
 * Votação (por byte):
 *   - 3 iguais              → nada a fazer;
 *   - 2 iguais + 1 difere   → a cópia divergente é reparada (corrected++);
 *   - 3 diferentes          → irrecuperável por maioria: adopta-se a cópia B
 *                             (último valor consolidado) e uncorrectable++.
 *
 * Janela não protegida (por desenho): um SEU entre a escrita legítima e o
 * commit da mesma iteração é consolidado. A janela é < 1 ms (1 tick).
 */

/** Nº máximo de regiões protegidas (alocação estática). */
#define SEU_DATA_MAX_REGIONS 12U

/** Capacidade de cada banco shadow, em bytes. */
#define SEU_DATA_BANK_BYTES 256U

typedef struct
{
    uint32_t corrected;     /**< bytes reparados por votação 2-de-3        */
    uint32_t uncorrectable; /**< bytes com 3 cópias distintas (fallback B) */
    uint32_t scrub_runs;    /**< nº de execuções de seu_data_scrub()       */
} seu_data_stats_t;

/* ── Registo (uma vez, no arranque) ──────────────────────────────────── */

/**
 * @brief Regista uma região global sob proteção TMR.
 * @param live Endereço da variável global (cópia A).
 * @param size Tamanho em bytes.
 * @param name Nome curto para logging.
 * @return 0 em sucesso; -1 se não há slots/espaço nos bancos.
 * Faz o commit inicial (B = C = live).
 */
int seu_data_protect(volatile void *live, uint16_t size, const char *name);

/* ── Operação cíclica ────────────────────────────────────────────────── */

/**
 * @brief Vota e repara todas as regiões registadas.
 * @return Nº de bytes divergentes encontrados nesta passagem.
 */
uint32_t seu_data_scrub(void);

/** @brief Consolida o estado live de todas as regiões nas cópias B e C. */
void seu_data_commit(void);

/* ── Estatísticas ────────────────────────────────────────────────────── */

const seu_data_stats_t *seu_data_get_stats(void);
void seu_data_reset_stats(void);

/* ── Hook de teste (injeção de falhas) ───────────────────────────────── */

/**
 * @brief Devolve o ponteiro para a cópia shadow de uma região registada.
 * @param live Endereço da variável global registada.
 * @param bank 1 = banco B, 2 = banco C.
 * @return Ponteiro para a shadow, ou 0 se a região não está registada.
 * @warning APENAS para testes de injeção de SEU (seu_injection_test.c).
 */
volatile uint8_t *seu_data_dbg_shadow(volatile void *live, uint8_t bank);

#endif /* SEU_DATA_H */
