#ifndef SEU_H
#define SEU_H

#include <stdint.h>

/**
 * @file seu.h
 * @brief Proteção contra SEU (Single Event Upset) por TMR — Triple Modular
 *        Redundancy ao nível de variável.
 *
 * Cada variável crítica é guardada em TRÊS cópias, colocadas em três bancos
 * estáticos distintos (arrays separados, logo endereços de RAM afastados —
 * um multi-bit upset numa região contígua não corrompe as 3 cópias).
 *
 * Em cada leitura é feita votação por maioria (2-de-3):
 *   - as 3 iguais            → valor devolvido, nada a fazer;
 *   - 2 iguais + 1 diferente → a cópia divergente é REPARADA com a maioria
 *                              (scrubbing on-read) e o evento é contado;
 *   - as 3 diferentes        → falha não corrigível: aplica-se o valor de
 *                              fallback seguro definido na alocação.
 *
 * Para upsets latentes em variáveis raramente lidas existe seu_scrub(),
 * que re-vota todas as variáveis registadas — chamar periodicamente no
 * ciclo principal.
 */

/** Número máximo de variáveis protegidas (alocação estática, sem heap). */
#define SEU_MAX_VARS 16U

/** Handle devolvido quando não há slots livres. */
#define SEU_INVALID_HANDLE 0xFFU

typedef uint8_t seu_handle_t;

/** Contadores de eventos SEU detetados desde o arranque. */
typedef struct
{
    uint32_t corrected;     /**< 1 cópia divergente — corrigida por maioria */
    uint32_t uncorrectable; /**< 3 cópias diferentes — fallback aplicado    */
} seu_stats_t;

/* ── Alocação (uma vez, no arranque) ─────────────────────────────────── */

/**
 * @brief Regista uma variável protegida de 32 bits.
 * @param initial  Valor inicial (escrito nas 3 cópias).
 * @param fallback Valor seguro a aplicar se as 3 cópias divergirem todas.
 * @return Handle para read/write, ou SEU_INVALID_HANDLE se não há slots.
 */
seu_handle_t seu_alloc_u32(uint32_t initial, uint32_t fallback);
seu_handle_t seu_alloc_i32(int32_t initial, int32_t fallback);
seu_handle_t seu_alloc_f32(float initial, float fallback);

/* ── Acesso (votação + reparação em cada leitura) ────────────────────── */

uint32_t seu_read_u32(seu_handle_t h);
int32_t  seu_read_i32(seu_handle_t h);
float    seu_read_f32(seu_handle_t h);

void seu_write_u32(seu_handle_t h, uint32_t value);
void seu_write_i32(seu_handle_t h, int32_t value);
void seu_write_f32(seu_handle_t h, float value);

/* ── Manutenção ──────────────────────────────────────────────────────── */

/**
 * @brief Re-vota e repara todas as variáveis registadas (memory scrubbing).
 * Chamar periodicamente (ex.: em mission_lifecycle) para corrigir upsets
 * latentes antes que um segundo upset na mesma variável os torne
 * incorrigíveis.
 */
void seu_scrub(void);

/** @return Estatísticas acumuladas de SEUs detetados. */
const seu_stats_t *seu_get_stats(void);

#endif /* SEU_H */
