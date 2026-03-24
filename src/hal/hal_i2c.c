#include "hal/hal_i2c.h"

typedef struct {
    uint8_t  addr;
    uint8_t  data[8];
    uint8_t  len;
} i2c_device_t;

static const i2c_device_t devices[] = {
    { 0x42, { 0x15, 0x3F, 0x00, 0x12, 0x00, 0x03, 0x01, 0x02 }, 8 }, /* GNSS        */
    { 0x68, { 0x01, 0xA0, 0x00, 0xFF, 0x10, 0x00, 0x00, 0x00 }, 8 }, /* IMU         */
    { 0x77, { 0x65, 0x00 },                                      2 }, /* Pressão     */
    { 0x48, { 0x19, 0x00 },                                      2 }, /* Temperatura */
    { 0x62, { 0xAA, 0x01 },                                      2 }, /* EPS         */
};

#define NUM_DEVICES (sizeof(devices) / sizeof(devices[0]))

static uint8_t  current_addr  = 0x00;
static uint8_t  byte_index    = 0;
static uint8_t  rw_bit        = 0;

void hal_i2c_start(void) {
    byte_index = 0;
}

void hal_i2c_stop(void) {
    current_addr = 0x00;
    byte_index   = 0;
}

void hal_i2c_send_byte(uint8_t byte) {
    current_addr = byte >> 1;
    rw_bit       = byte & 0x01;
    byte_index   = 0;
}

int hal_i2c_get_ack(void) {
    for (uint8_t i = 0; i < NUM_DEVICES; i++) {
        if (devices[i].addr == current_addr) return 1;
    }
    return 0; 
}

uint8_t hal_i2c_read_byte(void) {
    for (uint8_t i = 0; i < NUM_DEVICES; i++) {
        if (devices[i].addr == current_addr) {
            if (byte_index < devices[i].len) {
                return devices[i].data[byte_index++];
            }
            return 0x00;
        }
    }
    return 0xFF;  
}

void hal_i2c_send_ack(void) {

}

void hal_i2c_send_nack(void) {

}