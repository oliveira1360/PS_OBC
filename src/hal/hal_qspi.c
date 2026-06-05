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
#define PMC_BASE 0x400E0600UL
#define PMC_PCER0 (*(volatile uint32_t *)(PMC_BASE + 0x10U))  /* IDs 0-31  */
#define PMC_PCER1 (*(volatile uint32_t *)(PMC_BASE + 0x100U)) /* IDs 32-63 */
/* QSPI peripheral ID 43 → PCER1 bit 11
 * PIOD  peripheral ID 16 → PCER0 bit 16 (necessário para escrever em PIOD) */
#define ID_PIOD 16U

/* PIOA para pinos QSPI (Peripheral A) */
#define PIOA_BASE 0x400E0E00UL
#define PIOA_PDR (*(volatile uint32_t *)(PIOA_BASE + 0x04U))
#define PIOA_ABCDSR0 (*(volatile uint32_t *)(PIOA_BASE + 0x70U))
#define PIOA_ABCDSR1 (*(volatile uint32_t *)(PIOA_BASE + 0x74U))

/* PIOD para PD31 = QIO3 / HOLD# (Peripheral A) */
#define PIOD_BASE 0x400E1400UL
#define PIOD_PDR (*(volatile uint32_t *)(PIOD_BASE + 0x04U))
#define PIOD_ABCDSR0 (*(volatile uint32_t *)(PIOD_BASE + 0x70U))
#define PIOD_ABCDSR1 (*(volatile uint32_t *)(PIOD_BASE + 0x74U))

/* QSPI pins on SAM V71 Xplained Ultra — Peripheral A:
 * PA11 = QCS   (QSPI Chip Select)
 * PA13 = QIO0  (MOSI)
 * PA12 = QIO1  (MISO)
 * PA17 = QIO2  (WP#)
 * PA14 = SCK
 * PD31 = QIO3  (HOLD#) — obrigatório! flutuante = chip suspenso
 */
#define QSPI_PIN_MASK ((1UL << 11) | (1UL << 12) | (1UL << 13) | \
                       (1UL << 14) | (1UL << 17))
#define QSPI_QIO3_MASK (1UL << 31) /* PD31 */

#endif /* USE_REAL_HW */

/* ==========================================================================
 * SECÇÃO SIMULAÇÃO (só compilada quando USE_REAL_HW == 0)
 * ========================================================================== */
#if !USE_REAL_HW

#define SIM_FLASH_SIZE (64U * 1024U) /* 64KB de flash simulada */

static uint8_t sim_flash[SIM_FLASH_SIZE];
static uint8_t sim_status_reg = 0x00U;  /* SR1: bit0=BUSY, bit1=WEL */
static uint8_t sim_busy_countdown = 0U; /* Simula tempo de write/erase */

#endif /* !USE_REAL_HW */

#ifndef ID_PIOA
#define ID_PIOA 10U
#endif

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

#define MPU_CTRL *(volatile uint32_t *)0xE000ED94UL
#define MPU_RNR *(volatile uint32_t *)0xE000ED98UL
#define MPU_RBAR *(volatile uint32_t *)0xE000ED9CUL
#define MPU_RASR *(volatile uint32_t *)0xE000EDA0UL
    /* 1. Ativa clocks no PMC:
     *    - QSPI (ID 43) em PCER1 bit 11
     *    - PIOD (ID 16) em PCER0 bit 16 — SEM este clock os writes em
     *      PIOD_PDR/ABCDSR são no-op e PD31 fica como GPIO input flutuante */
    PMC_PCER1 = (1UL << (ID_QSPI - 32U));
    PMC_PCER0 = (1UL << ID_PIOD) | (1UL << ID_PIOA);

    /* 2. Configura pinos QSPI como Peripheral A */
    PIOA_PDR = QSPI_PIN_MASK;
    PIOA_ABCDSR0 &= ~QSPI_PIN_MASK;
    PIOA_ABCDSR1 &= ~QSPI_PIN_MASK;

    /* PD31 = QIO3 / HOLD# — Peripheral A para garantir HIGH.
     * Sem PIOD clock (acima) estes writes eram no-op; PD31 flutuava,
     * o S25FL116K ficava em HOLD e todas as leituras devolviam 0xFF. */
    PIOD_PDR = QSPI_QIO3_MASK;
    PIOD_ABCDSR0 &= ~QSPI_QIO3_MASK;
    PIOD_ABCDSR1 &= ~QSPI_QIO3_MASK;

    /* 3. Software reset */
    QSPI_CR = QSPI_CR_SWRST;

    /* 4. Configura modo: Serial Memory Mode, 8-bit, CS after LASTXFER */
    QSPI_MR = QSPI_MR_SMM | QSPI_MR_NBBITS_8 | QSPI_MR_CSMODE_LASTXFER
              // | QSPI_MR_DLYBCT(QSPI_CS_HIGH_2)
              | QSPI_MR_DLYCS(1U);

    /* 5. Clock: Mode 0 (CPOL=0, CPHA=0), baudrate = MCK / (SCBR+1) */
    QSPI_SCR = QSPI_SCR_SCBR(QSPI_CLK_DIV);

    /* 6. Enable QSPI */
    QSPI_CR = QSPI_CR_QSPIEN;

    /* Espera que fique ativo — timeout evita bloqueio se HW falhar */
    {
        uint32_t _t = 0x40000U;
        while (!(QSPI_SR & QSPI_SR_QSPIENS) && --_t)
            ;
    }

    /* 7. Release from Power Down (0xAB) — por precaução se o chip ficou
     *    em power-down de uma sessão anterior. Tempo de wake-up: tRES1 ≤ 3µs. */
    QSPI_ICR = W25Q_CMD_RELEASE_PD;
    QSPI_IFR = QSPI_IFR_WIDTH_SINGLE | QSPI_IFR_INSTEN | QSPI_IFR_TFRTYP_WRITE;
    (void)QSPI_IFR;
    {
        uint32_t _t = 0x40000U;
        while (!(QSPI_SR & QSPI_SR_INSTRE) && --_t)
            ;
    }
    QSPI_CR = QSPI_CR_LASTXFER;
    /* Delay tRES1: ≥ 3µs a 12 MHz ≈ 36 ciclos — usa margem generosa */
    {
        volatile uint32_t _d = 500U;
        while (_d--)
        {
        }
    }

#define MPU_CTRL *(volatile uint32_t *)0xE000ED94UL
#define MPU_RNR *(volatile uint32_t *)0xE000ED98UL
#define MPU_RBAR *(volatile uint32_t *)0xE000ED9CUL
#define MPU_RASR *(volatile uint32_t *)0xE000EDA0UL

    MPU_RNR = 7UL;            /* Seleciona a região 7 da MPU */
    MPU_RBAR = QSPI_MEM_BASE; /* Endereço base (0x80000000) */

    /* Atributos: Non-Cacheable, Full Access, Size=256MB, Enable=1 */
    MPU_RASR = 0x13080037UL;

    /* Liga a MPU preservando o mapa de memória original para o resto (PRIVDEFENA=1) */
    MPU_CTRL = 5UL;

    __asm volatile("dsb 0xF" ::: "memory");
    __asm volatile("isb 0xF" ::: "memory");
    /* ========================================================== */

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
    QSPI_ICR = (uint32_t)cmd;
    QSPI_IFR = QSPI_IFR_WIDTH_SINGLE | QSPI_IFR_INSTEN | QSPI_IFR_TFRTYP_READ;
    (void)QSPI_IFR;

    {
        uint32_t _t = 0x40000U;
        while (!(QSPI_SR & QSPI_SR_INSTRE) && --_t)
            ;
    }

    /* Desasserta o Chip Select */
    QSPI_CR = QSPI_CR_LASTXFER;

    /* AQUI ESTÁ O SEGREDO: Dá tempo ao pino físico para subir! */
    {
        volatile uint32_t _d = 150U;
        while (_d--)
        {
        }
    }

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

    /* CORREÇÃO: TFRTYP_READ em vez de WRITE. Como o Erase não tem fase
     * de dados, usar WRITE encravava a controladora QSPI. */
    QSPI_IFR = QSPI_IFR_WIDTH_SINGLE | QSPI_IFR_INSTEN | QSPI_IFR_ADDREN | QSPI_IFR_ADDRL_24 | QSPI_IFR_TFRTYP_READ;
    (void)QSPI_IFR;

    /* Desasserta o Chip Select */
    QSPI_CR = QSPI_CR_LASTXFER;

    /* Espera que a instrução termine na FSM interna */
    {
        uint32_t _t = 0x40000U;
        while (!(QSPI_SR & QSPI_SR_INSTRE) && --_t)
            ;
    }

    /* DELAY DE HARDWARE: Dá tempo para o pino CS subir fisicamente e
     * a flash preparar o seu circuito interno de alta voltagem. */
    {
        volatile uint32_t _d = 10000U;
        while (_d--)
        {
        }
    }
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

    /* Definição manual do registo de Invalidação da Cache L1 (D-Cache) do Cortex-M7 */

    QSPI_ICR = W25Q_CMD_READ_STATUS_1;
    QSPI_IFR = QSPI_IFR_WIDTH_SINGLE | QSPI_IFR_INSTEN | QSPI_IFR_DATAEN | QSPI_IFR_TFRTYP_READ | QSPI_IFR_NBDUM(0);
    (void)QSPI_IFR;

    /* Garante que o CPU espera que a invalidação termine antes de avançar */
    __asm volatile("dsb 0xF" ::: "memory");

    /* 2. Lê 1 byte do espaço QSPI memory-mapped (agora vai fisicamente ao bus) */
    status = *(volatile uint8_t *)QSPI_MEM_BASE;
    __asm volatile("dsb 0xF" ::: "memory");

    QSPI_CR = QSPI_CR_LASTXFER;
    {
        uint32_t _t = 0x40000U;
        while (!(QSPI_SR & QSPI_SR_INSTRE) && --_t)
            ;
    }

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
 * @brief Lê dados da flash para um buffer usando memcpy (32-bit AHB accesses).
 */
/**
 * @brief Lê dados da flash para um buffer usando memcpy (32-bit AHB accesses).
 */
void hal_qspi_read_memory(uint32_t addr, uint8_t *buf, uint32_t len)
{
#if USE_REAL_HW
    QSPI_IAR = addr;
    QSPI_ICR = W25Q_CMD_FAST_READ;
    QSPI_IFR = QSPI_IFR_WIDTH_SINGLE | QSPI_IFR_INSTEN | QSPI_IFR_ADDREN | QSPI_IFR_ADDRL_24 | QSPI_IFR_DATAEN | QSPI_IFR_TFRTYP_READMEM | QSPI_IFR_NBDUM(W25Q_FAST_READ_DUMMY);
    (void)QSPI_IFR;

    /* Atraso curto para estabilizar o bus após o comando de leitura */
    {
        volatile uint32_t _d = 100U;
        while (_d--)
        {
        }
    }

    __asm volatile("cpsid i" ::: "memory");
    memcpy(buf, (const void *)(QSPI_MEM_BASE + addr), len);
    __asm volatile("dsb 0xF" ::: "memory");
    __asm volatile("cpsie i" ::: "memory");

    QSPI_CR = QSPI_CR_LASTXFER;
    {
        uint32_t _t = 0x40000U;
        while (!(QSPI_SR & QSPI_SR_INSTRE) && --_t)
            ;
    }
#else
    for (uint32_t i = 0U; i < len; i++)
    {
        buf[i] = ((addr + i) < SIM_FLASH_SIZE) ? sim_flash[addr + i] : 0xFFU;
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
    QSPI_IFR = QSPI_IFR_WIDTH_SINGLE | QSPI_IFR_INSTEN | QSPI_IFR_ADDREN | QSPI_IFR_ADDRL_24 | QSPI_IFR_DATAEN | QSPI_IFR_TFRTYP_WRITEMEM;
    (void)QSPI_IFR;

    /* Desliga as interrupções (Assembly Bare-Metal) para garantir que o SysTick
     * não interrompe o CPU a meio do memcpy, evitando FIFO underrun. */
    __asm volatile("cpsid i" ::: "memory");

    memcpy((void *)(QSPI_MEM_BASE + addr), buf, len);
    __asm volatile("dsb 0xF" ::: "memory");

    /* Volta a ligar as interrupções (Assembly Bare-Metal) */
    __asm volatile("cpsie i" ::: "memory");

    /* OBRIGATÓRIO: Esperar que a FIFO de transmissão (TX) esvazie
     * ANTES de sinalizar o LASTXFER. Bit 2 é o TXEMPTY. */
    {
        uint32_t _t = 0x40000U;
        while (!(QSPI_SR & (1UL << 2)) && --_t)
            ;
    }

    QSPI_CR = QSPI_CR_LASTXFER;
    {
        uint32_t _t = 0x40000U;
        while (!(QSPI_SR & QSPI_SR_INSTRE) && --_t)
            ;
    }

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

/**
 * @brief Limpa os bits de write-protection do W25Q (SR1 = 0x00).
 *
 * O W25Q pode ter os bits BP0-BP3/TB setados (por configuração prévia ou
 * por reset de hardware), o que faz com que os sector erases sejam ignorados
 * silenciosamente. Esta função envia WREN + WRSR(0x00) para limpar toda a
 * protecção antes de qualquer operação de erase/write OTA.
 *
 * Operação bloqueante (~15 ms para o WRSR completar no W25Q).
 */
void hal_qspi_clear_write_protection(void)
{
#if USE_REAL_HW
    /* Definição manual do registo de Limpeza da Cache L1 (D-Cache) do Cortex-M7 */

    hal_qspi_send_command(W25Q_CMD_WRITE_ENABLE);
    {
        volatile uint32_t d = 2000U;
        while (d--)
        {
        }
    }

    QSPI_ICR = 0x01U;
    QSPI_IFR = QSPI_IFR_WIDTH_SINGLE | QSPI_IFR_INSTEN | QSPI_IFR_DATAEN | QSPI_IFR_TFRTYP_WRITE;
    (void)QSPI_IFR;

    /* 1. Escrevemos o 0x00. Isto vai apenas para a Cache L1 do M7. */
    *(volatile uint8_t *)QSPI_MEM_BASE = 0x00U;
    __asm volatile("dsb 0xF" ::: "memory");

    /* 2. OBRIGATÓRIO: Obrigamos o M7 a "despejar" (Clean) esta linha de cache para o bus físico QSPI */
    __asm volatile("dsb 0xF" ::: "memory");

    QSPI_CR = QSPI_CR_LASTXFER;
    {
        uint32_t _t = 0x40000U;
        while (!(QSPI_SR & QSPI_SR_INSTRE) && --_t)
            ;
    }

    {
        uint32_t _t = 500000U;
        while (hal_qspi_is_busy() && --_t)
            ;
    }
#else
    sim_status_reg &= 0x03U;
#endif
}
