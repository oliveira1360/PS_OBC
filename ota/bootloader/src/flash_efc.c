#include "flash_efc.h"
#include "bootloader.h"
#include "system_samv71.h"
#include <string.h>

void debug_uart_hex(const char *label, uint32_t val);
void debug_uart_puts(const char *s);

#define RAMFUNC __attribute__((section(".ramfunc"), noinline, long_call))

static inline uint32_t addr_to_page(uint32_t addr)
{
    return (addr - FLASH_BASE_ADDR) / FLASH_PAGE_SIZE;
}

RAMFUNC static void efc_send_cmd(uint32_t cmd, uint32_t farg)
{
    EFC_FCR = EFC_FCR_KEY | (farg << 8U) | cmd;
}

RAMFUNC flash_efc_result_t flash_efc_wait_ready(void)
{
    uint32_t timeout = EFC_TIMEOUT_LOOPS;
    while (!(EFC_FSR & EFC_FSR_FRDY))
    {
        if (--timeout == 0U)
            return FLASH_EFC_TIMEOUT;
    }
    if (EFC_FSR & EFC_FSR_FLOCKE)
        return FLASH_EFC_LOCKED;
    if (EFC_FSR & EFC_FSR_ERRORS)
        return FLASH_EFC_ERROR;
    return FLASH_EFC_OK;
}

flash_efc_result_t flash_efc_unlock(uint32_t addr, uint32_t size)
{
    uint32_t end = addr + size;
    uint32_t cur = addr;
    const uint32_t LOCK_REGION_SIZE = 16U * 1024U;
    while (cur < end)
    {
        uint32_t page = addr_to_page(cur & ~(LOCK_REGION_SIZE - 1U));
        efc_send_cmd(EFC_CMD_CLB, page);
        flash_efc_result_t res = flash_efc_wait_ready();
        if (res != FLASH_EFC_OK)
            return res;
        cur += LOCK_REGION_SIZE;
    }
    return FLASH_EFC_OK;
}

RAMFUNC flash_efc_result_t flash_efc_erase_sector(uint32_t addr)
{
    if (addr & (FLASH_SECTOR_SIZE - 1U)) return FLASH_EFC_ALIGN_ERR;
    if (addr < APP_START_ADDR)           return FLASH_EFC_RANGE_ERR;

    uint32_t page = addr_to_page(addr);
    uint32_t farg = (page & ~0x0FU) | 2U;  /* EPA 16 paginas (tipo=2), pagina mult. de 16 */

    for (uint32_t attempt = 0U; attempt < 4U; attempt++)
    {
        efc_send_cmd(EFC_CMD_EPA, farg);
        flash_efc_result_t res = flash_efc_wait_ready();
        if (res != FLASH_EFC_OK) continue;

        volatile uint32_t *p = (volatile uint32_t *)addr;
        if (p[0] == 0xFFFFFFFFUL && p[(FLASH_SECTOR_SIZE/4)-1] == 0xFFFFFFFFUL)
            return FLASH_EFC_OK;
    }
    return FLASH_EFC_ERROR;
}

flash_efc_result_t flash_efc_erase_region(uint32_t start, uint32_t size)
{
    flash_efc_result_t res;
    uint32_t aligned_start = start & ~((uint32_t)(FLASH_SECTOR_SIZE - 1U));
    uint32_t end = start + size;
    uint32_t aligned_end = (end + FLASH_SECTOR_SIZE - 1U) & ~((uint32_t)(FLASH_SECTOR_SIZE - 1U));

    res = flash_efc_unlock(aligned_start, aligned_end - aligned_start);
    if (res != FLASH_EFC_OK)
        return res;

    for (uint32_t addr = aligned_start; addr < aligned_end; addr += FLASH_SECTOR_SIZE)
    {
        res = flash_efc_erase_sector(addr);
        if (res != FLASH_EFC_OK)
            return res;
    }
    return FLASH_EFC_OK;
}

/* RAMFUNC: corre da RAM. NAO chamar debug_uart aqui dentro (estao na flash). */
RAMFUNC flash_efc_result_t flash_efc_write_page(uint32_t addr, const uint8_t *data)
{
    if (addr & (FLASH_PAGE_SIZE - 1U)) return FLASH_EFC_ALIGN_ERR;
    if (addr < APP_START_ADDR)          return FLASH_EFC_RANGE_ERR;

    volatile uint32_t *flash_ptr = (volatile uint32_t *)addr;
    uint32_t words = FLASH_PAGE_SIZE / 4U;

    for (uint32_t i = 0U; i < words; i++)
    {
        uint32_t w = (uint32_t)data[i*4+0] | ((uint32_t)data[i*4+1]<<8)
                   | ((uint32_t)data[i*4+2]<<16) | ((uint32_t)data[i*4+3]<<24);
        flash_ptr[i] = w;
    }
    __asm__ volatile("dsb" ::: "memory");
    efc_send_cmd(EFC_CMD_WP, addr_to_page(addr));   /* WP — pagina ja apagada pelo EPA */
    flash_efc_result_t res = flash_efc_wait_ready();
    for (volatile uint32_t d = 0; d < 2000U; d++) { }

    const uint32_t *fchk = (const uint32_t *)addr;
    for (uint32_t i = 0U; i < words; i++)
    {
        uint32_t want = (uint32_t)data[i*4+0] | ((uint32_t)data[i*4+1]<<8)
                      | ((uint32_t)data[i*4+2]<<16) | ((uint32_t)data[i*4+3]<<24);
        if (fchk[i] != want) return FLASH_EFC_ERROR;
    }
    return res;
}

flash_efc_result_t flash_efc_write_firmware(uint32_t dest_addr, const uint8_t *src, uint32_t size)
{
    (void)dest_addr;
    (void)src;
    (void)size;
    return FLASH_EFC_OK; /* nao usada — a copia esta no bootloader_run */
}

RAMFUNC uint32_t flash_efc_crc_ramfunc(uint32_t addr, uint32_t size)
{
    uint32_t crc = 0xFFFFFFFFUL;
    const uint8_t *p = (const uint8_t *)addr;
    for (uint32_t i = 0U; i < size; i++)
    {
        crc ^= (uint32_t)p[i];
        for (uint8_t b = 0U; b < 8U; b++)
            crc = (crc & 1UL) ? (crc >> 1) ^ 0xEDB88320UL : (crc >> 1);
    }
    return crc ^ 0xFFFFFFFFUL;
}