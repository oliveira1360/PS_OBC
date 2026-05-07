/**
 * @file hal_qspi.c
 * @brief Hardware Abstraction Layer para QSPI + W25Q128.
 *
 * Usa USE_REAL_HW em board.h:
 *   0 → dados simulados (memória em RAM)
 *   1 → hardware real via QSPI do ATSAMV71Q21 (Serial Memory Mode)
 *
 * O SAM V71 QSPI opera em Serial Memory Mode: o controller gere
 * automaticamente instruction + address + dummy cycles. O software
 * configura os registos ICR/IFR e depois acede à região 0x80000000
 * como memória normal.
 */

#include "hal/hal_qspi.h"
#include "config/board.h"
#include <string.h>

#if USE_REAL_HW

/* PMC */
#define PMC_BASE      0x400E0600UL
#define PMC_PCER1     (*(volatile uint32_t *)(PMC_BASE + 0x100U))
/* QSPI peripheral ID 43 → bit 43-32=11 no PCER1 */

/* PIOA para pinos QSPI (Peripheral A) */
#define PIOA_BASE     0x400E0E00UL
#define PIOA_PDR      (*(volatile uint32_t *)(PIOA_BASE + 0x04U))
#define PIOA_ABCDSR0  (*(volatile uint32_t *)(PIOA_BASE + 0x70U))
#define PIOA_ABCDSR1  (*(volatile uint32_t *)(PIOA_BASE + 0x74U))

/* QSPI pins on SAM V71 — Peripheral A:
 * PA11 = QCS   (QSPI Chip Select)
 * PA13 = QIO0  (MOSI)
 * PA12 = QIO1  (MISO)
 * PA17 = QIO2
 * PA14 = SCK
 * PD31 = QIO3  (on some packages)
 */
#define QSPI_PIN_MASK  ((1UL << 11) | (1UL << 12) | (1UL << 13) | \
                        (1UL << 14) | (1UL << 17))

#endif /* USE_REAL_HW */

/* ==========================================================================
 * SECÇÃO SIMULAÇÃO (só compilada quando USE_REAL_HW == 0)
 * ========================================================================== */
#if !USE_REAL_HW

#define SIM_FLASH_SIZE  (64U * 1024U)  /* 64KB de flash simulada */

static uint8_t sim_flash[SIM_FLASH_SIZE];
static uint8_t sim_status_reg = 0x00U;  /* SR1: bit0=BUSY, bit1=WEL */
static uint8_t sim_busy_countdown = 0U; /* Simula tempo de write/erase */

#endif /* !USE_REAL_HW */

/* ==========================================================================
 * IMPLEMENTAÇÕES HAL
 * ========================================================================== */

/**
 * @brief Inicializa o periférico QSPI.
 * @return 1 se sucesso.
 */
uint8_t hal_qspi_init(void)
{
#if USE_REAL_HW
    /* 1. Ativa clock do QSPI no PMC (peripheral ID 43 → PCER1 bit 11) */
    PMC_PCER1 = (1UL << (ID_QSPI - 32U));

    /* 2. Configura pinos QSPI como Peripheral A */
    PIOA_PDR = QSPI_PIN_MASK;
    PIOA_ABCDSR0 &= ~QSPI_PIN_MASK;  /* Peripheral A: ABCDSR0=0, ABCDSR1=0 */
    PIOA_ABCDSR1 &= ~QSPI_PIN_MASK;

    /* 3. Software reset */
    QSPI_CR = QSPI_CR_SWRST;

    /* 4. Configura modo: Serial Memory Mode, 8-bit, CS after LASTXFER */
    QSPI_MR = QSPI_MR_SMM
            | QSPI_MR_NBBITS_8
            | QSPI_MR_CSMODE_LASTXFER
           // | QSPI_MR_DLYBCT(QSPI_CS_HIGH_2)
            | QSPI_MR_DLYCS(1U);

    /* 5. Clock: Mode 0 (CPOL=0, CPHA=0), baudrate = MCK / (SCBR+1) */
    QSPI_SCR = QSPI_SCR_SCBR(QSPI_CLK_DIV);

    /* 6. Enable QSPI */
    QSPI_CR = QSPI_CR_QSPIEN;

    /* Espera que fique ativo */
    while (!(QSPI_SR & QSPI_SR_QSPIENS))
        ;

    return 1U;

#else
    /* Simulação: inicializa flash com 0xFF (estado erased) */
    memset(sim_flash, 0xFF, SIM_FLASH_SIZE);
    sim_status_reg = 0x00U;
    sim_busy_countdown = 0U;
    return 1U;
#endif
}

/**
 * @brief Envia um comando simples (sem endereço, sem dados).
 *        Ex: WREN (0x06), WRDI (0x04), Chip Erase.
 * @param cmd Opcode do comando.
 */
void hal_qspi_send_command(uint8_t cmd)
{
#if USE_REAL_HW
    /* Configura instruction frame: só instruction, sem addr/data */
    QSPI_ICR = (uint32_t)cmd;
    QSPI_IFR = QSPI_IFR_WIDTH_SINGLE
             | QSPI_IFR_INSTEN
             | QSPI_IFR_TFRTYP_READ;
    /* Dummy read do IFR para sincronizar (datasheet requirement) */
    (void)QSPI_IFR;

    /* Espera INSTRE (instruction end) */
    while (!(QSPI_SR & QSPI_SR_INSTRE))
        ;

#else
    if (cmd == W25Q_CMD_WRITE_ENABLE)
    {
        sim_status_reg |= W25Q_SR1_WEL;
    }
    else if (cmd == W25Q_CMD_WRITE_DISABLE)
    {
        sim_status_reg &= ~W25Q_SR1_WEL;
    }
    else if (cmd == W25Q_CMD_CHIP_ERASE)
    {
        if (sim_status_reg & W25Q_SR1_WEL)
        {
            memset(sim_flash, 0xFF, SIM_FLASH_SIZE);
            sim_status_reg |= W25Q_SR1_BUSY;
            sim_busy_countdown = 5U;
            sim_status_reg &= ~W25Q_SR1_WEL;
        }
    }
#endif
}

/**
 * @brief Envia um comando com endereço (sem dados).
 *        Ex: Sector Erase (0x20), Block Erase.
 * @param cmd  Opcode do comando.
 * @param addr Endereço de 24 bits.
 */
void hal_qspi_send_command_addr(uint8_t cmd, uint32_t addr)
{
#if USE_REAL_HW
    QSPI_IAR = addr;
    QSPI_ICR = (uint32_t)cmd;
    QSPI_IFR = QSPI_IFR_WIDTH_SINGLE
             | QSPI_IFR_INSTEN
             | QSPI_IFR_ADDREN
             | QSPI_IFR_ADDRL_24
             | QSPI_IFR_TFRTYP_WRITE;
    (void)QSPI_IFR;

    while (!(QSPI_SR & QSPI_SR_INSTRE))
        ;

#else
    if (cmd == W25Q_CMD_SECTOR_ERASE && (sim_status_reg & W25Q_SR1_WEL))
    {
        uint32_t base = addr & ~(W25Q_SECTOR_SIZE - 1U);
        if (base + W25Q_SECTOR_SIZE <= SIM_FLASH_SIZE)
        {
            memset(&sim_flash[base], 0xFF, W25Q_SECTOR_SIZE);
        }
        sim_status_reg |= W25Q_SR1_BUSY;
        sim_busy_countdown = 3U;
        sim_status_reg &= ~W25Q_SR1_WEL;
    }
    else if (cmd == W25Q_CMD_BLOCK_ERASE_64 && (sim_status_reg & W25Q_SR1_WEL))
    {
        uint32_t base = addr & ~(W25Q_BLOCK_SIZE_64K - 1U);
        if (base + W25Q_BLOCK_SIZE_64K <= SIM_FLASH_SIZE)
        {
            memset(&sim_flash[base], 0xFF, W25Q_BLOCK_SIZE_64K);
        }
        sim_status_reg |= W25Q_SR1_BUSY;
        sim_busy_countdown = 4U;
        sim_status_reg &= ~W25Q_SR1_WEL;
    }
#endif
}

/**
 * @brief Lê o Status Register 1 do W25Q.
 * @return Valor do SR1 (bit0=BUSY, bit1=WEL).
 */
uint8_t hal_qspi_read_status(void)
{
#if USE_REAL_HW
    uint8_t status;

    QSPI_ICR = W25Q_CMD_READ_STATUS_1;
    QSPI_IFR = QSPI_IFR_WIDTH_SINGLE
             | QSPI_IFR_INSTEN
             | QSPI_IFR_DATAEN
             | QSPI_IFR_TFRTYP_READ
             | QSPI_IFR_NBDUM(0);
    (void)QSPI_IFR;

    /* Lê 1 byte do espaço QSPI memory-mapped */
    status = *(volatile uint8_t *)QSPI_MEM_BASE;

    /* Sinaliza fim da transferência */
    QSPI_CR = QSPI_CR_LASTXFER;
    while (!(QSPI_SR & QSPI_SR_INSTRE))
        ;

    return status;

#else
    /* Simulação: decrementa busy countdown */
    if (sim_busy_countdown > 0U)
    {
        sim_busy_countdown--;
        if (sim_busy_countdown == 0U)
        {
            sim_status_reg &= ~W25Q_SR1_BUSY;
        }
    }
    return sim_status_reg;
#endif
}

/**
 * @brief Verifica se a flash está ocupada (write/erase em curso).
 * @return 1 se busy, 0 se livre.
 */
uint8_t hal_qspi_is_busy(void)
{
    return (hal_qspi_read_status() & W25Q_SR1_BUSY) ? 1U : 0U;
}

/**
 * @brief Verifica se a última instrução QSPI terminou.
 * @return 1 se terminou, 0 se ainda em curso.
 */
uint8_t hal_qspi_instruction_done(void)
{
#if USE_REAL_HW
    return (QSPI_SR & QSPI_SR_INSTRE) ? 1U : 0U;
#else
    return 1U;
#endif
}

/**
 * @brief Lê dados da flash para um buffer.
 *
 * Usa Fast Read (0x0B) com 8 dummy cycles.
 *
 * @param addr Endereço na flash (24-bit).
 * @param buf  Buffer de destino.
 * @param len  Número de bytes a ler.
 */
void hal_qspi_read_memory(uint32_t addr, uint8_t *buf, uint32_t len)
{
#if USE_REAL_HW
    QSPI_IAR = addr;
    QSPI_ICR = W25Q_CMD_FAST_READ;
    QSPI_IFR = QSPI_IFR_WIDTH_SINGLE
             | QSPI_IFR_INSTEN
             | QSPI_IFR_ADDREN
             | QSPI_IFR_ADDRL_24
             | QSPI_IFR_DATAEN
             | QSPI_IFR_TFRTYP_READMEM
             | QSPI_IFR_NBDUM(W25Q_FAST_READ_DUMMY);
    (void)QSPI_IFR;

    /* Lê do espaço memory-mapped */
    volatile uint8_t *src = (volatile uint8_t *)(QSPI_MEM_BASE + addr);
    for (uint32_t i = 0U; i < len; i++)
    {
        buf[i] = src[i];
    }

    QSPI_CR = QSPI_CR_LASTXFER;
    while (!(QSPI_SR & QSPI_SR_INSTRE))
        ;

#else
    /* Simulação: copia da RAM */
    for (uint32_t i = 0U; i < len; i++)
    {
        if ((addr + i) < SIM_FLASH_SIZE)
        {
            buf[i] = sim_flash[addr + i];
        }
        else
        {
            buf[i] = 0xFFU;
        }
    }
#endif
}

/**
 * @brief Escreve dados num page buffer da flash.
 *
 * ATENÇÃO: o Write Enable (WREN) deve ser enviado ANTES desta chamada.
 * Máximo 256 bytes por página. O endereço não pode cruzar fronteira de página.
 *
 * @param addr Endereço na flash (24-bit).
 * @param buf  Buffer de origem.
 * @param len  Número de bytes a escrever (max 256).
 */
void hal_qspi_write_memory(uint32_t addr, const uint8_t *buf, uint32_t len)
{
#if USE_REAL_HW
    QSPI_IAR = addr;
    QSPI_ICR = W25Q_CMD_PAGE_PROGRAM;
    QSPI_IFR = QSPI_IFR_WIDTH_SINGLE
             | QSPI_IFR_INSTEN
             | QSPI_IFR_ADDREN
             | QSPI_IFR_ADDRL_24
             | QSPI_IFR_DATAEN
             | QSPI_IFR_TFRTYP_WRITEMEM;
    (void)QSPI_IFR;

    /* Escreve no espaço memory-mapped */
    volatile uint8_t *dst = (volatile uint8_t *)(QSPI_MEM_BASE + addr);
    for (uint32_t i = 0U; i < len; i++)
    {
        dst[i] = buf[i];
    }

    QSPI_CR = QSPI_CR_LASTXFER;
    while (!(QSPI_SR & QSPI_SR_INSTRE))
        ;

#else
    /* Simulação: Page Program — só pode mudar bits de 1→0 */
    if (sim_status_reg & W25Q_SR1_WEL)
    {
        for (uint32_t i = 0U; i < len; i++)
        {
            if ((addr + i) < SIM_FLASH_SIZE)
            {
                sim_flash[addr + i] &= buf[i]; /* AND — simula flash write */
            }
        }
        sim_status_reg |= W25Q_SR1_BUSY;
        sim_busy_countdown = 2U;
        sim_status_reg &= ~W25Q_SR1_WEL;
    }
#endif
}