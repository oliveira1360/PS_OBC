/**
 * @file cubesat_multi_slave.c
 * @brief CubeSat I2C + UART + SPI Simulator — Multicore
 *
 * Usa os dois cores do RP2040:
 *   Core 0: I2C PIO (sensores) + UART (Ground Station / OTA)
 *   Core 1: SPI slave (propulsor cold-gas)
 *
 * Isto garante que o tráfego SPI não bloqueia o PIO I2C.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/uart.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "i2c_multi.h"

/* ========== I2C ========== */
#define ADDR_GNSS        0x42
#define ADDR_IMU         0x68
#define ADDR_PRESSURE    0x77
#define ADDR_TEMPERATURE 0x48
#define ADDR_EPS         0x60

#define SDA_PIN 12
#define LED_PIN 25

/* ========== UART (Ground Station) ========== */
#define TTC_UART     uart1
#define TTC_BAUDRATE 9600
#define TTC_TX_PIN   8
#define TTC_RX_PIN   9
#define TTC_CMD_LEN  4
#define DATA_BITS    8
#define STOP_BITS    1
#define PARITY       UART_PARITY_NONE

/* ========== SPI Slave (Propulsor) — Core 1 ========== */
#define PROP_SPI       spi0
#define PROP_SPI_FREQ  100000  
#define PROP_SCK_PIN   2
#define PROP_MOSI_PIN  4    /* RX — recebe do master */
#define PROP_MISO_PIN  3    /* TX — envia ao master  */
#define PROP_CS_PIN    5

#define PROP_FRAME_LEN       9
#define PROP_CMD_READ        0x01
#define PROP_CMD_VALVE_OPEN  0x02
#define PROP_CMD_VALVE_CLOSE 0x03
#define PROP_CMD_SET_THRUST  0x04

/* ========== OTA Simulation ========== */
#define OTA_PACKET_SIZE   128
#define OTA_TOTAL_PACKETS 32
#define OTA_HEADER_SIZE   6
#define OTA_SYNC_WORD     0xAA55
#define OTA_ACK_BYTE      0xAC

/* ========== Timing (ms) ========== */
#define TTC_INTERVAL_MS        1000
#define OTA_PACKET_INTERVAL_MS 500
#define OTA_ACK_WAIT_MS        200
#define OTA_ACK_POLL_MS        10

/* ========== OTA state (Core 0) ========== */
static bool     ota_active      = false;
static uint16_t ota_seq         = 0;
static uint16_t ota_total       = OTA_TOTAL_PACKETS;
static uint8_t  ota_packet_buf[OTA_HEADER_SIZE + OTA_PACKET_SIZE];

/* ========== I2C state (Core 0) ========== */
static uint8_t  current_addr  = 0x00;
static uint8_t  current_reg   = 0xFF;
static bool     reg_received  = false;

/* ========== I2C buffers (Core 0) ========== */
static uint8_t buf_gnss[18];
static uint8_t buf_imu_accel[6];
static uint8_t buf_imu_default[1];
static uint8_t buf_pressure[6];
static uint8_t buf_pressure_default[2];
static uint8_t buf_temperature[2];
static uint8_t buf_eps_voltage[2];
static uint8_t buf_eps_default[2];
static uint8_t *active_buf = NULL;

/* ========== UART state (Core 0) ========== */
static uint16_t tlm_timestamp = 0;
static uint8_t  uart_rx_buf[16];

/* ==========================================================================
 * SENSOR DATA INIT (Core 0)
 * ========================================================================== */

static void init_sensor_data(void)
{
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
    buf_gnss[16] = 0x01;     buf_gnss[17] = 0x08;

    buf_imu_accel[0] = 0x04; buf_imu_accel[1] = 0x00;
    buf_imu_accel[2] = 0x00; buf_imu_accel[3] = 0x00;
    buf_imu_accel[4] = 0x40; buf_imu_accel[5] = 0x00;
    buf_imu_default[0] = 0x00;

    buf_pressure[0] = 0x65; buf_pressure[1] = 0x5A;
    buf_pressure[2] = 0x00; buf_pressure[3] = 0x88;
    buf_pressure[4] = 0x7A; buf_pressure[5] = 0x00;
    buf_pressure_default[0] = 0x00; buf_pressure_default[1] = 0x00;

    buf_temperature[0] = 0x19; buf_temperature[1] = 0x00;

    buf_eps_voltage[0] = 0xE8; buf_eps_voltage[1] = 0x1C;
    buf_eps_default[0] = 0x00; buf_eps_default[1] = 0x00;
}

/* ==========================================================================
 * I2C HELPERS & HANDLERS (Core 0)
 * ========================================================================== */

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

static uint8_t *get_buffer(uint8_t addr, uint8_t reg)
{
    switch (addr)
    {
    case ADDR_GNSS:        return buf_gnss;
    case ADDR_IMU:         return (reg == 0x3B) ? buf_imu_accel : buf_imu_default;
    case ADDR_PRESSURE:    return (reg == 0xF7) ? buf_pressure : buf_pressure_default;
    case ADDR_TEMPERATURE: return buf_temperature;
    case ADDR_EPS:         return (reg == 0x09) ? buf_eps_voltage : buf_eps_default;
    default:               return buf_eps_default;
    }
}

static void receive_handler(uint8_t data, bool is_address)
{
    if (is_address)
        current_addr = data;
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
    if (!reg_received) current_reg = 0xFF;
    active_buf = get_buffer(current_addr, current_reg);
    i2c_multi_set_write_buffer(active_buf);
    i2c_multi_fixed_length(get_buffer_length(current_addr, current_reg));
}

static void stop_handler(uint8_t length) { (void)length; }

/* ==========================================================================
 * CORE 1 — SPI Slave (Propulsor)
 *
 * Corre completamente isolado do Core 0.
 * ========================================================================== */

static volatile uint8_t  prop_valve_state     = 0U;
static volatile uint16_t prop_thrust_setpoint = 0U;
static uint8_t  prop_tx_frame[PROP_FRAME_LEN];

static void prop_generate_frame(void)
{
    uint8_t *f = prop_tx_frame;

    f[0] = prop_valve_state ? 0x01U : 0x00U;

    uint16_t press;
    if (prop_valve_state)
        press = 300U + (uint16_t)(rand() % 201U);
    else
        press = 150U + (uint16_t)(rand() % 101U);
    f[1] = (uint8_t)(press >> 8U);
    f[2] = (uint8_t)(press & 0xFFU);

    uint16_t temp;
    if (prop_valve_state)
        temp = 2000U + (uint16_t)(rand() % 2001U);
    else
        temp = 220U + (uint16_t)(rand() % 61U);
    f[3] = (uint8_t)(temp >> 8U);
    f[4] = (uint8_t)(temp & 0xFFU);

    uint16_t thrust;
    if (prop_valve_state && prop_thrust_setpoint > 0U)
    {
        int16_t variation = (int16_t)(rand() % 11) - 5;
        int16_t val = (int16_t)prop_thrust_setpoint + variation;
        thrust = (val > 0) ? (uint16_t)val : 0U;
    }
    else
    {
        thrust = 0U;
    }
    f[5] = (uint8_t)(thrust >> 8U);
    f[6] = (uint8_t)(thrust & 0xFFU);

    f[7] = prop_valve_state;

    uint8_t chk = 0U;
    for (uint8_t j = 0U; j < 8U; j++)
        chk ^= f[j];
    f[8] = chk;
}

static void prop_process_command(uint8_t cmd, const uint8_t *extra, uint8_t extra_len)
{
    switch (cmd)
    {
    case PROP_CMD_READ:
        break;
    case PROP_CMD_VALVE_OPEN:
        prop_valve_state = 1U;
        if (prop_thrust_setpoint == 0U)
            prop_thrust_setpoint = 100U;
        break;
    case PROP_CMD_VALVE_CLOSE:
        prop_valve_state = 0U;
        prop_thrust_setpoint = 0U;
        break;
    case PROP_CMD_SET_THRUST:
        if (extra_len >= 2U)
            prop_thrust_setpoint = ((uint16_t)extra[0] << 8) | extra[1];
        break;
    default:
        break;
    }
}


/* ==========================================================================
 * UART helpers (Core 0)
 * ========================================================================== */

static void uart_flush_rx(void)
{
    while (uart_is_readable(TTC_UART))
        (void)uart_getc(TTC_UART);
}

static void ttc_check_response(void)
{
    uint8_t idx = 0;
    while (uart_is_readable(TTC_UART) && idx < sizeof(uart_rx_buf))
        uart_rx_buf[idx++] = uart_getc(TTC_UART);

    if (idx > 0)
    {
        printf("[TTC RX] %d bytes:", idx);
        for (uint8_t i = 0; i < idx; i++)
            printf(" %02X", uart_rx_buf[i]);
        printf("\n");
    }
}

static bool ttc_wait_for_ack(uint32_t timeout_ms)
{
    absolute_time_t deadline = make_timeout_time_ms(timeout_ms);
    uint8_t ack_buf[TTC_CMD_LEN];
    uint8_t ack_idx = 0;

    while (absolute_time_diff_us(deadline, get_absolute_time()) < 0)
    {
        if (uart_is_readable(TTC_UART))
        {
            ack_buf[ack_idx++] = uart_getc(TTC_UART);
            if (ack_idx >= 2 && ack_buf[0] == 0x10 && ack_buf[1] == OTA_ACK_BYTE)
            {
                while (ack_idx < TTC_CMD_LEN && uart_is_readable(TTC_UART))
                    ack_buf[ack_idx++] = uart_getc(TTC_UART);
                printf("[OTA] ACK received\n");
                return true;
            }
            if (ack_idx >= TTC_CMD_LEN)
                ack_idx = 0;
        }
        sleep_ms(OTA_ACK_POLL_MS);
    }
    return false;
}

static void ttc_send_command(void)
{
    uint8_t cmd[TTC_CMD_LEN];
    uint32_t roll = rand() % 100;

    if (roll < 85)
    {
        tlm_timestamp++;
        cmd[0] = 0x20;
        cmd[1] = (uint8_t)(5 + rand() % 31);
        cmd[2] = (uint8_t)(tlm_timestamp >> 8);
        cmd[3] = (uint8_t)(tlm_timestamp & 0xFF);
        uart_write_blocking(TTC_UART, cmd, TTC_CMD_LEN);
    }
    else if (roll < 93)
    {
        cmd[0] = 0x01; cmd[1] = 0; cmd[2] = 0; cmd[3] = 0;
        uart_write_blocking(TTC_UART, cmd, TTC_CMD_LEN);
    }
    else if (roll < 97)
    {
        cmd[0] = 0x10; cmd[1] = 0x01;
        cmd[2] = (uint8_t)(rand() % 10); cmd[3] = 0x00;
        printf("[OTA] === Starting OTA handshake ===\n");
        uart_flush_rx();
        uart_write_blocking(TTC_UART, cmd, TTC_CMD_LEN);
        if (ttc_wait_for_ack(OTA_ACK_WAIT_MS))
        {
            printf("[OTA] Handshake OK\n");
            uart_flush_rx();
            sleep_ms(500);
            ota_active = true;
            ota_seq = 0;
        }
        else
        {
            printf("[OTA] ACK timeout\n");
            uart_flush_rx();
        }
    }
    else if (roll < 99)
    {
        cmd[0] = 0x02; cmd[1] = 0; cmd[2] = 0; cmd[3] = 0;
        uart_write_blocking(TTC_UART, cmd, TTC_CMD_LEN);
    }
    else
    {
        cmd[0] = 0x11; cmd[1] = 0; cmd[2] = 0; cmd[3] = 0;
        uart_write_blocking(TTC_UART, cmd, TTC_CMD_LEN);
    }
}

static void ota_send_packet(void)
{
    if (ota_seq >= ota_total)
    {
        ota_packet_buf[0] = (uint8_t)(OTA_SYNC_WORD >> 8);
        ota_packet_buf[1] = (uint8_t)(OTA_SYNC_WORD & 0xFF);
        ota_packet_buf[2] = 0xFF; ota_packet_buf[3] = 0xFF;
        ota_packet_buf[4] = 0x00; ota_packet_buf[5] = 0x00;
        memset(&ota_packet_buf[OTA_HEADER_SIZE], 0, OTA_PACKET_SIZE);
        uart_write_blocking(TTC_UART, ota_packet_buf, OTA_HEADER_SIZE + OTA_PACKET_SIZE);
        printf("[OTA] Sent END marker\n");
        ota_active = false;
        ota_seq = 0;
        return;
    }

    ota_packet_buf[0] = (uint8_t)(OTA_SYNC_WORD >> 8);
    ota_packet_buf[1] = (uint8_t)(OTA_SYNC_WORD & 0xFF);
    ota_packet_buf[2] = (uint8_t)(ota_seq >> 8);
    ota_packet_buf[3] = (uint8_t)(ota_seq & 0xFF);
    ota_packet_buf[4] = (uint8_t)(OTA_PACKET_SIZE >> 8);
    ota_packet_buf[5] = (uint8_t)(OTA_PACKET_SIZE & 0xFF);
    for (uint16_t i = 0; i < OTA_PACKET_SIZE; i++)
        ota_packet_buf[OTA_HEADER_SIZE + i] = (uint8_t)(rand() & 0xFF);

    uart_write_blocking(TTC_UART, ota_packet_buf, OTA_HEADER_SIZE + OTA_PACKET_SIZE);
    printf("[OTA] Sent packet %d/%d\n", ota_seq + 1, ota_total);
    ota_seq++;
}

static void core1_spi_entry(void)
{
    spi_init(PROP_SPI, PROP_SPI_FREQ);
    spi_set_slave(PROP_SPI, true);
    spi_set_format(PROP_SPI, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(PROP_MISO_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PROP_MOSI_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PROP_SCK_PIN,  GPIO_FUNC_SPI);
    gpio_set_function(PROP_CS_PIN,   GPIO_FUNC_SPI);

    uint8_t rx_buf[1 + PROP_FRAME_LEN];

    /* Gera e pré-carrega o PRIMEIRO frame no TX FIFO */
    prop_generate_frame();
    spi_get_hw(PROP_SPI)->dr = 0x00U;  /* dummy durante cmd */
    for (uint8_t i = 0; i < PROP_FRAME_LEN; i++)
        spi_get_hw(PROP_SPI)->dr = prop_tx_frame[i];

    while (true)
    {
        /* Espera bloqueante — lê os bytes que o master enviou.
         * Enquanto lê, o hardware SPI envia automaticamente
         * os bytes que estão no TX FIFO (pré-carregados). */
        spi_read_blocking(PROP_SPI, 0x00, rx_buf, 1 + PROP_FRAME_LEN);

        /* Processa o comando recebido */
        uint8_t cmd = rx_buf[0];
        prop_process_command(cmd, &rx_buf[1], PROP_FRAME_LEN);

        /* Gera novo frame e pré-carrega para a PRÓXIMA transação */
        prop_generate_frame();

        /* Limpa qualquer lixo do RX FIFO */
        while (spi_is_readable(PROP_SPI))
            (void)spi_get_hw(PROP_SPI)->dr;

        /* Carrega TX FIFO para a próxima transferência */
        spi_get_hw(PROP_SPI)->dr = 0x00U;
        for (uint8_t i = 0; i < PROP_FRAME_LEN; i++)
            spi_get_hw(PROP_SPI)->dr = prop_tx_frame[i];
    }
}


/* ==========================================================================
 * MAIN — Core 0: I2C PIO + UART
 * ========================================================================== */

int main(void)
{
    stdio_init_all();
    sleep_ms(2000);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    srand(time(NULL));

    /* ===== I2C PIO init ===== */
    init_sensor_data();
    i2c_multi_init(pio0, SDA_PIN);
    i2c_multi_set_receive_handler(receive_handler);
    i2c_multi_set_request_handler(request_handler);
    i2c_multi_set_stop_handler(stop_handler);
    i2c_multi_set_write_buffer(buf_gnss);

    i2c_multi_enable_address(ADDR_GNSS);
    i2c_multi_enable_address(ADDR_IMU);
    i2c_multi_enable_address(ADDR_PRESSURE);
    i2c_multi_enable_address(ADDR_TEMPERATURE);
    i2c_multi_enable_address(ADDR_EPS);

    /* ===== UART init ===== */
    uart_init(TTC_UART, TTC_BAUDRATE);
    gpio_set_function(TTC_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(TTC_RX_PIN, GPIO_FUNC_UART);
    uart_set_hw_flow(TTC_UART, false, false);
    uart_set_format(TTC_UART, DATA_BITS, STOP_BITS, PARITY);
    uart_set_fifo_enabled(TTC_UART, true);
    uart_flush_rx();

    /* SEM SPI, SEM MULTICORE */
    multicore_launch_core1(core1_spi_entry);

    for (int i = 0; i < 3; i++)
    {
        gpio_put(LED_PIN, 1); sleep_ms(100);
        gpio_put(LED_PIN, 0); sleep_ms(100);
    }

    absolute_time_t next_cmd = get_absolute_time();

    while (true)
    {
        ttc_check_response();

        if (absolute_time_diff_us(next_cmd, get_absolute_time()) > 0)
        {
            ttc_send_command();
            next_cmd = make_timeout_time_ms(TTC_INTERVAL_MS);
        }
    }

    return 0;
}