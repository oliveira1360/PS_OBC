/**
 * @file flash_efc.c
 * @brief Driver bloqueante para a flash interna via EFC (ATSAMV71Q21B).
 *
 * Operações de erase usam EPA (Erase Pages) com 16 páginas = 8 KB por chamada.
 * Operações de escrita usam EWP (Erase and Write Page) de 512 bytes.
 *
 * ATENÇÃO: Antes de chamar flash_efc_write_firmware(), a I-Cache deve ser
 * desabilitada via system_cache_disable() para evitar leituras stale.
 * Após a escrita, chamar system_cache_invalidate().
 */

#include "flash_efc.h"
#include "bootloader.h"    /* APP_START_ADDR, APP_MAX_SIZE */
#include "system_samv71.h"
#include <string.h>

/* =========================================================================
 * Helpers internos
 * ========================================================================= */

/**
 * @brief Converte endereço de flash em número de página.
 */
static inline uint32_t addr_to_page(uint32_t addr)
{
    return (addr - FLASH_BASE_ADDR) / FLASH_PAGE_SIZE;
}

/**
 * @brief Envia um comando ao EFC.
 *
 * @param cmd    Código do comando (EFC_CMD_*).
 * @param farg   Argumento FARG (número de página, etc.).
 */
static inline void efc_send_cmd(uint32_t cmd, uint32_t farg)
{
    EFC_FCR = EFC_FCR_KEY | (farg << 8U) | cmd;
}

/* =========================================================================
 * flash_efc_wait_ready
 * ========================================================================= */
flash_efc_result_t flash_efc_wait_ready(void)
{
    uint32_t timeout = EFC_TIMEOUT_LOOPS;

    while (!(EFC_FSR & EFC_FSR_FRDY))
    {
        if (--timeout == 0U)
        {
            return FLASH_EFC_TIMEOUT;
        }
    }

    /* Verifica bits de erro */
    if (EFC_FSR & EFC_FSR_FLOCKE)  return FLASH_EFC_LOCKED;
    if (EFC_FSR & EFC_FSR_ERRORS)  return FLASH_EFC_ERROR;

    return FLASH_EFC_OK;
}

/* =========================================================================
 * flash_efc_unlock
 * ========================================================================= */
flash_efc_result_t flash_efc_unlock(uint32_t addr, uint32_t size)
{
    /* Itera sobre os lock regions afectados e limpa cada um (CLB) */
    uint32_t end  = addr + size;
    uint32_t cur  = addr;

    /* Lock region = 16 KB no SAMV71Q21 (32 páginas de 512 B) */
    const uint32_t LOCK_REGION_SIZE = 16U * 1024U;

    while (cur < end)
    {
        uint32_t page = addr_to_page(cur & ~(LOCK_REGION_SIZE - 1U));
        efc_send_cmd(EFC_CMD_CLB, page);

        flash_efc_result_t res = flash_efc_wait_ready();
        if (res != FLASH_EFC_OK) return res;

        cur += LOCK_REGION_SIZE;
    }

    return FLASH_EFC_OK;
}

/* =========================================================================
 * flash_efc_erase_sector — apaga 16 páginas (8 KB) alinhadas
 * ========================================================================= */
flash_efc_result_t flash_efc_erase_sector(uint32_t addr)
{
    /* Verifica alinhamento */
    if (addr & (FLASH_SECTOR_SIZE - 1U))
    {
        return FLASH_EFC_ALIGN_ERR;
    }

    /* Verifica que não apaga o bootloader */
    if (addr < APP_START_ADDR)
    {
        return FLASH_EFC_RANGE_ERR;
    }

    uint32_t page = addr_to_page(addr);

    /* EPA: FARG = page_number[15:2] | type[1:0]
     * type = EFC_EPA_TYPE_16 = 0x01 → 16 páginas (8 KB) */
    uint32_t farg = (page & ~0x03U) | (uint32_t)EFC_EPA_TYPE_16;

    efc_send_cmd(EFC_CMD_EPA, farg);

    return flash_efc_wait_ready();
}

/* =========================================================================
 * flash_efc_erase_region
 * ========================================================================= */
flash_efc_result_t flash_efc_erase_region(uint32_t start, uint32_t size)
{
    flash_efc_result_t res;

    /* Alinha start para baixo ao sector */
    uint32_t aligned_start = start & ~((uint32_t)(FLASH_SECTOR_SIZE - 1U));

    /* Alinha end para cima ao sector */
    uint32_t end = start + size;
    uint32_t aligned_end = (end + FLASH_SECTOR_SIZE - 1U)
                         & ~((uint32_t)(FLASH_SECTOR_SIZE - 1U));

    /* Desbloqueia a região */
    res = flash_efc_unlock(aligned_start, aligned_end - aligned_start);
    if (res != FLASH_EFC_OK) return res;

    /* Apaga sector a sector */
    for (uint32_t addr = aligned_start; addr < aligned_end;
         addr += FLASH_SECTOR_SIZE)
    {
        res = flash_efc_erase_sector(addr);
        if (res != FLASH_EFC_OK) return res;
    }

    return FLASH_EFC_OK;
}

/* =========================================================================
 * flash_efc_write_page — escreve 512 bytes
 * ========================================================================= */
flash_efc_result_t flash_efc_write_page(uint32_t addr, const uint8_t *data)
{
    /* Verifica alinhamento */
    if (addr & (FLASH_PAGE_SIZE - 1U))
    {
        return FLASH_EFC_ALIGN_ERR;
    }

    /* Não permite escrever na região do bootloader */
    if (addr < APP_START_ADDR)
    {
        return FLASH_EFC_RANGE_ERR;
    }

    /* Escreve os 512 bytes via writes de 32 bits para o latch buffer.
     * O hardware do SAMV71 dirige estas escritas ao page latch, não à flash.
     * O conteúdo só é comprometido à flash quando se envia o comando WP/EWP. */
    volatile uint32_t *flash_ptr = (volatile uint32_t *)addr;
    const uint32_t    *src32     = (const uint32_t *)data;
    uint32_t           words     = FLASH_PAGE_SIZE / 4U;

    for (uint32_t i = 0U; i < words; i++)
    {
        flash_ptr[i] = src32[i];
    }

    /* Barreira de memória para garantir que todas as escritas ao latch
     * estão completas antes de enviar o comando WP */
    __asm__ volatile ("dsb" ::: "memory");

    /* Comando WP (Write Page): FARG = número da página */
    uint32_t page = addr_to_page(addr);
    efc_send_cmd(EFC_CMD_WP, page);

    return flash_efc_wait_ready();
}

/* =========================================================================
 * flash_efc_write_firmware
 * ========================================================================= */
flash_efc_result_t flash_efc_write_firmware(uint32_t dest_addr,
                                              const uint8_t *src,
                                              uint32_t size)
{
    flash_efc_result_t res;

    /* Validações básicas */
    if (dest_addr < APP_START_ADDR)                   return FLASH_EFC_RANGE_ERR;
    if (size == 0U || size > APP_MAX_SIZE)            return FLASH_EFC_RANGE_ERR;
    if (dest_addr & (FLASH_PAGE_SIZE - 1U))           return FLASH_EFC_ALIGN_ERR;

    /* 1. Desabilita I-Cache antes de escrever na flash */
    system_cache_disable();

    /* 2. Apaga a região necessária */
    res = flash_efc_erase_region(dest_addr, size);
    if (res != FLASH_EFC_OK)
    {
        system_cache_invalidate();
        return res;
    }

    /* 3. Escreve página a página (512 bytes cada) */
    static uint8_t page_buf[FLASH_PAGE_SIZE];  /* Buffer estático — sem alloc */

    uint32_t bytes_written = 0U;
    uint32_t dst = dest_addr;

    while (bytes_written < size)
    {
        uint32_t chunk = size - bytes_written;
        if (chunk > FLASH_PAGE_SIZE)
        {
            chunk = FLASH_PAGE_SIZE;
        }

        /* Preenche o resto da página com 0xFF se o firmware não for
         * múltiplo de 512 bytes */
        if (chunk < FLASH_PAGE_SIZE)
        {
            for (uint32_t i = 0U; i < FLASH_PAGE_SIZE; i++)
            {
                page_buf[i] = 0xFFU;
            }
        }

        /* Copia dados para o buffer de página */
        for (uint32_t i = 0U; i < chunk; i++)
        {
            page_buf[i] = src[bytes_written + i];
        }

        /* Escreve a página */
        res = flash_efc_write_page(dst, page_buf);
        if (res != FLASH_EFC_OK)
        {
            system_cache_invalidate();
            return res;
        }

        dst           += FLASH_PAGE_SIZE;
        bytes_written += chunk;
    }

    /* 4. Invalida e reabilita I-Cache após escrever */
    system_cache_invalidate();

    return FLASH_EFC_OK;
}
