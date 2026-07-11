#include <stdio.h>
#include "app/seu.h"

/**
 * @file seu.c
 * @brief Implementação da proteção TMR contra SEUs.
 *
 * As três cópias vivem em três bancos estáticos SEPARADOS. O linker coloca
 * cada array numa região própria, garantindo separação física na SRAM —
 * essencial para que um único evento não atinja mais do que uma cópia.
 */

/* ── Armazenamento triplicado ────────────────────────────────────────── */

static volatile uint32_t s_bank_a[SEU_MAX_VARS];
static volatile uint32_t s_bank_b[SEU_MAX_VARS];
static volatile uint32_t s_bank_c[SEU_MAX_VARS];

/* Fallback por variável (escrito uma vez na alocação). */
static uint32_t s_fallback[SEU_MAX_VARS];

static uint8_t s_used = 0U;
static seu_stats_t s_stats = {0U, 0U};

/* Conversão float <-> bits sem violar strict aliasing. */
typedef union
{
    float f;
    uint32_t u;
} seu_f32_bits_t;

/* ── Núcleo: votação por maioria com reparação ───────────────────────── */

static uint32_t seu_vote(seu_handle_t h)
{
    const uint32_t a = s_bank_a[h];
    const uint32_t b = s_bank_b[h];
    const uint32_t c = s_bank_c[h];

    if (a == b && b == c)
        return a; /* caminho rápido: tudo coerente */

    if (a == b || a == c)
    {
        /* 'a' tem maioria — repara a cópia divergente */
        s_stats.corrected++;
        s_bank_b[h] = a;
        s_bank_c[h] = a;
        return a;
    }
    if (b == c)
    {
        /* 'b'/'c' têm maioria — 'a' estava corrompida */
        s_stats.corrected++;
        s_bank_a[h] = b;
        return b;
    }

    /* As 3 diferem: irrecuperável por votação — aplica fallback seguro. */
    s_stats.uncorrectable++;
    printf("[SEU] FALHA TRIPLA na var %u (0x%08lX/0x%08lX/0x%08lX) — fallback\n",
           (unsigned)h, (unsigned long)a, (unsigned long)b, (unsigned long)c);
    s_bank_a[h] = s_fallback[h];
    s_bank_b[h] = s_fallback[h];
    s_bank_c[h] = s_fallback[h];
    return s_fallback[h];
}

static void seu_store(seu_handle_t h, uint32_t v)
{
    s_bank_a[h] = v;
    s_bank_b[h] = v;
    s_bank_c[h] = v;
}

/* ── Alocação ────────────────────────────────────────────────────────── */

seu_handle_t seu_alloc_u32(uint32_t initial, uint32_t fallback)
{
    if (s_used >= SEU_MAX_VARS)
    {
        printf("[SEU] ERRO: sem slots livres (max %u)\n", (unsigned)SEU_MAX_VARS);
        return SEU_INVALID_HANDLE;
    }
    const seu_handle_t h = s_used++;
    s_fallback[h] = fallback;
    seu_store(h, initial);
    return h;
}

seu_handle_t seu_alloc_i32(int32_t initial, int32_t fallback)
{
    return seu_alloc_u32((uint32_t)initial, (uint32_t)fallback);
}

seu_handle_t seu_alloc_f32(float initial, float fallback)
{
    seu_f32_bits_t i = {.f = initial};
    seu_f32_bits_t fb = {.f = fallback};
    return seu_alloc_u32(i.u, fb.u);
}

/* ── Leitura (vota + repara) / Escrita (3 cópias) ────────────────────── */

uint32_t seu_read_u32(seu_handle_t h)
{
    if (h >= s_used)
        return 0U;
    return seu_vote(h);
}

int32_t seu_read_i32(seu_handle_t h)
{
    return (int32_t)seu_read_u32(h);
}

float seu_read_f32(seu_handle_t h)
{
    seu_f32_bits_t b = {.u = seu_read_u32(h)};
    return b.f;
}

void seu_write_u32(seu_handle_t h, uint32_t value)
{
    if (h >= s_used)
        return;
    seu_store(h, value);
}

void seu_write_i32(seu_handle_t h, int32_t value)
{
    seu_write_u32(h, (uint32_t)value);
}

void seu_write_f32(seu_handle_t h, float value)
{
    seu_f32_bits_t b = {.f = value};
    seu_write_u32(h, b.u);
}

/* ── Manutenção ──────────────────────────────────────────────────────── */

void seu_scrub(void)
{
    for (seu_handle_t h = 0U; h < s_used; h++)
        (void)seu_vote(h);
}

const seu_stats_t *seu_get_stats(void)
{
    return &s_stats;
}
