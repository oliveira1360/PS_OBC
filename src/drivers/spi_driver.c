// src/drivers/spi_driver.c
#include "drivers/spi_driver.h"
#include "hal/hal_spi.h"
#include <stdio.h> 

static void spi_handle_idle(spi_handle_t *h)
{
    /* Nada a fazer */
    (void)h; 
}

static void spi_handle_cs_low(spi_handle_t *h)
{
    //printf("[SPI] Asserting CS Low (Pin: %d)\n", h->cs_pin);
    hal_spi_cs_low(h->cs_pin);
    h->state = SPI_TRANSFER;
}

static void spi_handle_transfer(spi_handle_t *h)
{
    // printf("[SPI] Transferring byte %d/%d\n", h->index + 1, h->len);
    hal_spi_send_byte(h->tx_buf ? h->tx_buf[h->index] : 0xFFU);
    h->state = SPI_WAIT_TX;
}

static void spi_handle_wait_tx(spi_handle_t *h)
{
    if (!hal_spi_tx_ready())
    {
        if (++h->timeout > SPI_TIMEOUT_MAX)
        {
            printf("[SPI] ERROR: TX Timeout!\n");
            hal_spi_cs_high(h->cs_pin);
            h->timeout = 0U;
            h->state   = SPI_IDLE;
            if (h->callback) h->callback(-1);
        }
        return;
    }
    h->timeout = 0U;
    h->state   = SPI_WAIT_RX;
}

static void spi_handle_wait_rx(spi_handle_t *h)
{
    if (!hal_spi_rx_ready())
    {
        if (++h->timeout > SPI_TIMEOUT_MAX)
        {
            printf("[SPI] ERROR: RX Timeout!\n");
            hal_spi_cs_high(h->cs_pin);
            h->timeout = 0U;
            h->state   = SPI_IDLE;
            if (h->callback) h->callback(-1);
        }
        return;
    }
    
    h->timeout = 0U;
    if (h->rx_buf)
    {
        h->rx_buf[h->index] = hal_spi_read_byte();
    }
    else
    {
        (void)hal_spi_read_byte();
    }
    
    h->index++;
    h->state = (h->index >= h->len) ? SPI_CS_HIGH : SPI_TRANSFER;
}

static void spi_handle_cs_high(spi_handle_t *h)
{
    // printf("[SPI] De-asserting CS High\n");
    hal_spi_cs_high(h->cs_pin);
    h->state = SPI_STOP;
}

static void spi_handle_stop(spi_handle_t *h)
{
    if (h->callback) h->callback(0);
    h->state = SPI_IDLE;
}

void spi_tick(spi_handle_t *h)
{
    switch (h->state)
    {
        case SPI_IDLE:       spi_handle_idle(h);       break;
        case SPI_CS_LOW:     spi_handle_cs_low(h);     break;
        case SPI_TRANSFER:   spi_handle_transfer(h);   break;
        case SPI_WAIT_TX:    spi_handle_wait_tx(h);    break;
        case SPI_WAIT_RX:    spi_handle_wait_rx(h);    break;
        case SPI_CS_HIGH:    spi_handle_cs_high(h);    break;
        case SPI_STOP:       spi_handle_stop(h);       break;
        default:                                       break;
    }
}

void spi_transfer_async(spi_handle_t *h, uint8_t cs_pin,
                        uint8_t *tx, uint8_t *rx,
                        uint8_t len, void (*cb)(int))
{
    if (h->state != SPI_IDLE) {
        //printf("[SPI] Busy! State: %d\n", h->state);
        return;
    }

    //printf("[SPI] Starting Async Transfer (Len: %d)\n", len);
    h->cs_pin   = cs_pin;
    h->tx_buf   = tx;
    h->rx_buf   = rx;
    h->len      = len;
    h->index    = 0U;
    h->timeout  = 0U;
    h->callback = cb;
    h->state    = SPI_CS_LOW;
}