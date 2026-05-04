/**
 * @file cubesat_multi_slave.c
 * @brief CubeSat I2C Multi-Slave Simulator usando PIO (otimizado)
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "i2c_multi.h"

#define ADDR_GNSS        0x42
#define ADDR_IMU         0x68
#define ADDR_PRESSURE    0x77
#define ADDR_TEMPERATURE 0x48
#define ADDR_EPS         0x60

#define SDA_PIN  12
#define LED_PIN  25

static uint8_t current_addr = 0x00;
static uint8_t current_reg  = 0xFF;
static bool    reg_received = false;

/* Buffers PRÉ-CALCULADOS — um por sensor, nunca recalculados */
static uint8_t buf_gnss[18];
static uint8_t buf_imu_accel[6];
static uint8_t buf_imu_default[1];
static uint8_t buf_pressure[6];
static uint8_t buf_pressure_default[2];
static uint8_t buf_temperature[2];
static uint8_t buf_eps_voltage[2];
static uint8_t buf_eps_default[2];

/* Buffer ativo que o PIO usa para enviar */
static uint8_t *active_buf = NULL;

static void init_sensor_data(void)
{
    /* GNSS — big-endian floats */
    union { float f; uint8_t b[4]; } lat = {.f = 38.736946f};
    union { float f; uint8_t b[4]; } lon = {.f = -9.142685f};
    union { float f; uint8_t b[4]; } alt = {.f = 550.0f};
    union { float f; uint8_t b[4]; } spd = {.f = 7.8f};

    buf_gnss[0]  = lat.b[3]; buf_gnss[1]  = lat.b[2];
    buf_gnss[2]  = lat.b[1]; buf_gnss[3]  = lat.b[0];
    buf_gnss[4]  = lon.b[3]; buf_gnss[5]  = lon.b[2];
    buf_gnss[6]  = lon.b[1]; buf_gnss[7]  = lon.b[0];
    buf_gnss[8]  = alt.b[3]; buf_gnss[9]  = alt.b[2];
    buf_gnss[10] = alt.b[1]; buf_gnss[11] = alt.b[0];
    buf_gnss[12] = spd.b[3]; buf_gnss[13] = spd.b[2];
    buf_gnss[14] = spd.b[1]; buf_gnss[15] = spd.b[0];
    buf_gnss[16] = 0x01;
    buf_gnss[17] = 0x08;

    /* IMU accel */
    buf_imu_accel[0] = 0x04; buf_imu_accel[1] = 0x00;
    buf_imu_accel[2] = 0x00; buf_imu_accel[3] = 0x00;
    buf_imu_accel[4] = 0x40; buf_imu_accel[5] = 0x00;
    buf_imu_default[0] = 0x00;

    /* Pressure */
    buf_pressure[0] = 0x65; buf_pressure[1] = 0x5A;
    buf_pressure[2] = 0x00; buf_pressure[3] = 0x88;
    buf_pressure[4] = 0x7A; buf_pressure[5] = 0x00;
    buf_pressure_default[0] = 0x00; buf_pressure_default[1] = 0x00;

    /* Temperature — 25°C */
    buf_temperature[0] = 0x19; buf_temperature[1] = 0x00;

    /* EPS — 7400mV */
    buf_eps_voltage[0] = 0xE8; buf_eps_voltage[1] = 0x1C;
    buf_eps_default[0] = 0x00; buf_eps_default[1] = 0x00;
}

static int16_t get_buffer_length(uint8_t addr, uint8_t reg)
{
    switch (addr)
    {
    case ADDR_GNSS:        return 18;
    case ADDR_IMU:         return (reg == 0x3B) ? 6 : 1;
    case ADDR_PRESSURE:    return (reg == 0xF7) ? 6 : 2;
    case ADDR_TEMPERATURE: return 2;
    case ADDR_EPS:         return 2;
    default:               return 2;
    }
}

/* Retorna ponteiro para o buffer correto — ZERO alocações, ZERO cópias */
static uint8_t *get_buffer(uint8_t addr, uint8_t reg)
{
    switch (addr)
    {
    case ADDR_GNSS:
        return buf_gnss;

    case ADDR_IMU:
        if (reg == 0x3B) return buf_imu_accel;
        return buf_imu_default;

    case ADDR_PRESSURE:
        if (reg == 0xF7) return buf_pressure;
        return buf_pressure_default;

    case ADDR_TEMPERATURE:
        return buf_temperature;

    case ADDR_EPS:
        if (reg == 0x09) return buf_eps_voltage;
        return buf_eps_default;

    default:
        return buf_eps_default;
    }
}

/* Handlers — mínimo de processamento possível */

static void receive_handler(uint8_t data, bool is_address)
{
    if (is_address)
    {
        current_addr = data;
    }
    else
    {
        current_reg = data;
        reg_received = true;
        active_buf = get_buffer(current_addr, current_reg);
        i2c_multi_set_write_buffer(active_buf);
        i2c_multi_fixed_length(get_buffer_length(current_addr, current_reg));
    }
}

static void request_handler(uint8_t address)
{
    current_addr = address;
    if (!reg_received)
        current_reg = 0xFF;

    active_buf = get_buffer(current_addr, current_reg);
    i2c_multi_set_write_buffer(active_buf);
    i2c_multi_fixed_length(get_buffer_length(current_addr, current_reg));
}

static void stop_handler(uint8_t length)
{
    (void)length;
}

int main(void)
{
    stdio_init_all();
    sleep_ms(2000);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    /* Pré-calcula todos os dados uma vez */
    init_sensor_data();

    /* Inicializa PIO I2C */
    i2c_multi_init(pio0, SDA_PIN);
    i2c_multi_set_receive_handler(receive_handler);
    i2c_multi_set_request_handler(request_handler);
    i2c_multi_set_stop_handler(stop_handler);
    i2c_multi_set_write_buffer(buf_gnss);  /* default */

    i2c_multi_enable_address(ADDR_GNSS);
    i2c_multi_enable_address(ADDR_IMU);
    i2c_multi_enable_address(ADDR_PRESSURE);
    i2c_multi_enable_address(ADDR_TEMPERATURE);
    i2c_multi_enable_address(ADDR_EPS);

    printf("[OK] 5 slaves (optimized)\n");

    for (int i = 0; i < 3; i++)
    {
        gpio_put(LED_PIN, 1); sleep_ms(100);
        gpio_put(LED_PIN, 0); sleep_ms(100);
    }

    while (true)
    {
        tight_loop_contents();
    }

    return 0;
}