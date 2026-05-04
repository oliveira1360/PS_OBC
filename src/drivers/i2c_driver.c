#include "drivers/i2c_driver.h"
#include "hal/hal_i2c.h"
#include <stdio.h>

static uint8_t bus_locked = 0U;

static void i2c_fail(i2c_handle_t *h)
{
    hal_i2c_stop();
    h->state = I2C_IDLE;
    h->timeout = 0;
    h->error_count++;

    /* Se falhar várias vezes seguidas, o bus pode estar preso */
    if (h->error_count >= 3)
    {
        printf("bus recovery for addr=0x%02X\n", h->addr);
        hal_i2c_bus_recovery();
        h->error_count = 0;
    }

    bus_locked = 0U;
    if (h->callback)
        h->callback(-1);
}

static void i2c_success(i2c_handle_t *h)
{
    hal_i2c_stop();
    h->state = I2C_IDLE;
    h->error_count = 0; /* reset no sucesso */
    bus_locked = 0U;
    if (h->callback)
        h->callback(0);
}

static void handle_starting(i2c_handle_t *h)
{
    if (!hal_i2c_bus_free() || bus_locked)
        return;

    bus_locked = 1U;
    hal_i2c_start();

    if (h->use_reg)
        hal_i2c_send_addr((h->addr << 1) | 0);
    else
        hal_i2c_send_addr((h->addr << 1) | (h->rw ? 1 : 0));

    h->state = I2C_SELECT_MODE;
}

static void handle_select_mode(i2c_handle_t *h)
{
    if (!hal_i2c_get_ack())
    {
        i2c_fail(h);
        return;
    }

    h->index = 0;

    if (h->use_reg)
    {
        hal_i2c_send_byte(h->reg);
        h->state = I2C_RESTART;
    }
    else
    {
        h->state = h->rw ? I2C_READ : I2C_WRITE;
    }
}

static void handle_restart(i2c_handle_t *h)
{
    if (!hal_i2c_tx_ready())
    {
        if (++h->timeout > I2C_TIMEOUT_MAX)
            i2c_fail(h);
        return;
    }
    h->timeout = 0;

    if (!hal_i2c_get_ack())
    {
        i2c_fail(h);
        return;
    }

    hal_i2c_restart_read(h->addr);
    h->use_reg = 0;
    h->state = I2C_WAIT_RESTART;
}

static void handle_wait_restart(i2c_handle_t *h)
{
    if (!hal_i2c_get_ack())
    {
        i2c_fail(h);
        return;
    }
    h->index = 0;
    h->state = I2C_READ;
}

static void handle_write(i2c_handle_t *h)
{
    hal_i2c_send_byte(h->buf[h->index]);
    h->state = I2C_WAIT_TX;
}

static void handle_wait_tx(i2c_handle_t *h)
{
    if (!hal_i2c_tx_ready())
    {
        if (++h->timeout > I2C_TIMEOUT_MAX)
            i2c_fail(h);
        return;
    }
    h->timeout = 0;

    if (!hal_i2c_get_ack())
    {
        i2c_fail(h);
        return;
    }

    h->index++;
    h->state = (h->index == h->len) ? I2C_STOP : I2C_WRITE;
}

static void handle_read(i2c_handle_t *h)
{
    hal_i2c_request_byte();
    h->state = I2C_WAIT_RX;
}

static void handle_wait_rx(i2c_handle_t *h)
{
    if (!hal_i2c_rx_ready())
    {
        if (++h->timeout > I2C_TIMEOUT_MAX)
            i2c_fail(h);
        return;
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
}

void i2c_tick(i2c_handle_t *h)
{
    switch (h->state)
    {
    case I2C_STARTING:
        handle_starting(h);
        break;
    case I2C_SELECT_MODE:
        handle_select_mode(h);
        break;
    case I2C_RESTART:
        handle_restart(h);
        break;
    case I2C_WAIT_RESTART:
        handle_wait_restart(h);
        break;
    case I2C_WRITE:
        handle_write(h);
        break;
    case I2C_WAIT_TX:
        handle_wait_tx(h);
        break;
    case I2C_READ:
        handle_read(h);
        break;
    case I2C_WAIT_RX:
        handle_wait_rx(h);
        break;
    case I2C_STOP:
        i2c_success(h);
        break;
    case I2C_IDLE:
        break;
    }
}