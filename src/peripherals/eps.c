#include "peripherals/eps.h"
#include "drivers/i2c_driver.h"
#include "config/board.h"
#include "app/sensors.h"

static i2c_handle_t i2c = {0};
static uint8_t buf[EPS_BUF_LEN];

static void eps_parse(uint8_t *buf)
{
    eps.voltage = (float)buf[0] / 10.0f;  // 0xAA = 170 → 17.0V
    eps.current = (float)buf[1] / 100.0f; // 0x01 = 1   → 0.01A
}

static void on_eps_done(int result)
{
    if (result == 0)
        eps_parse(buf);
}

void eps_read_async()
{
    i2c_enqueue(EPS_ADDR, buf, EPS_BUF_LEN, 1, on_eps_done);
}

void eps_tick(void)
{
    i2c_tick(&i2c);
}