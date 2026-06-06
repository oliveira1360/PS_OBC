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

#define EEFC_FMR (*(volatile uint32_t *)0x400E0C00UL)
#define EEFC_FSR (*(volatile uint32_t *)0x400E0C08UL)
#define COPY_PAGE 512U

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
    /* Tenta apagar até 3 vezes — erase falhou silenciosamente... */
    for (uint32_t attempt = 0U; attempt < 3U; attempt++)
    {
        if (qspi_boot_erase_sector(OTA_EXT_METADATA_SECTOR) == QSPI_BOOT_OK)
        {
            return;
        }
        volatile uint32_t delay = 0x10000U;
        while (delay--)
        {
        }
    }
}

/**
 * @brief Verifica se a aplicação em flash interna parece válida.
 *        Critério simples: o vector de reset (offset 4) não deve ser
 *        0x00000000 nem 0xFFFFFFFF.
 */
static bool app_is_valid(void)
{
    const uint32_t *app_vectors = (const uint32_t *)APP_START_ADDR;
    uint32_t reset_handler = app_vectors[1];
    return (reset_handler != 0x00000000UL &&
            reset_handler != 0xFFFFFFFFUL);
}

/* =========================================================================
 * boot_init_efc — configura FWS e acesso ao EFC para escrita
 * ========================================================================= */
static void boot_init_efc(void)
{
   EEFC_FMR = (1UL << 8);
    __asm__ volatile("dsb" ::: "memory");
}

/* =========================================================================
 * boot_prepare_flash_access — MPU off + caches off (para RAMFUNC + escrita)
 * ========================================================================= */
static void boot_prepare_flash_access(void)
{
    /* MPU off */
    *(volatile uint32_t *)0xE000ED94UL = 0UL;
    __asm__ volatile("dsb" ::: "memory");
    __asm__ volatile("isb" ::: "memory");

    /* Caches off */
    system_cache_disable();
    SCB_CCR &= ~((1UL << 17) | (1UL << 16)); /* IC, DC off */
    __asm__ volatile("dsb" ::: "memory");
    __asm__ volatile("isb" ::: "memory");

    debug_uart_hex("[BOOT] SCB_CCR=", SCB_CCR);
}

/* =========================================================================
 * boot_read_metadata — le e valida os metadados OTA
 * Retorna true se ha OTA pendente e valido para copiar.
 * ========================================================================= */
static bool boot_read_metadata(ota_metadata_t *meta)
{
    qspi_boot_read(OTA_EXT_METADATA_ADDR, (uint8_t *)meta, sizeof(*meta));
    debug_uart_hex("[BOOT] magic=", meta->magic);
    debug_uart_hex("[BOOT] size=", meta->firmware_size);
    debug_uart_hex("[BOOT] crc=", meta->firmware_crc32);

    if (meta->magic != OTA_MAGIC_PENDING)
    {
        debug_uart_puts("[BOOT] sem OTA -> app antiga\n");
        return false;
    }
    if (meta->firmware_size == 0U || meta->firmware_size > APP_MAX_SIZE)
    {
        debug_uart_puts("[BOOT] size invalido\n");
        clear_ota_metadata();
        return false;
    }
    return true;
}

/* =========================================================================
 * boot_verify_external — verifica CRC do firmware na flash externa
 * ========================================================================= */
static bool boot_verify_external(const ota_metadata_t *meta)
{
    bool crc_ok = ota_verify_crc(OTA_EXT_FIRMWARE_ADDR, meta->firmware_size, meta->firmware_crc32);
    debug_uart_puts(crc_ok ? "[BOOT] CRC externa OK\n" : "[BOOT] CRC externa FAIL\n");
    return crc_ok;
}

/* =========================================================================
 * boot_compute_internal_crc — CRC32 da flash interna ja escrita
 * ========================================================================= */
static uint32_t boot_compute_internal_crc(uint32_t size)
{
    uint32_t crc = 0xFFFFFFFFUL;
    const uint8_t *app = (const uint8_t *)APP_START_ADDR;
    for (uint32_t i = 0U; i < size; i++)
    {
        crc ^= (uint32_t)app[i];
        for (uint8_t b = 0U; b < 8U; b++)
            crc = (crc & 1UL) ? (crc >> 1) ^ 0xEDB88320UL : (crc >> 1);
    }
    return crc ^ 0xFFFFFFFFUL;
}

/* =========================================================================
 * boot_copy_firmware — copia da flash externa para a interna, pagina a pagina
 * Retorna FLASH_EFC_OK se todas as paginas escreveram.
 * ========================================================================= */
static flash_efc_result_t boot_copy_firmware(const ota_metadata_t *meta)
{
    static uint8_t copy_buf[COPY_PAGE];
    flash_efc_result_t fres = FLASH_EFC_OK;
    uint32_t off = 0U;
    uint32_t pagenum = 0U;

    /* Desbloqueia a regiao da app (alinhada ao sector) */
    uint32_t esz = (meta->firmware_size + FLASH_SECTOR_SIZE - 1U) & ~(FLASH_SECTOR_SIZE - 1U);
    flash_efc_unlock(APP_START_ADDR, esz);
    debug_uart_puts("[BOOT] regiao desbloqueada\n");

    /* APAGA toda a regiao com EPA (o EWP nao apaga fora dos primeiros 16KB) */
    for (uint32_t a = APP_START_ADDR; a < APP_START_ADDR + esz; a += FLASH_SECTOR_SIZE)
    {
        if (flash_efc_erase_sector(a) != FLASH_EFC_OK)
        {
            debug_uart_hex("[BOOT] ERASE FALHOU a=", a);
            return FLASH_EFC_ERROR;
        }
    }
    debug_uart_puts("[BOOT] regiao apagada\n");

    /* Escreve pagina a pagina com WP (regiao ja apagada) */
    while (off < meta->firmware_size)
    {
        uint32_t chunk = (meta->firmware_size - off) > COPY_PAGE
                             ? COPY_PAGE
                             : (meta->firmware_size - off);
        if (chunk < COPY_PAGE)
            for (uint32_t i = 0U; i < COPY_PAGE; i++)
                copy_buf[i] = 0xFFU;

        if (qspi_boot_read(OTA_EXT_FIRMWARE_ADDR + off, copy_buf, chunk) != QSPI_BOOT_OK)
        {
            debug_uart_hex("[BOOT] qspi_read FALHOU off=", off);
            return FLASH_EFC_ERROR;
        }

        fres = flash_efc_write_page(APP_START_ADDR + off, copy_buf);
        if (fres != FLASH_EFC_OK)
        {
            debug_uart_hex("[BOOT] WRITE FALHOU na off=", off);
            return fres;
        }

        off += COPY_PAGE;
        pagenum++;
    }

    debug_uart_hex("[BOOT] paginas escritas=", pagenum);
    return FLASH_EFC_OK;
}
/* =========================================================================
 * bootloader_run — orquestra o fluxo
 * ========================================================================= */
boot_result_t bootloader_run(void)
{
    boot_init_efc();

    if (qspi_boot_init() != QSPI_BOOT_OK)
    {
        debug_uart_puts("[BOOT] qspi_init FALHOU\n");
        bootloader_jump_to_app();
    }
    debug_uart_puts("[BOOT] qspi_init OK\n");

    static ota_metadata_t meta;
    if (!boot_read_metadata(&meta))
    {
        bootloader_jump_to_app();
    }

    if (!boot_verify_external(&meta))
    {
        clear_ota_metadata();
        if (app_is_valid())
            bootloader_jump_to_app();
        while (1)
        {
        }
    }

    debug_uart_puts("[BOOT] === INICIO COPIA (RAMFUNC) ===\n");

    boot_prepare_flash_access();
    debug_uart_puts("[BOOT] cache OFF\n");

    debug_uart_hex("[BOOT] page calc=", (APP_START_ADDR - 0x00400000UL) / 512UL);
    flash_efc_result_t fres = boot_copy_firmware(&meta);

    system_cache_invalidate();
    debug_uart_puts("[BOOT] cache invalidada\n");

    /* Encontra a primeira divergencia entre flash interna e externa */
    {
        static uint8_t ext_buf[512];
        bool found = false;
        for (uint32_t off = 0U; off < meta.firmware_size && !found; off += 512U)
        {
            uint32_t chunk = (meta.firmware_size - off) > 512U ? 512U : (meta.firmware_size - off);
            qspi_boot_read(OTA_EXT_FIRMWARE_ADDR + off, ext_buf, chunk);
            const uint8_t *intf = (const uint8_t *)(APP_START_ADDR + off);
            for (uint32_t k = 0U; k < chunk; k++)
            {
                if (intf[k] != ext_buf[k])
                {
                    debug_uart_hex("[BOOT] DIVERGE off=", off + k);
                    debug_uart_hex("  interna=", intf[k]);
                    debug_uart_hex("  externa=", ext_buf[k]);
                    found = true;
                    break;
                }
            }
        }
        if (!found)
            debug_uart_puts("[BOOT] flash interna == externa (sem diverg)\n");
    }

    uint32_t crc_normal = boot_compute_internal_crc(meta.firmware_size);
    uint32_t crc_ram = flash_efc_crc_ramfunc(APP_START_ADDR, meta.firmware_size);
    debug_uart_hex("[BOOT] CRC normal=", crc_normal);
    debug_uart_hex("[BOOT] CRC ramfunc=", crc_ram);
    debug_uart_hex("[BOOT] CRC esperado=", meta.firmware_crc32);

    if (fres != FLASH_EFC_OK || crc_normal != meta.firmware_crc32)
    {
        debug_uart_puts("[BOOT] COPIA FALHOU -> app antiga\n");
        if (app_is_valid())
            bootloader_jump_to_app();
        while (1)
        {
        }
    }

    debug_uart_puts("[BOOT] === COPIA OK ===\n");
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