#ifndef I2C_DRIVER_H
#define I2C_DRIVER_H

#include <stdint.h>

typedef enum {
    I2C_IDLE,
    I2C_STARTING,
    I2C_ADDR,
    I2C_SELECT_MODE,
    I2C_WRITE,
    I2C_READ,
    I2C_ACK,
    I2C_NACK,
    I2C_STOP
} i2c_state_t;

typedef struct {
    i2c_state_t state;
    uint8_t     address;
    uint8_t    *buffer;
    uint8_t     len;
} i2c_handle_t;

int i2c_read (i2c_handle_t *h, uint8_t addr, uint8_t *buf, uint8_t len);
int i2c_write(i2c_handle_t *h, uint8_t addr, uint8_t *buf, uint8_t len);

#endif