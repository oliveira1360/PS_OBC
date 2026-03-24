#include "peripherals/gnss.h"
#include "drivers/i2c_driver.h"

#define GNSS_ADDR 0x42
#define GNSS_BUF_LEN 8

static i2c_handle_t i2c = {0};

/* HAL devolve: { lat_int, lat_dec, lon_int, lon_dec, alt_hi, alt_lo, speed_int, speed_dec } */
static void gnss_parse(gnss_data_t *out, uint8_t *buf) {
    out->latitude   = (float)buf[0] + (float)buf[1] / 100.0f;
    out->longitude  = (float)buf[2] + (float)buf[3] / 100.0f;
    out->altitude   = (float)((buf[4] << 8) | buf[5]);
    out->speed      = (float)buf[6] + (float)buf[7] / 100.0f;
    out->fix        = 2;  /* simulado: fix 3D */
    out->satellites = 8;  /* simulado         */
}

int gnss_read(gnss_data_t *out) {
    static uint8_t buf[GNSS_BUF_LEN];

    if (i2c_read(&i2c, GNSS_ADDR, buf, GNSS_BUF_LEN) != 0)
        return -1;

    gnss_parse(out, buf);
    return 0;
}