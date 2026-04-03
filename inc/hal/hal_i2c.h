#ifndef HAL_I2C_H
#define HAL_I2C_H

#include <stdint.h>

int hal_i2c_bus_free(void);
void    hal_i2c_start(void);
void    hal_i2c_stop(void);
void    hal_i2c_send_byte(uint8_t addr);
int     hal_i2c_get_ack(void);
uint8_t hal_i2c_read_byte(void);
void hal_i2c_send_addr(uint8_t byte);
void    hal_i2c_send_ack(void);
void    hal_i2c_send_nack(void);
void    hal_i2c_init(void);
int hal_i2c_tx_ready(void);
int hal_i2c_rx_ready(void);
void hal_i2c_request_byte(void);


#endif