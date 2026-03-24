#include "drivers/i2c_driver.h"
#include "hal/hal_i2c.h"

/* ---------- Write Mode ---------- */
static int i2c_write_mode(i2c_handle_t *h, uint8_t *buf, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        h->state = I2C_WRITE;
        hal_i2c_send_byte(buf[i]);

        h->state = I2C_ACK;
        if (!hal_i2c_get_ack()) {
            h->state = I2C_NACK;
            return -1;          /* Fail: Send NACK */
        }
    }
    return 0;
}

/* ---------- Read Mode ---------- */
static int i2c_read_mode(i2c_handle_t *h, uint8_t *buf, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        h->state = I2C_READ;
        buf[i] = hal_i2c_read_byte();

        uint8_t last = (i == len - 1);

        if (last) {
            h->state = I2C_NACK;
            hal_i2c_send_nack();    /* Byte == last: Send NACK */
        } else {
            h->state = I2C_ACK;
            hal_i2c_send_ack();     /* Byte != last: Send ACK  */
        }
    }
    return 0;
}

/* ---------- Public API ---------- */
int i2c_write(i2c_handle_t *h, uint8_t addr, uint8_t *buf, uint8_t len) {
    /* Idle → Starting */
    h->state = I2C_STARTING;
    hal_i2c_start();
    hal_i2c_send_byte((addr << 1) | 0x00);  /* R/W = 0 */

    /* Starting → Select Mode → Write */
    h->state = I2C_SELECT_MODE;
    if (!hal_i2c_get_ack()) {
        h->state = I2C_IDLE;
        return -1;
    }

    int result = i2c_write_mode(h, buf, len);

    /* → Stop → Idle */
    h->state = I2C_STOP;
    hal_i2c_stop();
    h->state = I2C_IDLE;
    return result;
}

int i2c_read(i2c_handle_t *h, uint8_t addr, uint8_t *buf, uint8_t len) {
    /* Idle → Starting */
    h->state = I2C_STARTING;
    hal_i2c_start();
    hal_i2c_send_byte((addr << 1) | 0x01);  /* R/W = 1 */

    /* Starting → Select Mode → Read */
    h->state = I2C_SELECT_MODE;
    if (!hal_i2c_get_ack()) {
        h->state = I2C_IDLE;
        return -1;
    }

    int result = i2c_read_mode(h, buf, len);

    /* → Stop → Idle */
    h->state = I2C_STOP;
    hal_i2c_stop();
    h->state = I2C_IDLE;
    return result;
}