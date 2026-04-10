// src/drivers/spi_driver.c
#include "drivers/spi_driver.h"
#include "hal/hal_spi.h"

void spi_tick(spi_handle_t *h)
{
    switch (h->state)
    {
    case SPI_IDLE:
        break;

    case SPI_CS_LOW:
        hal_spi_cs_low(h->cs_pin);
        h->state = SPI_TRANSFER;
        break;

    case SPI_TRANSFER:
        hal_spi_send_byte(h->tx_buf ? h->tx_buf[h->index] : 0xFFU);
        h->state = SPI_WAIT_TX;
        break;

    case SPI_WAIT_TX:
        if (!hal_spi_tx_ready())
        {
            if (++h->timeout > SPI_TIMEOUT_MAX)
            {
                hal_spi_cs_high(h->cs_pin);
                h->timeout = 0U;
                h->state   = SPI_IDLE;
                if (h->callback) h->callback(-1);
            }
            break;
        }
        h->timeout = 0U;
        h->state   = SPI_WAIT_RX;
        break;

    case SPI_WAIT_RX:
        if (!hal_spi_rx_ready())
        {
            if (++h->timeout > SPI_TIMEOUT_MAX)
            {
                hal_spi_cs_high(h->cs_pin);
                h->timeout = 0U;
                h->state   = SPI_IDLE;
                if (h->callback) h->callback(-1);
            }
            break;
        }
        h->timeout = 0U;
        if (h->rx_buf)
            h->rx_buf[h->index] = hal_spi_read_byte();
        else
            (void)hal_spi_read_byte();
        h->index++;
        h->state = (h->index >= h->len) ? SPI_CS_HIGH : SPI_TRANSFER;
        break;

    case SPI_CS_HIGH:
        hal_spi_cs_high(h->cs_pin);
        h->state = SPI_STOP;
        break;

    case SPI_STOP:
        if (h->callback) h->callback(0);
        h->state = SPI_IDLE;
        break;

    default:
        break;
    }
}

void spi_transfer_async(spi_handle_t *h, uint8_t cs_pin,
                        uint8_t *tx, uint8_t *rx,
                        uint8_t len, void (*cb)(int))
{
    if (h->state != SPI_IDLE)
        return;

    h->cs_pin   = cs_pin;
    h->tx_buf   = tx;
    h->rx_buf   = rx;
    h->len      = len;
    h->index    = 0U;
    h->timeout  = 0U;
    h->callback = cb;
    h->state    = SPI_CS_LOW;
}