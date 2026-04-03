#include "drivers/i2c_driver.h"
#include "hal/hal_i2c.h"

i2c_handle_t i2c_master = {0};
i2c_queue_t i2c_queue = {0};


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
            break;
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
            break;
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
        h->timeout = 0;
        break;

    case I2C_STOP:
        hal_i2c_stop();
        if (h->callback)
            h->callback(0);

        h->state = I2C_IDLE;
        break;

    case I2C_IDLE:
        if (i2c_queue.count > 0)
        {
            i2c_request_t *next = &i2c_queue.requests[i2c_queue.tail];
            h->addr = next->addr;
            h->buf = next->buf;
            h->len = next->len;
            h->rw = next->rw;
            h->callback = next->callback;
            h->index = 0;

            i2c_queue.tail = (i2c_queue.tail + 1) % I2C_QUEUE_SIZE;
            i2c_queue.count--;

            h->state = I2C_STARTING;
        }
        break;
    }
}

void i2c_enqueue(uint8_t addr, uint8_t *buf, uint8_t len, uint8_t rw, void (*cb)(int))
{
    if (i2c_queue.count >= I2C_QUEUE_SIZE)
        return;

    i2c_request_t *req = &i2c_queue.requests[i2c_queue.head];
    req->addr = addr;
    req->buf = buf;
    req->len = len;
    req->rw = rw;
    req->callback = cb;

    i2c_queue.head = (i2c_queue.head + 1) % I2C_QUEUE_SIZE;
    i2c_queue.count++;
}
