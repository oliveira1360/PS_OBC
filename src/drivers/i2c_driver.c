#include "drivers/i2c_driver.h"
#include "hal/hal_i2c.h"

void i2c_tick(i2c_handle_t *h)
{
    switch (h->state)
    {

    case I2C_STARTING:
        if (!hal_i2c_bus_free())
            break;
        hal_i2c_start();
        hal_i2c_send_addr((h->addr << 1) | (h->rw ? 1 : 0));
        h->state = I2C_SELECT_MODE;
        break;

    case I2C_SELECT_MODE:
        if (!hal_i2c_get_ack())
        {
            h->state = I2C_IDLE;
            if (h->callback)
                h->callback(-1);
            break;
        }
        h->index = 0;
        h->state = h->rw ? I2C_READ : I2C_WRITE;
        break;

    case I2C_WRITE:
        hal_i2c_send_byte(h->buf[h->index]);
        h->state = I2C_WAIT_TX;
        break;

    case I2C_WAIT_TX:
        if (!hal_i2c_tx_ready())
        {
            if (++h->timeout > I2C_TIMEOUT_MAX)
            {
                hal_i2c_stop();
                h->timeout = 0;
                h->state = I2C_IDLE;
                if (h->callback)
                    h->callback(-1);
            }
            break;
        }
        h->timeout = 0;
        if (!hal_i2c_get_ack())
        {
            hal_i2c_stop();
            h->state = I2C_IDLE;
            if (h->callback)
                h->callback(-1);
            break;
        }
        h->index++;
        h->state = (h->index == h->len) ? I2C_STOP : I2C_WRITE;
        break;

    case I2C_READ:
        hal_i2c_request_byte();
        h->state = I2C_WAIT_RX;
        break;

    case I2C_WAIT_RX:
        if (!hal_i2c_rx_ready())
        {
            if (++h->timeout > I2C_TIMEOUT_MAX)
            {
                hal_i2c_stop();
                h->timeout = 0;
                h->state = I2C_IDLE;
                if (h->callback)
                    h->callback(-1);
            }
            break;
        }
        h->timeout = 0;
        h->buf[h->index] = hal_i2c_read_byte();
        h->index++;
        if (h->index == h->len)
        {
            hal_i2c_send_nack();
            h->state = I2C_STOP;
        }
        else
        {
            hal_i2c_send_ack();
            h->state = I2C_READ;
        }
        break;

    case I2C_STOP:
        hal_i2c_stop();
        if (h->callback)
            h->callback(0);

        h->state = I2C_IDLE;
        break;

    case I2C_IDLE:
        break;
    }
}
