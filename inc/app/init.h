#ifndef INIT_H
#define INIT_H
#include <stdint.h>

#define MAX_INIT_RETRIES 0x05

typedef struct
{
    uint8_t gpio;
    uint8_t i2c;
    uint8_t spi;
    uint8_t qspi;
    uint8_t usart;
    uint8_t gnss;
    uint8_t imu;
    uint8_t ttc;
    uint8_t pressure;
    uint8_t temperature;
    uint8_t ext_memory;
} init_status_t; // vai guardar todos os dados de sucesso/falha

extern init_status_t init_status;
int system_init(void);


#endif 
