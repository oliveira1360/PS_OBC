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

/* =========================================================================
 * Helpers internos
 * ========================================================================= */

/**
 * @brief Apaga o sector de metadados OTA na flash externa.
 *        Chamado após aplicação bem-sucedida ou após erro de CRC.
 */
static void clear_ota_metadata(void)
{
    (void)qspi_boot_erase_sector(OTA_EXT_METADATA_SECTOR);
}

/**
 * @brief Verifica se a aplicação em flash interna parece válida.
 *        Critério simples: o vector de reset (offset 4) não deve ser
 *        0x00000000 nem 0xFFFFFFFF.
 */
static bool app_is_valid(void)
{
    const uint32_t *app_vectors = (const uint32_t *)APP_START_ADDR;
    uint32_t reset_handler = app_vectors[1];  /* Reset_Handler no vector[1] */

    return (reset_handler != 0x00000000UL &&
            reset_handler != 0xFFFFFFFFUL);
}

/* =========================================================================
 * bootloader_run
 * ========================================================================= */
boot_result_t bootloader_run(void)
{
    /* --- 1. Inicializa QSPI -------------------------------------------- */
    if (qspi_boot_init() != QSPI_BOOT_OK)
    {
        /* QSPI não respondeu — arranca aplicação existente sem OTA */
        bootloader_jump_to_app();
        return BOOT_OTA_READ_ERR;  /* Nunca chega aqui */
    }

    /* --- 2. Lê metadados OTA ------------------------------------------- */
    static ota_metadata_t meta;  /* Estático — evita stack overflow          */

    qspi_boot_result_t qres =
        qspi_boot_read(OTA_EXT_METADATA_ADDR,
                       (uint8_t *)&meta,
                       sizeof(meta));

    if (qres != QSPI_BOOT_OK)
    {
        /* Falha ao ler flash externa — arranca aplicação existente */
        bootloader_jump_to_app();
        return BOOT_OTA_READ_ERR;  /* Nunca chega aqui */
    }

    /* --- 3. Verifica se há OTA pendente -------------------------------- */
    if (meta.magic != OTA_MAGIC_PENDING)
    {
        /* Nenhuma actualização pendente — arranca normalmente */
        bootloader_jump_to_app();
        return BOOT_NO_OTA;  /* Nunca chega aqui */
    }

    /* --- 4. Valida tamanho do firmware --------------------------------- */
    if (meta.firmware_size == 0U || meta.firmware_size > APP_MAX_SIZE)
    {
        /* Metadados inválidos — limpa e arranca */
        clear_ota_metadata();
        bootloader_jump_to_app();
        return BOOT_OTA_SIZE_ERR;  /* Nunca chega aqui */
    }

    /* --- 5. Verifica CRC32 do firmware --------------------------------- */
    bool crc_ok = ota_verify_crc(OTA_EXT_FIRMWARE_ADDR,
                                  meta.firmware_size,
                                  meta.firmware_crc32);

    if (!crc_ok)
    {
        /* CRC falhou — limpa metadados para evitar loop de boot infinito,
         * e arranca com o firmware existente (se válido) */
        clear_ota_metadata();
        if (app_is_valid())
        {
            bootloader_jump_to_app();
        }
        /* Se não há firmware válido, fica aqui (hardfault safety) */
        while (1) {}
        return BOOT_OTA_CRC_FAIL;  /* Nunca chega aqui */
    }

    /* --- 6. Copia firmware da flash externa para a flash interna ------- */
    /*
     * O firmware é lido em blocos de W25Q_PAGE_SIZE bytes (256 B) da flash
     * externa e escrito em blocos de FLASH_PAGE_SIZE bytes (512 B) na flash
     * interna. A função flash_efc_write_firmware() trata do erase+write.
     *
     * Como o firmware pode ser até ~1 MB, não cabe num buffer estático de RAM.
     * A estratégia é ler directamente via memory-mapped QSPI (0x80000000)
     * e passar o ponteiro para flash_efc_write_firmware().
     *
     * Nota: O memory-mapped QSPI precisa de ser configurado com o frame
     * correcto antes de aceder. Usamos qspi_boot_read() internamente no
     * flash_efc_write_firmware() para ler bloco a bloco.
     *
     * Alternativa mais simples: acesso directo via ponteiro memory-mapped
     * (0x80000000 + OTA_EXT_FIRMWARE_ADDR) após configurar o IFR.
     * Optamos por esta abordagem para evitar buffer intermédio.
     */

    /* Configura QSPI para leitura memory-mapped contínua */
    QSPI_IAR = OTA_EXT_FIRMWARE_ADDR;
    QSPI_ICR = (uint32_t)W25Q_CMD_FAST_READ;
    QSPI_IFR = QSPI_IFR_WIDTH_SINGLE
             | QSPI_IFR_INSTEN
             | QSPI_IFR_ADDREN
             | QSPI_IFR_ADDRL_24
             | QSPI_IFR_DATAEN
             | QSPI_IFR_TFRTYP_READMEM
             | QSPI_IFR_NBDUM(W25Q_FAST_READ_DUMMY);
    (void)QSPI_IFR;

    /* Ponteiro para o firmware na região memory-mapped do QSPI */
    const uint8_t *fw_ptr =
        (const uint8_t *)(QSPI_MEM_BASE_ADDR + OTA_EXT_FIRMWARE_ADDR);

    /* Escreve na flash interna directamente a partir do ponteiro QSPI */
    flash_efc_result_t fres =
        flash_efc_write_firmware(APP_START_ADDR, fw_ptr, meta.firmware_size);

    /* Finaliza transferência QSPI */
    QSPI_CR = QSPI_CR_LASTXFER;
    /* Não aguarda INSTRE aqui — a leitura memory-mapped já terminou */

    if (fres != FLASH_EFC_OK)
    {
        /* Falha ao escrever — NÃO limpa metadados (permite retry) */
        /* Tenta arrancar com firmware anterior se válido */
        if (app_is_valid())
        {
            bootloader_jump_to_app();
        }
        while (1) {}
        return BOOT_OTA_FLASH_ERR;  /* Nunca chega aqui */
    }

    /* --- 7. Limpa metadados OTA (apaga flag) --------------------------- */
    clear_ota_metadata();

    /* --- 8. Salta para a nova aplicação -------------------------------- */
    bootloader_jump_to_app();

    /* Nunca chega aqui */
    return BOOT_OTA_APPLIED;
}

/* =========================================================================
 * bootloader_jump_to_app
 * ========================================================================= */
void bootloader_jump_to_app(void)
{
    /* Ponteiro para a tabela de vectores da aplicação */
    const uint32_t *app_vectors = (const uint32_t *)APP_START_ADDR;

    /* Verifica se a aplicação parece válida antes de saltar */
    if (!app_is_valid())
    {
        /* Sem aplicação válida — loop de segurança */
        while (1) {}
    }

    /* 1. Prepara sistema: desabilita IRQs, SysTick, etc. */
    system_prepare_jump();

    /* 2. Configura Vector Table Offset Register para a aplicação */
    SCB_VTOR = APP_START_ADDR;

    /* Barreira para garantir que o VTOR está actualizado */
    __asm__ volatile ("dsb" ::: "memory");
    __asm__ volatile ("isb" ::: "memory");

    /* 3. Carrega novo Stack Pointer (vector[0] da aplicação) */
    uint32_t app_sp = app_vectors[0];

    /* 4. Carrega endereço do Reset_Handler da aplicação (vector[1]) */
    uint32_t app_reset = app_vectors[1];

    /* 5. Salta para a aplicação:
     *    - Configura MSP com o SP da aplicação
     *    - Salta para o Reset_Handler
     *    Esta sequência em assembly garante que não ficam restos
     *    do stack do bootloader a afectar a aplicação. */
    __asm__ volatile (
        "msr msp, %0      \n"  /* Configura Main Stack Pointer              */
        "bx  %1           \n"  /* Branch para Reset_Handler da aplicação    */
        :
        : "r" (app_sp), "r" (app_reset)
        : "memory"
    );

    /* Nunca chega aqui */
    __builtin_unreachable();
}
