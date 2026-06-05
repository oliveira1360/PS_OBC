/**
 * @file qspi_boot.c
 * @brief Driver QSPI bloqueante para o bootloader (ATSAMV71Q21B + W25Q128).
 *
 * Implementação baseada no hal_qspi.c do projecto principal, mas totalmente
 * bloqueante (sem FSM nem super-loop). Usa os mesmos registos e o mesmo
 * esquema de Serial Memory Mode (SMM) para operações de leitura.
 */

#include "qspi_boot.h"
#include <string.h>

/* =========================================================================
 * qspi_boot_init
 * ========================================================================= */
qspi_boot_result_t qspi_boot_init(void)
{
    /* 1. Configura pinos QSPI como Peripheral A (PA11/12/13/14/17) */
    PIOA_PDR = QSPI_PIN_MASK;
    PIOA_ABCDSR0 &= ~QSPI_PIN_MASK; /* Peripheral A: ABCDSR0=0, ABCDSR1=0   */
    PIOA_ABCDSR1 &= ~QSPI_PIN_MASK;

    /* 2. Software reset */
    QSPI_CR = QSPI_CR_SWRST;

    /* 3. Modo: Serial Memory Mode, CS liberto após LASTXFER,
     *    delay mínimo inactivo de 1 ciclo MCK */
    QSPI_MR = QSPI_MR_SMM | QSPI_MR_CSMODE_LASTXFER | QSPI_MR_DLYCS(1U);

    /* 4. Clock: Mode 0 (CPOL=0, CPHA=0), SCK = MCK / (SCBR+1) = MCK/3 */
    QSPI_SCR = QSPI_SCR_SCBR(QSPI_CLK_DIV);

    /* 5. Activa QSPI e aguarda enable com timeout */
    QSPI_CR = QSPI_CR_QSPIEN;
    uint32_t timeout = QSPI_BOOT_TIMEOUT;
    while (!(QSPI_SR & QSPI_SR_QSPIENS))
    {
        if (--timeout == 0U)
        {
            return QSPI_BOOT_TIMEOUT_ERR; /* Hardware não respondeu */
        }
    }

/* Configura MPU região 7: 0x80000000 non-cacheable.
 * Sem isto, o D-Cache do Cortex-M7 serve dados stale e o CRC
 * do firmware lê lixo (mesmo problema do firmware principal). */
#define MPU_RNR_B (*(volatile uint32_t *)0xE000ED98UL)
#define MPU_RBAR_B (*(volatile uint32_t *)0xE000ED9CUL)
#define MPU_RASR_B (*(volatile uint32_t *)0xE000EDA0UL)
#define MPU_CTRL_B (*(volatile uint32_t *)0xE000ED94UL)

    MPU_RNR_B = 7UL;
    MPU_RBAR_B = QSPI_MEM_BASE_ADDR; /* 0x80000000 */
    MPU_RASR_B = 0x13080037UL;       /* Non-Cacheable, Full Access, 256MB, Enable */
    MPU_CTRL_B = 5UL;                /* Enable MPU + PRIVDEFENA */

    __asm__ volatile("dsb" ::: "memory");
    __asm__ volatile("isb" ::: "memory");

    return QSPI_BOOT_OK;
}

/* =========================================================================
 * qspi_boot_read
 * ========================================================================= */
qspi_boot_result_t qspi_boot_read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    /* Configura Fast Read (0x0B) com 8 dummy cycles via memory-mapped */
    QSPI_IAR = addr;
    QSPI_ICR = (uint32_t)W25Q_CMD_FAST_READ;
    QSPI_IFR = QSPI_IFR_WIDTH_SINGLE | QSPI_IFR_INSTEN | QSPI_IFR_ADDREN | QSPI_IFR_ADDRL_24 | QSPI_IFR_DATAEN | QSPI_IFR_TFRTYP_READMEM | QSPI_IFR_NBDUM(W25Q_FAST_READ_DUMMY);

    /* Leitura dummy do IFR para sincronizar (requisito do datasheet SAMV71) */
    (void)QSPI_IFR;

    /* Lê do espaço memory-mapped: 0x80000000 + addr */
    const volatile uint8_t *src =
        (const volatile uint8_t *)(QSPI_MEM_BASE_ADDR + addr);

    for (uint32_t i = 0U; i < len; i++)
    {
        buf[i] = src[i];
    }

    /* Sinaliza fim da transferência */
    QSPI_CR = QSPI_CR_LASTXFER;

    /* Aguarda INSTRE (instruction end) */
    uint32_t timeout = QSPI_BOOT_TIMEOUT;
    while (!(QSPI_SR & QSPI_SR_INSTRE))
    {
        if (--timeout == 0U)
        {
            return QSPI_BOOT_TIMEOUT_ERR;
        }
    }

    return QSPI_BOOT_OK;
}

/* =========================================================================
 * qspi_boot_wren — Write Enable
 * ========================================================================= */
qspi_boot_result_t qspi_boot_wren(void)
{
    uint32_t retries = 3U;

    while (retries > 0U)
    {
        /* Envia WREN (0x06): só instrução, sem endereço nem dados */
        QSPI_ICR = (uint32_t)W25Q_CMD_WREN;
        QSPI_IFR = QSPI_IFR_WIDTH_SINGLE | QSPI_IFR_INSTEN | QSPI_IFR_TFRTYP_WRITE;
        (void)QSPI_IFR;

        uint32_t timeout = QSPI_BOOT_TIMEOUT;
        while (!(QSPI_SR & QSPI_SR_INSTRE))
        {
            if (--timeout == 0U)
            {
                return QSPI_BOOT_TIMEOUT_ERR;
            }
        }

        /* Verifica WEL bit no status register */
        uint8_t sr = 0U;
        qspi_boot_result_t res = qspi_boot_read(0U, &sr, 0U);
        /* Leitura do status register via ICR */
        QSPI_ICR = (uint32_t)W25Q_CMD_READ_STATUS_1;
        QSPI_IFR = QSPI_IFR_WIDTH_SINGLE | QSPI_IFR_INSTEN | QSPI_IFR_DATAEN | QSPI_IFR_TFRTYP_READ | QSPI_IFR_NBDUM(0U);
        (void)QSPI_IFR;

        sr = *(volatile uint8_t *)QSPI_MEM_BASE_ADDR;

        QSPI_CR = QSPI_CR_LASTXFER;
        timeout = QSPI_BOOT_TIMEOUT;
        while (!(QSPI_SR & QSPI_SR_INSTRE))
        {
            if (--timeout == 0U)
                return QSPI_BOOT_TIMEOUT_ERR;
        }
        (void)res;

        if (sr & W25Q_SR1_WEL)
        {
            return QSPI_BOOT_OK; /* WEL confirmado */
        }
        retries--;
    }

    return QSPI_BOOT_WEL_ERR;
}

/* =========================================================================
 * qspi_boot_wait_busy
 * ========================================================================= */
qspi_boot_result_t qspi_boot_wait_busy(void)
{
    uint32_t timeout = QSPI_BOOT_TIMEOUT;

    while (timeout > 0U)
    {
        /* Lê status register */
        QSPI_ICR = (uint32_t)W25Q_CMD_READ_STATUS_1;
        QSPI_IFR = QSPI_IFR_WIDTH_SINGLE | QSPI_IFR_INSTEN | QSPI_IFR_DATAEN | QSPI_IFR_TFRTYP_READ | QSPI_IFR_NBDUM(0U);
        (void)QSPI_IFR;

        uint8_t sr = *(volatile uint8_t *)QSPI_MEM_BASE_ADDR;

        QSPI_CR = QSPI_CR_LASTXFER;
        uint32_t inner = QSPI_BOOT_TIMEOUT;
        while (!(QSPI_SR & QSPI_SR_INSTRE))
        {
            if (--inner == 0U)
                return QSPI_BOOT_TIMEOUT_ERR;
        }

        if (!(sr & W25Q_SR1_BUSY))
        {
            return QSPI_BOOT_OK; /* Flash livre */
        }
        timeout--;
    }

    return QSPI_BOOT_TIMEOUT_ERR;
}

/* =========================================================================
 * qspi_boot_erase_sector
 * ========================================================================= */
qspi_boot_result_t qspi_boot_erase_sector(uint32_t addr)
{
    qspi_boot_result_t res;

    /* 1. Write Enable */
    res = qspi_boot_wren();
    if (res != QSPI_BOOT_OK)
        return res;

    /* 2. Sector Erase (0x20): instrução + endereço alinhado ao sector */
    uint32_t sector_addr = addr & ~((uint32_t)(W25Q_SECTOR_SIZE - 1U));

    QSPI_IAR = sector_addr;
    QSPI_ICR = (uint32_t)W25Q_CMD_SECTOR_ERASE;
    QSPI_IFR = QSPI_IFR_WIDTH_SINGLE | QSPI_IFR_INSTEN | QSPI_IFR_ADDREN | QSPI_IFR_ADDRL_24 | QSPI_IFR_TFRTYP_WRITE;
    (void)QSPI_IFR;
    QSPI_CR = QSPI_CR_LASTXFER;

    uint32_t timeout = QSPI_BOOT_TIMEOUT;
    while (!(QSPI_SR & QSPI_SR_INSTRE))
    {
        if (--timeout == 0U)
            return QSPI_BOOT_TIMEOUT_ERR;
    }

    /* 3. Aguarda fim do erase (W25Q típico: ~50 ms) */
    return qspi_boot_wait_busy();
}

/* =========================================================================
 * qspi_boot_write_page
 * ========================================================================= */
qspi_boot_result_t qspi_boot_write_page(uint32_t addr,
                                        const uint8_t *buf,
                                        uint32_t len)
{
    qspi_boot_result_t res;

    if (len == 0U || len > W25Q_PAGE_SIZE)
        return QSPI_BOOT_TIMEOUT_ERR;

    /* 1. Write Enable */
    res = qspi_boot_wren();
    if (res != QSPI_BOOT_OK)
        return res;

    /* 2. Page Program (0x02): instrução + endereço + dados */
    QSPI_IAR = addr;
    QSPI_ICR = (uint32_t)W25Q_CMD_PAGE_PROGRAM;
    QSPI_IFR = QSPI_IFR_WIDTH_SINGLE | QSPI_IFR_INSTEN | QSPI_IFR_ADDREN | QSPI_IFR_ADDRL_24 | QSPI_IFR_DATAEN | QSPI_IFR_TFRTYP_WRITEMEM;
    (void)QSPI_IFR;

    volatile uint8_t *dst =
        (volatile uint8_t *)(QSPI_MEM_BASE_ADDR + addr);
    for (uint32_t i = 0U; i < len; i++)
    {
        dst[i] = buf[i];
    }

    QSPI_CR = QSPI_CR_LASTXFER;
    uint32_t timeout = QSPI_BOOT_TIMEOUT;
    while (!(QSPI_SR & QSPI_SR_INSTRE))
    {
        if (--timeout == 0U)
            return QSPI_BOOT_TIMEOUT_ERR;
    }

    /* 3. Aguarda fim do page program (W25Q típico: ~0.7 ms) */
    return qspi_boot_wait_busy();
}
