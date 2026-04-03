#include <stdio.h>
#include <stdlib.h> // para simulacao apenas !!!
#include <time.h>   // para simulacao apenas !!!
#include "hal/hal_i2c.h"
#include "config/board.h"

#define NUM_DEVICES (sizeof(devices) / sizeof(devices[0]))

typedef struct
{
    uint8_t addr;
    uint8_t data[18];
    uint8_t len;
} i2c_device_t;

static i2c_device_t devices[] = {
    {GNSS_ADDR, {0x15, 0x3F, 0x00, 0x12, 0x00, 0x03, 0x01, 0x02}, GNSS_BUF_LEN}, /* GNSS        */
    {IMU_ADDR, {0x00, 0x64, 0x00, 0xC8, 0x00, 0x32,                              /* ax=1.00  ay=2.00  az=0.50  */
                0x00, 0x0A, 0x00, 0x05, 0x00, 0x0F,                              /* gx=0.10  gy=0.05  gz=0.15  */
                0x00, 0x01, 0x00, 0x02, 0x00, 0x03},                             /* mx=0.01  my=0.02  mz=0.03  */
     IMU_BUF_LEN},                                                               /* IMU         */
    {PRES_ADDR, {0x65, 0x00}, PRES_BUF_LEN},                                     /* Pressão     */
    {TEMP_ADDR, {0x19, 0x00}, TEMP_BUF_LEN},                                     /* Temperatura */
    {EPS_ADDR, {0xAA, 0x01}, EPS_BUF_LEN},                                       /* EPS         */
};

static uint8_t bus_free = 1;

static uint8_t active_addr = 0xFF;
static uint8_t read_index = 0;

int hal_i2c_bus_free(void)
{
    return bus_free;
}

void hal_i2c_init(void)
{
    srand((unsigned int)time(NULL));
}

static void hal_i2c_randomize(uint8_t i)
{
    switch (devices[i].addr)
    {

    case GNSS_ADDR:
        devices[i].data[0] = rand() % 90;  // latitude graus 0-90
        devices[i].data[1] = rand() % 100; // latitude decimais
        devices[i].data[2] = rand() % 180; // longitude graus
        devices[i].data[3] = rand() % 100;
        devices[i].data[4] = rand() % 4; // altitude high byte 0-1023m
        devices[i].data[5] = rand() % 256;
        devices[i].data[6] = rand() % 10; // speed
        devices[i].data[7] = rand() % 100;
        break;

    case IMU_ADDR:
        for (uint8_t j = 0; j < IMU_BUF_LEN; j++)
            devices[i].data[j] = rand() % 256;
        break;

    case PRES_ADDR:
    {
        uint16_t hpa = 900 + rand() % 200;
        devices[i].data[0] = (hpa >> 8) & 0xFF;
        devices[i].data[1] = hpa & 0xFF;
        break;
    }

    case TEMP_ADDR:
    {
        uint16_t temp = 0 + rand() % 80; // 0-80°C
        devices[i].data[0] = (temp >> 8) & 0xFF;
        devices[i].data[1] = temp & 0xFF;
        break;
    }

    case EPS_ADDR:
        devices[i].data[0] = 60 + rand() % 60; // voltage: 6.0-12.0V (/10)
        devices[i].data[1] = rand() % 200;     // current: 0-2.0A (/100)
        break;
    }
}

void hal_i2c_start(void)
{
    bus_free = 0;
    active_addr = 0xFF;
    read_index = 0;
}

void hal_i2c_stop(void)
{
    bus_free = 1;
}

uint8_t hal_i2c_read_byte(void)
{
    for (uint8_t i = 0; i < NUM_DEVICES; i++)
    {
        if (devices[i].addr == active_addr)
        {
            if (read_index < devices[i].len)
                return devices[i].data[read_index++];
            return 0x00;
        }
    }
    return 0xFF;
}

void hal_i2c_send_addr(uint8_t byte)
{
    uint8_t addr = byte >> 1;
    active_addr = addr;
    read_index = 0;

    for (uint8_t i = 0; i < NUM_DEVICES; i++)
    {
        if (devices[i].addr == addr)
        {
            hal_i2c_randomize(i);
            break;
        }
    }
}

void hal_i2c_send_byte(uint8_t byte)
{
    for (uint8_t i = 0; i < NUM_DEVICES; i++)
    {
        if (devices[i].addr == active_addr)
        {
            /* numa simulação de escrita podias guardar o byte aqui */
            (void)byte;
            break;
        }
    }
}

int hal_i2c_get_ack(void)
{
    for (uint8_t i = 0; i < NUM_DEVICES; i++)
    {
        if (devices[i].addr == active_addr)
            return 1;
    }
    return 0;
}

void hal_i2c_send_ack(void)
{
}

void hal_i2c_send_nack(void)
{
}

int hal_i2c_tx_ready(void)
{
    return 1; /* simulação: transmissão é instantânea */
}

int hal_i2c_rx_ready(void)
{
    return 1; /* simulação: dado sempre disponível */
}

void hal_i2c_request_byte(void)
{
    /* simulação: no hardware activaria o clock para receber */
}