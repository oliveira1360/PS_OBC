#include <stdio.h>
#include "peripherals/pressure.h"
#include "drivers/i2c_driver.h"
#include "config/board.h"
#include "app/sensors.h"

static i2c_handle_t i2c = {0};
static uint8_t buf[PRES_BUF_LEN];

static void pressure_parse(uint8_t *buf)
{
    float val = (float)((buf[0] << 8) | buf[1]);
    pressure.pressure = val;
}

static void on_pressure_done(int result)
{

    if (result == 0)
        pressure_parse(buf);
}

void pressure_read_async()
{
        if (i2c.state != I2C_IDLE)
        return;  // já está a ler 

    i2c.addr     = PRESS_ADDR;
    i2c.buf      = buf;
    i2c.len      = PRES_BUF_LEN;
    i2c.rw       = 1;
    i2c.index    = 0;
    i2c.callback = on_pressure_done;
    i2c.state    = I2C_STARTING;
}

void pressure_tick(void)
{
    i2c_tick(&i2c);
}