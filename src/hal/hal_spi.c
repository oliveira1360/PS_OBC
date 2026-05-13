/**
 * @file hal_spi.c
 * @brief Hardware Abstraction Layer para SPI.
 *
 * Usa USE_REAL_HW em board.h:
 *   0 → dados simulados (propulsor fictício)
 *   1 → hardware real via SPI0 do ATSAMV71Q21
 *         MOSI = PD21 (SPI0_MOSI)
 *         MISO = PD20 (SPI0_MISO)
 *         SCK  = PD22 (SPI0_SPCK)
 *         CS   = PD25 (SPI0_NPCS1)
 */

#include "hal/hal_spi.h"
#include "config/board.h"

#if !USE_REAL_HW
#include <stdlib.h>
#include <time.h>
#endif

#if USE_REAL_HW

/* PMC */
#define PMC_BASE 0x400E0600UL
#define PMC_PCER0 (*(volatile uint32_t *)(PMC_BASE + 0x10U))

/* PIOD — pinos SPI0 estão no Port D */
#define PIOD_BASE 0x400E1400UL
#define PIOD_PDR (*(volatile uint32_t *)(PIOD_BASE + 0x04U))
#define PIOD_ABCDSR0 (*(volatile uint32_t *)(PIOD_BASE + 0x70U))
#define PIOD_ABCDSR1 (*(volatile uint32_t *)(PIOD_BASE + 0x74U))
#define PIOD_PER (*(volatile uint32_t *)(PIOD_BASE + 0x00U))
#define PIOD_OER (*(volatile uint32_t *)(PIOD_BASE + 0x10U))
#define PIOD_SODR (*(volatile uint32_t *)(PIOD_BASE + 0x30U))
#define PIOD_CODR (*(volatile uint32_t *)(PIOD_BASE + 0x34U))

/* SPI0 pin masks no Port D
 * PD20 = MISO (SPI0_MISO)  — Peripheral B
 * PD21 = MOSI (SPI0_MOSI)  — Peripheral B
 * PD22 = SCK  (SPI0_SPCK)  — Peripheral B
 * PD25 = CS   (SPI0_NPCS1) — Peripheral B
 *
 * Peripheral B no SAMV71: ABCDSR0=1, ABCDSR1=0
 */
#define PIO_MISO (1UL << 20)
#define PIO_MOSI (1UL << 21)
#define PIO_SCK (1UL << 22)
#define PIO_CS1 (1UL << 25)
#define SPI_PIN_MASK (PIO_MISO | PIO_MOSI | PIO_SCK)

/* SPI0 Mode Register — peripheral select via PCS field */
#define SPI_MR_PCS_NPCS1 (0x01UL << 16) /* Select NPCS1 (PD25) */

/* SPI0 CSR1 bits (Chip Select Register for NPCS1) */
#define SPI_CSR1_OFFSET 0x34U
#define SPI0_CSR1 REG(SPI0_BASE + SPI_CSR1_OFFSET)

/* Baud rate divider: SCK = MCK / SCBR
 * With MCK=12MHz, SCBR=12 → SCK=1MHz */
#define SPI_BAUD_DIV 255U


#define ID_PIOD 16U /* Peripheral ID for PIOD — Página 57 */

#endif /* USE_REAL_HW */

/* ==========================================================================
 * SIMULAÇÃO (USE_REAL_HW == 0)
 * ========================================================================== */
#if !USE_REAL_HW

static uint8_t tx_ready = 1U;
static uint8_t rx_ready = 0U;
static uint8_t rx_data = 0x00U;

static uint8_t prop_frame[9];
static uint8_t prop_byte_idx = 0U;

static void prop_generate_frame(void)
{
    uint8_t *f = prop_frame;

    f[0] = 0x00U; /* STATUS: IDLE */

    uint16_t press_raw = 150U + (uint16_t)(rand() % 101U);
    f[1] = (uint8_t)(press_raw >> 8U);
    f[2] = (uint8_t)(press_raw & 0xFFU);

    uint16_t temp_raw = 220U + (uint16_t)(rand() % 61U);
    f[3] = (uint8_t)(temp_raw >> 8U);
    f[4] = (uint8_t)(temp_raw & 0xFFU);

    f[5] = 0x00U;
    f[6] = 0x00U;
    f[7] = 0x00U;

    uint8_t chk = 0U;
    for (uint8_t j = 0U; j < 8U; j++)
        chk ^= f[j];
    f[8] = chk;

    prop_byte_idx = 0U;
}

#endif /* !USE_REAL_HW */

/* ==========================================================================
 * IMPLEMENTAÇÕES HAL
 * ========================================================================== */

uint8_t hal_spi_init(void)
{
#if USE_REAL_HW
    PMC_PCER0 = (1UL << ID_SPI0) | (1UL << ID_PIOD);

    /* Pinos SPI como Peripheral B (MISO, MOSI, SCK) */
    PIOD_PDR = SPI_PIN_MASK;          /* só PD20/21/22 */
    PIOD_ABCDSR0 |= SPI_PIN_MASK;
    PIOD_ABCDSR1 &= ~SPI_PIN_MASK;

    /* CS (PD25) como GPIO — controlo manual */
    PIOD_PER  = PIO_CS1;
    PIOD_OER  = PIO_CS1;
    PIOD_SODR = PIO_CS1;   /* CS HIGH (inativo) */

    SPI0_CR = SPI_CR_SWRST;

    /* Master, MODFDIS, fixed peripheral NPCS0 */
    SPI0_MR = SPI_MR_MSTR | SPI_MR_MODFDIS;

    /* CSR0: baud rate — PCS=NPCS0 usa CSR0 */
    SPI0_CSR0 = (SPI_BAUD_DIV << 8U);

    SPI0_CR = SPI_CR_SPIEN;
    return 1U;
#else
    srand((unsigned int)time(NULL));
    return 1U;
#endif
}

uint8_t hal_spi_tx_ready(void)
{
#if USE_REAL_HW
    return ((SPI0_SR & SPI_SR_TDRE) != 0U) ? 1U : 0U;
#else
    return tx_ready;
#endif
}

uint8_t hal_spi_rx_ready(void)
{
#if USE_REAL_HW
    return ((SPI0_SR & SPI_SR_RDRF) != 0U) ? 1U : 0U;
#else
    return rx_ready;
#endif
}

void hal_spi_cs_low(uint8_t cs_pin)
{
#if USE_REAL_HW
    (void)cs_pin;
    hal_spi_prepare_transfer();
    PIOD_CODR = PIO_CS1;  /* ativa CS */
#else
    (void)cs_pin;
    prop_generate_frame();
#endif
}

void hal_spi_cs_high(uint8_t cs_pin)
{
#if USE_REAL_HW
    (void)cs_pin;
    PIOD_SODR = PIO_CS1;
#else
    (void)cs_pin;
#endif
}

void hal_spi_send_byte(uint8_t data)
{
#if USE_REAL_HW
    SPI0_TDR = (uint32_t)data | (0x0EUL << 16);
#else
    (void)data;
    tx_ready = 1U;
    if (prop_byte_idx < sizeof(prop_frame))
        rx_data = prop_frame[prop_byte_idx++];
    else
        rx_data = 0x00U;
    rx_ready = 1U;
#endif
}

uint8_t hal_spi_read_byte(void)
{
#if USE_REAL_HW
    return (uint8_t)(SPI0_RDR & 0xFFU);
#else
    rx_ready = 0U;
    return rx_data;
#endif
}

void hal_spi_prepare_transfer(void)
{
#if USE_REAL_HW
    /* Limpa overrun/erros lendo RDR e SR */
    (void)SPI0_SR;
    (void)SPI0_RDR;
#endif
}
