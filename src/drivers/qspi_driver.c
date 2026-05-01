#include "drivers/qspi_driver.h"

void qspi_tick(qspi_handle_t *h)
{
    switch(h->state)
    {
        case QSPI_STARTING:
        case QSPI_TRANSFER:
        case QSPI_WAIT_TX:
        case QSPI_STOP:
        case QSPI_IDLE:
    }
}
