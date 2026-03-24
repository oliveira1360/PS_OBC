#ifndef HAL_I2C_H
#define HAL_I2C_H

#include <stdint.h>

void    hal_i2c_start(void);
void    hal_i2c_stop(void);
void    hal_i2c_send_byte(uint8_t byte);
void    hal_i2c_send_ack(void);
void    hal_i2c_send_nack(void);
uint8_t hal_i2c_read_byte(void);
int     hal_i2c_get_ack(void);

#endif