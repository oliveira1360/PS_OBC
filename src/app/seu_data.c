/**
 * @file seu_data.c
 * @brief Implementação da proteção TMR para estruturas globais (ver seu_data.h).
 *
 * As shadows B e C vivem em dois bancos estáticos separados. Tal como em
 * seu.c, a separação em arrays distintos garante endereços de RAM afastados,
 * de modo a que um único evento não atinja mais do que uma cópia.
 *
 * Sem alocação dinâmica: bancos e tabela de regiões são estáticos (RNF4).
 */

#include <stdio.h>
#include "app/seu_data.h"

/* ── Bancos shadow (separados fisicamente na SRAM) ───────────────────── */

static volatile uint8_t s_bank_b[SEU_DATA_BANK_BYTES];
static volatile uint8_t s_bank_c[SEU_DATA_BANK_BYTES];

/* ── Tabela de regiões ───────────────────────────────────────────────── */

typedef struct
{
    volatile uint8_t *live;  /* cópia A (a variável global)      */
    uint16_t          size;  /* bytes                            */
    uint16_t          off;   /* offset da região dentro dos bancos */
    const char       *name;  /* para logging                     */
} seu_region_t;

static seu_region_t s_regions[SEU_DATA_MAX_REGIONS];
static uint8_t  s_n_regions  = 0U;
static uint16_t s_bank_used  = 0U;

static seu_data_stats_t s_stats = {0U, 0U, 0U};

/* ── Registo ─────────────────────────────────────────────────────────── */

int seu_data_protect(volatile void *live, uint16_t size, const char *name)
{
    seu_region_t *r;
    uint16_t i;

    if (s_n_regions >= SEU_DATA_MAX_REGIONS)
    {
        printf("[SEU-DATA] sem slots para '%s'\r\n", name);
        return -1;
    }
    if ((uint16_t)(s_bank_used + size) > SEU_DATA_BANK_BYTES)
    {
        printf("[SEU-DATA] sem espaco de banco para '%s' (%u B)\r\n",
               name, (unsigned)size);
        return -1;
    }

    r = &s_regions[s_n_regions];
    r->live = (volatile uint8_t *)live;
    r->size = size;
    r->off  = s_bank_used;
    r->name = name;

    /* commit inicial: B = C = live */
    for (i = 0U; i < size; i++)
    {
        s_bank_b[r->off + i] = r->live[i];
        s_bank_c[r->off + i] = r->live[i];
    }

    s_bank_used = (uint16_t)(s_bank_used + size);
    s_n_regions++;
    return 0;
}

/* ── Scrub: votação 2-de-3 byte a byte, com reparação ────────────────── */

uint32_t seu_data_scrub(void)
{
    uint32_t divergent = 0U;
    uint8_t  n;
    uint16_t i;

    for (n = 0U; n < s_n_regions; n++)
    {
        const seu_region_t *r = &s_regions[n];

        for (i = 0U; i < r->size; i++)
        {
            const uint8_t a = r->live[i];
            const uint8_t b = s_bank_b[r->off + i];
            const uint8_t c = s_bank_c[r->off + i];

            if (a == b && b == c)
            {
                continue; /* caminho rápido: tudo coerente */
            }

            divergent++;

            if (a == b)
            {
                s_bank_c[r->off + i] = a;       /* C estava corrompida */
                s_stats.corrected++;
            }
            else if (a == c)
            {
                s_bank_b[r->off + i] = a;       /* B estava corrompida */
                s_stats.corrected++;
            }
            else if (b == c)
            {
                r->live[i] = b;                 /* live estava corrompida */
                s_stats.corrected++;
            }
            else
            {
                /* 3 cópias distintas: irrecuperável por maioria.
                 * Adopta-se B (último valor consolidado no commit). */
                r->live[i]           = b;
                s_bank_c[r->off + i] = b;
                s_stats.uncorrectable++;
                printf("[SEU-DATA] FALHA TRIPLA em '%s'[%u] — adoptado ultimo commit\r\n",
                       r->name, (unsigned)i);
            }
        }
    }

    s_stats.scrub_runs++;
    return divergent;
}

/* ── Commit: consolida escritas legítimas ────────────────────────────── */

void seu_data_commit(void)
{
    uint8_t  n;
    uint16_t i;

    for (n = 0U; n < s_n_regions; n++)
    {
        const seu_region_t *r = &s_regions[n];

        for (i = 0U; i < r->size; i++)
        {
            const uint8_t a = r->live[i];
            s_bank_b[r->off + i] = a;
            s_bank_c[r->off + i] = a;
        }
    }
}

/* ── Estatísticas ────────────────────────────────────────────────────── */

const seu_data_stats_t *seu_data_get_stats(void)
{
    return &s_stats;
}

void seu_data_reset_stats(void)
{
    s_stats.corrected     = 0U;
    s_stats.uncorrectable = 0U;
    s_stats.scrub_runs    = 0U;
}

/* ── Hook de teste ───────────────────────────────────────────────────── */

volatile uint8_t *seu_data_dbg_shadow(volatile void *live, uint8_t bank)
{
    uint8_t n;

    for (n = 0U; n < s_n_regions; n++)
    {
        if (s_regions[n].live == (volatile uint8_t *)live)
        {
            if (bank == 1U) { return &s_bank_b[s_regions[n].off]; }
            if (bank == 2U) { return &s_bank_c[s_regions[n].off]; }
            return 0;
        }
    }
    return 0;
}
