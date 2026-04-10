#include "peripherals/imu.h"
#include "drivers/i2c_driver.h"
#include "config/board.h"
#include "app/sensors.h"

static i2c_handle_t i2c = {0};
static uint8_t buf[IMU_BUF_LEN];

static void imu_parse(uint8_t *buf)
{
    imu.ax = (float)buf[0] / 100.0f;
    imu.ay = (float)buf[1] / 100.0f;
    imu.az = (float)buf[2] / 100.0f;

    imu.gx = (float)buf[3] / 100.0f;
    imu.gy = (float)buf[4] / 100.0f;
    imu.gz = (float)buf[5] / 100.0f;

    imu.mx = (float)buf[6] / 100.0f;
    imu.my = (float)buf[7] / 100.0f;
    imu.mz = (float)buf[8] / 100.0f;
}

static void on_imu_done(int result)
{
    if (result == 0)
        imu_parse(buf);
}

void imu_read_async()
{
    if (i2c.state != I2C_IDLE)
        return; // já está a ler

    i2c.addr = IMU_ADDR;
    i2c.buf = buf;
    i2c.len = IMU_BUF_LEN;
    i2c.rw = 1;
    i2c.index = 0;
    i2c.callback = on_imu_done;
    i2c.state = I2C_STARTING;
}

void imu_tick(void)
{
    i2c_tick(&i2c);
}