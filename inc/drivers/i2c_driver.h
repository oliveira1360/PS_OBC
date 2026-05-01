#ifndef I2C_DRIVER_H
#define I2C_DRIVER_H

#include <stdint.h>

#define I2C_QUEUE_SIZE 25
#define I2C_TIMEOUT_MAX 10

typedef enum {
    I2C_IDLE,
    I2C_STARTING,
    I2C_SELECT_MODE,
    I2C_WRITE,
    I2C_WAIT_TX,
    I2C_RESTART,
    I2C_WAIT_RESTART,
    I2C_READ,
    I2C_WAIT_RX,
    I2C_STOP
} i2c_state_t;

typedef struct
{
    i2c_state_t state;
    uint8_t addr;
    uint8_t *buf;
    uint8_t len;
    uint8_t index;
    uint8_t rw;
    uint8_t reg;        /* registo a enviar antes da leitura */
    uint8_t use_reg;    /* 1 = write reg + restart + read    */
    void (*callback)(int);
    uint16_t timeout;
} i2c_handle_t;

typedef struct
{
    uint8_t addr;
    uint8_t *buf;
    uint8_t len;
    uint8_t rw;
    void (*callback)(int);
} i2c_request_t;

typedef struct {
    i2c_request_t requests[I2C_QUEUE_SIZE];
    uint8_t       head;
    uint8_t       tail;
    uint8_t       count;
} i2c_queue_t;

void i2c_tick(i2c_handle_t *h);
int i2c_read(i2c_handle_t *h, uint8_t addr, uint8_t *buf, uint8_t len);
int i2c_write(i2c_handle_t *h, uint8_t addr, uint8_t *buf, uint8_t len);

#endif