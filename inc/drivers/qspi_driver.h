#ifndef QSPI_DRIVER_H
#define QSPI_DRIVER_H

#include <stdint.h>

typedef enum {
    QSPI_IDLE,
    QSPI_STARTING,
    QSPI_TRANSFER,
    QSPI_WAIT_TX,
    QSPI_STOP
} qspi_state_t;

typedef struct {
    qspi_state_t  state;
    uint8_t      *buf;
    uint32_t      addr;   
    uint32_t      len;     
    uint32_t      index;
    uint8_t       timeout;
    void        (*callback)(int);
} qspi_handle_t;

void spi_tick(qspi_handle_t *h);


#endif