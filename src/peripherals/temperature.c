#include <stdio.h>
#include "peripherals/temperature.h"
#include "drivers/i2c_driver.h"
#include "config/board.h"
#include "app/sensors.h"

static i2c_handle_t i2c = {0};
static uint8_t buf[TEMP_BUF_LEN];

static void pressure_parse(uint8_t *buf)
{
    temperature.temperature = (float)((buf[0] << 8) | buf[1]);
}

static void on_temp_done(int result)
{
    if (result == 0)
        pressure_parse(buf);
}

void temperature_read_async()
{
    if (i2c.state != I2C_IDLE)
        return; // já está a ler

    i2c.addr = TEMP_ADDR;
    i2c.buf = buf;
    i2c.len = TEMP_BUF_LEN;
    i2c.rw = 1;
    i2c.index = 0;
    i2c.callback = on_temp_done;
    i2c.state = I2C_STARTING;
}

void temperature_tick(void)
{
    i2c_tick(&i2c);
}