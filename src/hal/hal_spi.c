#include "hal/hal_spi.h"
#include "config/board.h"
#include <stdlib.h>  /* simulacao apenas !!! */

static uint8_t tx_ready = 1U;
static uint8_t rx_ready = 0U;
static uint8_t rx_data  = 0x00U;

uint8_t hal_spi_init(void)
{
    /* hardware real:
       PMC_PCER0 |= (1U << ID_SPI0);
       SPI0_CR   = SPI_CR_SWRST;
       SPI0_MR   = SPI_MR_MSTR | SPI_MR_MODFDIS;
       SPI0_CSR0 = (SPI_BAUD_DIV8 << 8U);
       SPI0_CR   = SPI_CR_SPIEN; */
    return 1U;
}

uint8_t hal_spi_tx_ready(void) { return tx_ready; }
uint8_t hal_spi_rx_ready(void) { return rx_ready; }

void hal_spi_cs_low(uint8_t cs_pin)
{
    (void)cs_pin;
    /* hardware: PIO_CODR = (1U << cs_pin) */
}

void hal_spi_cs_high(uint8_t cs_pin)
{
    (void)cs_pin;
    /* hardware: PIO_SODR = (1U << cs_pin) */
}

void hal_spi_send_byte(uint8_t data)
{
    (void)data;
    /* hardware: SPI0_TDR = data */
    tx_ready = 1U;
    rx_data  = (uint8_t)(rand() % 256U);  /* simulacao */
    rx_ready = 1U;
}

uint8_t hal_spi_read_byte(void)
{
    rx_ready = 0U;
    return rx_data;
    /* hardware: return (uint8_t)(SPI0_RDR & 0xFFU) */
}

void hal_spi_prepare_transfer(void) { }