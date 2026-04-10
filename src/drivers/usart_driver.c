#include "drivers/usart_driver.h"
#include "hal/hal_usart.h"

void usart_tick(usart_handle_t *h)
{
    switch (h->state)
    {
    case SERIAL_IDLE:
        break;

    case SERIAL_TX_BUSY:
        if (!hal_usart_is_tx_ready())
        {
            if (++h->timeout > USART_TIMEOUT_MAX)
            {
                h->timeout = 0;
                h->state = SERIAL_ERROR;
                if (h->callback)
                    h->callback(-1);
            }
            break;
        }
        h->timeout = 0;
        hal_usart_write_char(h->tx_buf[h->tx_index]);
        h->tx_index++;
        if (h->tx_index >= h->tx_len)
        {
            h->state = SERIAL_IDLE;
            if (h->callback)
                h->callback(0);
        }
        break;

    case SERIAL_RX_BUSY:
        if (!hal_usart_data_available())
        {
            if (++h->timeout > USART_TIMEOUT_MAX)
            {
                h->timeout = 0;
                h->state = SERIAL_ERROR;
                if (h->callback)
                    h->callback(-1);
            }
            break;
        }
        h->timeout = 0;
        h->rx_buf[h->rx_index] = hal_usart_read_char(); // ← lê e avança rx_index
        h->rx_index++;
        if (h->rx_index >= h->rx_len) // ← só termina quando rx_len bytes lidos
        {
            h->state = SERIAL_IDLE;
            if (h->callback)
                h->callback(0); // ← chama done aqui
        }
        break;

    case SERIAL_ERROR:
        break;

    default:
        break;
    }
}

void usart_send_async(usart_handle_t *h, uint8_t *data, uint8_t len)
{
    if (h->state != SERIAL_IDLE)
        return;

    h->tx_buf = data;
    h->tx_len = len;
    h->tx_index = 0;
    h->timeout = 0;
    h->state = SERIAL_TX_BUSY;
}

void usart_recv_async(usart_handle_t *h, uint8_t *buf, uint8_t len, void (*cb)(int))
{
    if (h->state != SERIAL_IDLE)
        return;

    h->rx_buf = buf;
    h->rx_len = len;
    h->rx_index = 0;
    h->timeout = 0;
    h->callback = cb;
    h->state = SERIAL_RX_BUSY;
}