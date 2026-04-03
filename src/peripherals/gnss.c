#include "peripherals/gnss.h"
#include "drivers/i2c_driver.h"
#include "config/board.h"
#include "app/sensors.h"

static i2c_handle_t i2c = {0};
static uint8_t buf[GNSS_BUF_LEN];

static void gnss_parse(uint8_t *buf)
{
    gnss.latitude = (float)buf[0] + (float)buf[1] / 100.0f;
    gnss.longitude = (float)buf[2] + (float)buf[3] / 100.0f;
    gnss.altitude = (float)((buf[4] << 8) | buf[5]);
    gnss.speed = (float)buf[6] + (float)buf[7] / 100.0f;
}

static void on_gnss_done(int result)
{
    if (result == 0)
        gnss_parse(buf);
}

void gnss_read_async()
{
    i2c_enqueue(GNSS_ADDR, buf, GNSS_BUF_LEN, 1, on_gnss_done);
}

void gnss_tick(void)
{
    i2c_tick(&i2c);
}