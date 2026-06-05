/**
 * @file bootloader.c
 * @brief Lógica principal do bootloader OTA para ATSAMV71Q21B.
 *
 * Fluxo completo:
 *
 *  1. bootloader_run() lê os metadados OTA da flash externa (0x100000).
 *  2. Se magic != OTA_MAGIC_PENDING → salta directamente para a aplicação.
 *  3. Se magic == OTA_MAGIC_PENDING:
 *       a. Valida firmware_size (não pode exceder APP_MAX_SIZE).
 *       b. Verifica CRC32 do firmware na flash externa.
 *       c. Copia firmware para a flash interna (0x00410000).
 *       d. Apaga o sector de metadados OTA (limpa flag).
 *       e. Salta para a nova aplicação.
 *  4. Em caso de erro, NÃO apaga os metadados (permite retry no próximo boot).
 *     Excepção: erro de CRC — os metadados são apagados para evitar loop.
 *
 * bootloader_jump_to_app():
 *  - Configura SCB->VTOR para APP_START_ADDR.
 *  - Carrega SP do vector[0] da aplicação.
 *  - Salta para o Reset_Handler da aplicação (vector[1]).
 */

#include "bootloader.h"
#include "qspi_boot.h"
#include "flash_efc.h"
#include "ota_verify.h"
#include "system_samv71.h"

static void boot_system_reset(void);

/* =========================================================================
 * Helpers internos
 * ========================================================================= */

/**
 * @brief Apaga o sector de metadados OTA na flash externa.
 *        Chamado após aplicação bem-sucedida ou após erro de CRC.
 */
static void clear_ota_metadata(void)
{
    /* Tenta apagar até 3 vezes — erase falhou silenciosamente em versões anteriores */
    for (uint32_t attempt = 0U; attempt < 3U; attempt++)
    {
        if (qspi_boot_erase_sector(OTA_EXT_METADATA_SECTOR) == QSPI_BOOT_OK)
        {
            return; /* Erase confirmado pelo wait_busy */
        }
        /* Pequena pausa antes de retry */
        volatile uint32_t delay = 0x10000U;
        while (delay--)
        {
        }
    }
    /* Se falhou 3 vezes, continua de qualquer forma — na pior das hipóteses
     * o próximo boot volta a aplicar o mesmo firmware (inofensivo). */
}

/**
 * @brief Verifica se a aplicação em flash interna parece válida.
 *        Critério simples: o vector de reset (offset 4) não deve ser
 *        0x00000000 nem 0xFFFFFFFF.
 */
static bool app_is_valid(void)
{
    const uint32_t *app_vectors = (const uint32_t *)APP_START_ADDR;
    uint32_t reset_handler = app_vectors[1]; /* Reset_Handler no vector[1] */

    return (reset_handler != 0x00000000UL &&
            reset_handler != 0xFFFFFFFFUL);
}

/* =========================================================================
 * bootloader_run
 * ========================================================================= */
boot_result_t bootloader_run(void)
{
    if (qspi_boot_init() != QSPI_BOOT_OK)
    {
        debug_uart_puts("[BOOT] qspi_init FALHOU\n");
        bootloader_jump_to_app();
    }
    debug_uart_puts("[BOOT] qspi_init OK\n");

    /* ===== TESTE A: CRC da app que JA esta na flash interna ===== */
    {
        uint32_t c = 0xFFFFFFFFUL;
        const uint8_t *a = (const uint8_t *)APP_START_ADDR;
        for (uint32_t i = 0U; i < 19372U; i++) /* size fixo so para o teste */
        {
            c ^= (uint32_t)a[i];
            for (uint8_t b = 0U; b < 8U; b++)
                c = (c & 1UL) ? (c >> 1) ^ 0xEDB88320UL : (c >> 1);
        }
        c ^= 0xFFFFFFFFUL;
        debug_uart_hex("[BOOT] CRC app atual=", c);
    }

    /* ===== TESTE B: escreve 1 pagina com padrao conhecido ===== */
    {
        static uint8_t test_buf[512];
        for (int i = 0; i < 512; i++)
            test_buf[i] = (uint8_t)(i & 0xFF);

        flash_efc_result_t te = flash_efc_erase_region(APP_START_ADDR, 512U);
        debug_uart_hex("[BOOT] test erase result=", (uint32_t)te);

        /* le DEPOIS do erase, ANTES de escrever — deve dar tudo 0xFF */
        const uint8_t *chk = (const uint8_t *)APP_START_ADDR;
        debug_uart_puts("[BOOT] pos-erase (esperado FF FF...): ");
        for (int i = 0; i < 8; i++)
            debug_uart_hex("", chk[i]);

        flash_efc_result_t tw = flash_efc_write_page(APP_START_ADDR, test_buf);
        debug_uart_hex("[BOOT] test write result=", (uint32_t)tw);

        const uint8_t *rd = (const uint8_t *)APP_START_ADDR;
        debug_uart_puts("[BOOT] test readback (esperado 00 01 02 03 04 05 06 07): ");
        for (int i = 0; i < 8; i++)
            debug_uart_hex("", rd[i]);
    }

    static ota_metadata_t meta;
    qspi_boot_result_t qres = qspi_boot_read(OTA_EXT_METADATA_ADDR, (uint8_t *)&meta, sizeof(meta));
    if (qres != QSPI_BOOT_OK)
    {
        debug_uart_puts("[BOOT] read meta FALHOU\n");
        bootloader_jump_to_app();
    }

    debug_uart_hex("[BOOT] magic=", meta.magic);
    debug_uart_hex("[BOOT] size=", meta.firmware_size);
    debug_uart_hex("[BOOT] crc=", meta.firmware_crc32);

    if (meta.magic != OTA_MAGIC_PENDING)
    {
        debug_uart_puts("[BOOT] sem OTA pendente -> app antiga\n");
        bootloader_jump_to_app();
    }

    if (meta.firmware_size == 0U || meta.firmware_size > APP_MAX_SIZE)
    {
        debug_uart_puts("[BOOT] size invalido -> limpa e app antiga\n");
        clear_ota_metadata();
        bootloader_jump_to_app();
    }

    bool crc_ok = ota_verify_crc(OTA_EXT_FIRMWARE_ADDR, meta.firmware_size, meta.firmware_crc32);
    debug_uart_puts(crc_ok ? "[BOOT] CRC externa OK\n" : "[BOOT] CRC externa FAIL\n");

    if (!crc_ok)
    {
        clear_ota_metadata();
        if (app_is_valid())
            bootloader_jump_to_app();
        while (1)
        {
        }
    }

    debug_uart_puts("[BOOT] a copiar fw para flash interna...\n");

#define COPY_PAGE 512U
    static uint8_t copy_buf[COPY_PAGE];

    system_cache_disable();

    flash_efc_result_t fres = flash_efc_erase_region(APP_START_ADDR, meta.firmware_size);
    if (fres == FLASH_EFC_OK)
    {
        uint32_t off = 0U;
        while (off < meta.firmware_size)
        {
            uint32_t chunk = (meta.firmware_size - off) > COPY_PAGE
                                 ? COPY_PAGE
                                 : (meta.firmware_size - off);
            if (chunk < COPY_PAGE)
                for (uint32_t i = 0U; i < COPY_PAGE; i++)
                    copy_buf[i] = 0xFFU;

            if (qspi_boot_read(OTA_EXT_FIRMWARE_ADDR + off, copy_buf, chunk) != QSPI_BOOT_OK)
            {
                fres = FLASH_EFC_ERROR;
                break;
            }
            fres = flash_efc_write_page(APP_START_ADDR + off, copy_buf);
            if (fres != FLASH_EFC_OK)
                break;

            /* verifica imediatamente esta pagina */
            const uint8_t *wr = (const uint8_t *)(APP_START_ADDR + off);
            for (uint32_t k = 0U; k < chunk; k++)
            {
                if (wr[k] != copy_buf[k])
                {
                    debug_uart_hex("[BOOT] MISMATCH na pagina off=", off);
                    debug_uart_hex("  byte k=", k);
                    debug_uart_hex("  escrito=", copy_buf[k]);
                    debug_uart_hex("  lido=", wr[k]);
                    break;
                }
            }

            off += COPY_PAGE;
        }
    }

    system_cache_invalidate();

    /* verifica flash interna */
    uint32_t crc_int = 0xFFFFFFFFUL;
    const uint8_t *app = (const uint8_t *)APP_START_ADDR;
    for (uint32_t i = 0U; i < meta.firmware_size; i++)
    {
        crc_int ^= (uint32_t)app[i];
        for (uint8_t b = 0U; b < 8U; b++)
            crc_int = (crc_int & 1UL) ? (crc_int >> 1) ^ 0xEDB88320UL : (crc_int >> 1);
    }
    crc_int ^= 0xFFFFFFFFUL;

    debug_uart_hex("[BOOT] CRC flash interna=", crc_int);
    debug_uart_hex("[BOOT] CRC esperado=", meta.firmware_crc32);

    if (crc_int != meta.firmware_crc32)
    {
        /* NAO faz reset (evita loop). Salta para a app existente se valida. */
        debug_uart_puts("[BOOT] copia FALHOU -> app existente\n");
        if (app_is_valid())
            bootloader_jump_to_app();
        while (1)
        {
        }
    }

    debug_uart_puts("[BOOT] copia verificada OK\n");
    clear_ota_metadata();
    bootloader_jump_to_app();

    return BOOT_OTA_APPLIED;
}

/* =========================================================================
 * bootloader_jump_to_app
 * ========================================================================= */
void bootloader_jump_to_app(void)
{
    const uint32_t *app_vectors = (const uint32_t *)APP_START_ADDR;
    debug_uart_hex("[BOOT] jump SP=", app_vectors[0]);
    debug_uart_hex("[BOOT] jump reset=", app_vectors[1]);

    if (!app_is_valid())
    {
        debug_uart_puts("[BOOT] app INVALIDA -> loop\n");
        while (1)
        {
        }
    }
    /* drena UART antes de saltar */
    for (volatile uint32_t d = 0; d < 200000U; d++)
    {
    }

    /* 1. Prepara sistema: desabilita IRQs, SysTick, etc. */
    system_prepare_jump();

    /* 2. Configura Vector Table Offset Register para a aplicação */
    SCB_VTOR = APP_START_ADDR;

    /* Barreira para garantir que o VTOR está actualizado */
    __asm__ volatile("dsb" ::: "memory");
    __asm__ volatile("isb" ::: "memory");

    /* 3. Carrega novo Stack Pointer (vector[0] da aplicação) */
    uint32_t app_sp = app_vectors[0];

    /* 4. Carrega endereço do Reset_Handler da aplicação (vector[1]) */
    uint32_t app_reset = app_vectors[1];

    /* 5. Salta para a aplicação:
     *    - Configura MSP com o SP da aplicação
     *    - Salta para o Reset_Handler
     *    Esta sequência em assembly garante que não ficam restos
     *    do stack do bootloader a afectar a aplicação. */
    __asm__ volatile(
        "msr msp, %0      \n" /* Configura Main Stack Pointer              */
        "bx  %1           \n" /* Branch para Reset_Handler da aplicação    */
        :
        : "r"(app_sp), "r"(app_reset)
        : "memory");

    /* Nunca chega aqui */
    __builtin_unreachable();
}

static void boot_system_reset(void)
{
    __asm__ volatile("dsb" ::: "memory");
    *(volatile uint32_t *)0xE000ED0CUL = (0x5FAUL << 16) | (1UL << 2); /* SYSRESETREQ */
    __asm__ volatile("dsb" ::: "memory");
    while (1)
    {
    }
}