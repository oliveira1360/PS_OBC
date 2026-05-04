/**
 * @file hal_i2c.c
 * @brief Hardware Abstraction Layer para o barramento I2C.
 *
 * Usa USE_REAL_HW em board.h:
 *   0 → dados simulados internamente (sem hardware)
 *   1 → hardware real via TWIHS0 do ATSAMV71Q21 (EXT1: PA03=SDA, PA04=SCL)
 *
 * Todos os nomes de registos e bits vêm de hal_i2c.h.
 */

#include <stdio.h>
#include <stdint.h>
#include "hal/hal_i2c.h"
#include "config/board.h"

#if !USE_REAL_HW
#include <stdlib.h>
#include <time.h>
#endif

#if USE_REAL_HW

/* ============================================================
 * Definições adicionais para hardware real
 *
 * PA03 = TWD0  (SDA) — Peripheral A
 * PA04 = TWCK0 (SCL) — Peripheral A
 * ============================================================ */

/* PMC */
#define PMC_BASE        0x400E0600UL
#define PMC_PCER0       (*(volatile uint32_t *)(PMC_BASE + 0x10U))

/* PIOA */
#define PIOA_BASE       0x400E0E00UL
#define PIOA_PER        (*(volatile uint32_t *)(PIOA_BASE + 0x00U))
#define PIOA_PDR        (*(volatile uint32_t *)(PIOA_BASE + 0x04U))
#define PIOA_OER        (*(volatile uint32_t *)(PIOA_BASE + 0x10U))
#define PIOA_SODR       (*(volatile uint32_t *)(PIOA_BASE + 0x30U))
#define PIOA_CODR       (*(volatile uint32_t *)(PIOA_BASE + 0x34U))
#define PIOA_ABCDSR0    (*(volatile uint32_t *)(PIOA_BASE + 0x70U))
#define PIOA_ABCDSR1    (*(volatile uint32_t *)(PIOA_BASE + 0x74U))

/* Pin masks */
#define PIO_SDA         (1UL << 3)   /* PA03 */
#define PIO_SCL         (1UL << 4)   /* PA04 */

/* TWIHS_MMR */
#define TWI_MMR_DADR_SHIFT  16U
#define TWI_MMR_MREAD       (1UL << 12)

/* TWIHS_CWGR */
#define TWI_CWGR_CKDIV_SHIFT  16U
#define TWI_CWGR_CHDIV_SHIFT   8U
#define TWI_CWGR_CLDIV_SHIFT   0U

/* Clock: MCK ~4 MHz, CKDIV=0, CLDIV=CHDIV=39 → ~50 kHz */
#define MCK_HZ              4000000UL
#define I2C_SPEED_HZ        ((uint32_t)I2C_SPEED_KHZ * 1000UL)
#define TWI_CKDIV           0UL
#define TWI_CLDIV           ((MCK_HZ / I2C_SPEED_HZ) - 3UL)
#define TWI_CHDIV           TWI_CLDIV

#define TWI_CWGR_VALUE      ((TWI_CKDIV << TWI_CWGR_CKDIV_SHIFT) | \
                             (TWI_CHDIV << TWI_CWGR_CHDIV_SHIFT) | \
                             (TWI_CLDIV << TWI_CWGR_CLDIV_SHIFT))

/* Timeouts */
#define TWI_STOP_TIMEOUT    100000UL
#define BUS_RECOVERY_DELAY  100

#endif /* USE_REAL_HW */
#if !USE_REAL_HW 
#define NUM_DEVICES (sizeof(devices) / sizeof(devices[0]))

typedef struct
{
    uint8_t addr;
    uint8_t data[18];
    uint8_t len;
} i2c_device_t;

static i2c_device_t devices[] = {
    {GNSS_ADDR, {38, 71, 9, 14, 0x02, 0x08, 7, 78, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, GNSS_BUF_LEN},
    {IMU_ADDR, {3, 2, 99, 8, 5, 10, 21, 14, 44, 0, 0, 0, 0, 0, 0, 0, 0, 0}, IMU_BUF_LEN},
    {PRESS_ADDR, {0x03, 0xF5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, PRES_BUF_LEN},
    {TEMP_ADDR, {0x00, 0x19, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, TEMP_BUF_LEN},
    {EPS_ADDR, {81, 120, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, EPS_BUF_LEN},
};

static uint8_t bus_free = 1U;
static uint8_t active_addr = 0xFFU;
static uint8_t read_index = 0U;
static uint16_t orbit_step = 0U;

static void hal_i2c_randomize(uint8_t i)
{
    switch (devices[i].addr)
    {
    case GNSS_ADDR:
    {
        orbit_step = (orbit_step + 1U) & 0x3FFU;
        uint16_t half = orbit_step & 0x1FFU;
        uint8_t lat_int = (half < 256U)
                              ? (uint8_t)(half * 51U / 256U)
                              : (uint8_t)(51U - (half - 256U) * 51U / 256U);
        uint16_t alt_km = 518U + (uint16_t)(rand() % 8);
        devices[i].data[0] = lat_int;
        devices[i].data[1] = (uint8_t)(rand() % 100);
        devices[i].data[2] = (uint8_t)((orbit_step * 2U) % 180U);
        devices[i].data[3] = (uint8_t)(rand() % 100);
        devices[i].data[4] = (uint8_t)(alt_km >> 8U);
        devices[i].data[5] = (uint8_t)(alt_km & 0xFFU);
        devices[i].data[6] = 7U;
        devices[i].data[7] = (uint8_t)(75U + rand() % 10U);
        break;
    }
    case IMU_ADDR:
        devices[i].data[0] = (uint8_t)(1U + rand() % 5U);
        devices[i].data[1] = (uint8_t)(1U + rand() % 4U);
        devices[i].data[2] = (uint8_t)(97U + rand() % 6U);
        devices[i].data[3] = (uint8_t)(4U + rand() % 8U);
        devices[i].data[4] = (uint8_t)(3U + rand() % 6U);
        devices[i].data[5] = (uint8_t)(6U + rand() % 9U);
        devices[i].data[6] = (uint8_t)(18U + rand() % 8U);
        devices[i].data[7] = (uint8_t)(10U + rand() % 7U);
        devices[i].data[8] = (uint8_t)(40U + rand() % 9U);
        break;
    case PRESS_ADDR:
    {
        uint16_t hpa = 1010U + (uint16_t)(rand() % 7U);
        devices[i].data[0] = (uint8_t)(hpa >> 8U);
        devices[i].data[1] = (uint8_t)(hpa & 0xFFU);
        break;
    }
    case TEMP_ADDR:
    {
        uint16_t temp = 22U + (uint16_t)(rand() % 16U);
        devices[i].data[0] = (uint8_t)(temp >> 8U);
        devices[i].data[1] = (uint8_t)(temp & 0xFFU);
        break;
    }
    case EPS_ADDR:
        devices[i].data[0] = (uint8_t)(33U + rand() % 10U);
        devices[i].data[1] = (uint8_t)(80U + rand() % 81U);
        break;
    default:
        break;
    }
}

#endif /* !USE_REAL_HW */

/* ==========================================================================
 * IMPLEMENTAÇÕES DAS FUNÇÕES HAL
 * ========================================================================== */

/**
 * @brief Inicializa o periférico I2C.
 *
 * USE_REAL_HW=1: Configura TWIHS0 como master a 400 kHz nos pinos PA03/PA04.
 * USE_REAL_HW=0: Inicializa seed para simulação.
 *
 * @return 1 em caso de sucesso.
 */
uint8_t hal_i2c_init(void)
{
#if USE_REAL_HW
    PMC_PCER0 = (1UL << ID_TWI0);

    PIOA_PDR = PIO_SDA | PIO_SCL;
    PIOA_ABCDSR0 &= ~(PIO_SDA | PIO_SCL);
    PIOA_ABCDSR1 &= ~(PIO_SDA | PIO_SCL);

    TWI0_CR = TWI_CR_SWRST;
    TWI0_CR = TWI_CR_MSEN | TWI_CR_SVDIS;
    (void)TWI0_SR;

    TWI0_CWGR = TWI_CWGR_VALUE;
    return 1U;
#else
    srand((unsigned int)time(NULL));
    return 1U;
#endif
}

/**
 * @brief Verifica se o barramento I2C está livre.
 * @return 1 se livre, 0 se ocupado.
 */
int hal_i2c_bus_free(void)
{
#if USE_REAL_HW
    uint32_t sr = TWI0_SR;
    /* TXCOMP=1 ou TXRDY=1 indica bus livre */
    return ((sr & (TWI_SR_TXCOMP | TWI_SR_TXRDY)) != 0U) ? 1 : 0;
#else
    return (int)bus_free;
#endif
}

/**
 * @brief Gera condição de START no barramento.
 *
 * No TWIHS do SAM V71 o START é gerado automaticamente ao configurar
 * o MMR e escrever no THR (TX) ou ao setar CR.START (RX).
 */
void hal_i2c_start(void)
{
#if USE_REAL_HW
    /* Limpa byte residual e flags */
    (void)(TWI0_RHR);
    (void)(TWI0_SR);
#else
    bus_free = 0U;
    active_addr = 0xFFU;
    read_index = 0U;
#endif
}

/**
 * @brief Gera condição de STOP no barramento.
 */
void hal_i2c_stop(void)
{
#if USE_REAL_HW
    TWI0_CR = TWI_CR_STOP;
    uint32_t timeout = TWI_STOP_TIMEOUT;
    while ((TWI0_SR & TWI_SR_TXCOMP) == 0U)
    {
        if (--timeout == 0U)
        {
            TWI0_CR = TWI_CR_SWRST;
            TWI0_CR = TWI_CR_MSEN | TWI_CR_SVDIS;
            break;
        }
    }
#else
    bus_free = 1U;
#endif
}

/**
 * @brief Envia o endereço do dispositivo com bit R/W.
 *
 * No hardware real configura o TWIHS_MMR com o endereço de 7 bits
 * e a direção (MREAD para leitura).
 *
 * @param byte Endereço I2C (7 bits << 1) | bit R/W no LSB.
 */
void hal_i2c_send_addr(uint8_t byte)
{
#if USE_REAL_HW
    uint8_t addr = byte >> 1U;
    uint8_t rw = byte & 0x01U;

    /* Configura MMR: endereço slave + direção */
    TWI0_MMR = ((uint32_t)addr << TWI_MMR_DADR_SHIFT) | (rw ? TWI_MMR_MREAD : 0UL);

    if (rw)
    {
        /* Modo leitura: gera START */
        TWI0_CR = TWI_CR_START;
    }
#else
    uint8_t addr = byte >> 1U;
    active_addr = addr;
    read_index = 0U;
    for (uint8_t i = 0U; i < NUM_DEVICES; i++)
    {
        if (devices[i].addr == addr)
        {
            hal_i2c_randomize(i);
            break;
        }
    }
#endif
}

/**
 * @brief Envia um byte de dados para o slave.
 * @param byte Byte a transmitir.
 */
void hal_i2c_send_byte(uint8_t byte)
{
#if USE_REAL_HW
    TWI0_THR = (uint32_t)byte;
#else
    (void)byte;
#endif
}

/**
 * @brief Verifica se o TX está pronto para o próximo byte.
 * @return 1 se pronto, 0 se ocupado.
 */
int hal_i2c_tx_ready(void)
{
#if USE_REAL_HW
    return ((TWI0_SR & TWI_SR_TXRDY) != 0U) ? 1 : 0;
#else
    return 1;
#endif
}

/**
 * @brief Pede ao hardware para gerar clock e receber um byte.
 *
 * No TWIHS do SAM V71 a receção é automática após o START+MMR(MREAD).
 * O hardware gera clock e deposita bytes em RHR continuamente.
 */
void hal_i2c_request_byte(void)
{
#if USE_REAL_HW
    /* Receção é automática no TWIHS após START com MREAD=1 */
#else
    /* simulação: dado sempre disponível */
#endif
}

/**
 * @brief Verifica se há um byte recebido pronto para leitura.
 * @return 1 se RXRDY, 0 caso contrário.
 */
int hal_i2c_rx_ready(void)
{
#if USE_REAL_HW
    return ((TWI0_SR & TWI_SR_RXRDY) != 0U) ? 1 : 0;
#else
    return 1;
#endif
}

/**
 * @brief Lê um byte recebido do barramento.
 * @return Byte lido do RHR.
 */
uint8_t hal_i2c_read_byte(void)
{
#if USE_REAL_HW
    return (uint8_t)(TWI0_RHR & 0xFFU);
#else
    for (uint8_t i = 0U; i < NUM_DEVICES; i++)
    {
        if (devices[i].addr == active_addr)
        {
            if (read_index < devices[i].len)
                return devices[i].data[read_index++];
            return 0x00U;
        }
    }
    return 0xFFU;
#endif
}

/**
 * @brief Verifica se o slave respondeu com ACK.
 *
 * Lê o bit NACK do Status Register. Se NACK=1 o slave não respondeu.
 *
 * @return 1 se ACK (slave respondeu), 0 se NACK.
 */
int hal_i2c_get_ack(void)
{
#if USE_REAL_HW
    return ((TWI0_SR & TWI_SR_NACK) != 0U) ? 0 : 1;
#else
    for (uint8_t i = 0U; i < NUM_DEVICES; i++)
    {
        if (devices[i].addr == active_addr)
            return 1;
    }
    return 0;
#endif
}

/**
 * @brief Envia ACK ao slave (continuação de leitura).
 *
 * No TWIHS o ACK é automático enquanto não se envia STOP.
 */
void hal_i2c_send_ack(void)
{
#if USE_REAL_HW
    /* ACK é automático no TWIHS durante receção contínua */
#else
    /* simulação: nada */
#endif
}

/**
 * @brief Envia NACK ao slave (último byte da leitura).
 *
 * No TWIHS deve-se enviar STOP antes de ler o último byte.
 * Isso gera automaticamente o NACK no bus.
 */
void hal_i2c_send_nack(void)
{
#if USE_REAL_HW
    /* STOP antes do último byte gera NACK implícito */
    TWI0_CR = TWI_CR_STOP;
#else
    /* simulação: nada */
#endif
}

void hal_i2c_restart_read(uint8_t addr)
{
#if USE_REAL_HW
    /* Limpa qualquer byte residual no RHR */
    (void)(TWI0_RHR);

    /* Reconfigura MMR para leitura e gera novo START */
    TWI0_MMR = ((uint32_t)addr << TWI_MMR_DADR_SHIFT) | TWI_MMR_MREAD;
    TWI0_CR = TWI_CR_START;
#else
    active_addr = addr;
    read_index = 0U;
    for (uint8_t i = 0U; i < NUM_DEVICES; i++)
    {
        if (devices[i].addr == addr)
        {
            hal_i2c_randomize(i);
            break;
        }
    }
#endif
}

void hal_i2c_bus_recovery(void)
{
#if USE_REAL_HW
    TWI0_CR = TWI_CR_SWRST;

    PIOA_PER = PIO_SCL;
    PIOA_OER = PIO_SCL;

    for (int i = 0; i < 9; i++)
    {
        PIOA_CODR = PIO_SCL;
        for (volatile int d = 0; d < BUS_RECOVERY_DELAY; d++) {}
        PIOA_SODR = PIO_SCL;
        for (volatile int d = 0; d < BUS_RECOVERY_DELAY; d++) {}
    }

    PIOA_PDR = PIO_SDA | PIO_SCL;

    TWI0_CR = TWI_CR_SWRST;
    TWI0_CR = TWI_CR_MSEN | TWI_CR_SVDIS;
    (void)TWI0_SR;
    TWI0_CWGR = TWI_CWGR_VALUE;
#endif
}