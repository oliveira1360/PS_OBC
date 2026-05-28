#ifndef SPI_DRIVER_H
#define SPI_DRIVER_H
#include <stdint.h>

#define SPI_TIMEOUT_MAX 200

typedef enum
{
    SPI_IDLE,
    SPI_CS_LOW,
    SPI_TRANSFER,
    SPI_WAIT_TX,
    SPI_WAIT_RX,
    SPI_CS_HIGH,
    SPI_STOP
} spi_state_t;

typedef struct
{
    spi_state_t state;
    uint8_t cs_pin;
    uint8_t *tx_buf;
    uint8_t *rx_buf;
    uint8_t len;
    uint8_t index;
    uint8_t timeout;
    void (*callback)(int);
} spi_handle_t;

void spi_tick(spi_handle_t *h);
void spi_transfer_async(spi_handle_t *h, uint8_t cs_pin, uint8_t *tx, uint8_t *rx, uint8_t len, void (*cb)(int));

#endif