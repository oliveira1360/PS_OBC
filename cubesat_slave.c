/**
 * @file cubesat_multi_slave.c
 * @brief CubeSat I2C + UART  — Dynamic LEO Sensor Simulation
 *
 * Sensor data updates every SENSOR_UPDATE_MS with realistic orbital dynamics:
 *   - GNSS: ground-track of a ~550km LEO orbit (ISS-like inclination 51.6°)
 *   - IMU:  micro-g residuals + attitude drift + occasional detumble spikes
 *   - Pressure: near-vacuum with sensor noise floor
 *   - Temperature: cyclic between sun/eclipse (-20°C to +45°C)
 *   - EPS: voltage/current following solar illumination cycle
 *
 * OTA forwarding:
 *   Dashboard → Pico (USB/COM5):
 *     "OTA_BEGIN" (9 B) | size (4 B BE) | crc32 (4 B BE) | version (4 B BE)
 *     | <size bytes of firmware>
 *   Pico → OBC (UART):
 *     CMD_START_OTA handshake, then OTA packets (128-byte payload each),
 *     then END marker with size/crc32/version in payload[0..11].
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/uart.h"
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

/* ========== OTA Simulation ========== */
#define OTA_PACKET_SIZE   128
#define OTA_HEADER_SIZE   6
#define OTA_SYNC_WORD     0xAA55
#define OTA_ACK_BYTE      0xAC

/* USB OTA reception — firmware buffer (32 KB).
 * Increase OTA_FW_BUF_SIZE if your firmware is larger (Pico has 264 KB SRAM). */
#define OTA_MAGIC_STR     "OTA_BEGIN"
#define OTA_MAGIC_LEN     9
#define OTA_FW_BUF_SIZE   (32U * 1024U)   /* 32 KB = 256 packets of 128 B */

/* ========== Timing (ms) ========== */
#define TTC_INTERVAL_MS        1000
#define SENSOR_UPDATE_MS       500
#define OTA_PACKET_INTERVAL_MS 500
#define OTA_ACK_WAIT_MS        2000
#define OTA_ACK_POLL_MS        10

/* ========== Orbital parameters ========== */
#define ORBIT_ALT_KM       550.0f
#define ORBIT_PERIOD_S     5760.0f
#define ORBIT_INCL_DEG     51.6f
#define ORBIT_SPEED_KMS    7.59f
#define ECLIPSE_FRACTION   0.35f
#define SIM_TIME_SCALE     60.0f

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

/* ========== OTA state ========== */
static bool     ota_active      = false;
static uint16_t ota_seq         = 0;
static uint16_t ota_total       = 0;
static uint8_t  ota_packet_buf[OTA_HEADER_SIZE + OTA_PACKET_SIZE];

/* USB-received firmware */
static uint8_t  ota_fw_buf[OTA_FW_BUF_SIZE];
static uint32_t ota_fw_size    = 0;
static uint32_t ota_fw_crc32   = 0;
static uint32_t ota_fw_version = 0;
static bool     ota_fw_ready   = false;

/* ========== I2C state ========== */
static uint8_t  current_addr  = 0x00;
static uint8_t  current_reg   = 0xFF;
static bool     reg_received  = false;

/* ========== I2C buffers ========== */
static uint8_t buf_gnss[18];
static uint8_t buf_imu_accel[6];
static uint8_t buf_imu_default[1];
static uint8_t buf_pressure[6];
static uint8_t buf_pressure_default[2];
static uint8_t buf_temperature[2];
static uint8_t buf_eps_voltage[2];
static uint8_t buf_eps_default[2];
static uint8_t *active_buf = NULL;

/* ========== UART state ========== */
static uint16_t tlm_timestamp = 0;
static uint8_t  uart_rx_buf[16];

/* ========== Simulation state ========== */
static float sim_time_s = 0.0f;

/* ==========================================================================
 * RANDOM HELPERS
 * ========================================================================== */

static float rand_float(float min, float max)
{
    return min + ((float)rand() / (float)RAND_MAX) * (max - min);
}

static float rand_gauss(float mean, float stddev)
{
    float u1 = ((float)rand() + 1.0f) / ((float)RAND_MAX + 2.0f);
    float u2 = (float)rand() / (float)RAND_MAX;
    float z  = sqrtf(-2.0f * logf(u1)) * cosf(2.0f * M_PI * u2);
    return mean + stddev * z;
}

/* ==========================================================================
 * SENSOR DATA PACKING HELPERS
 * ========================================================================== */

static void pack_float_be(uint8_t *dst, float val)
{
    union { float f; uint8_t b[4]; } u = {.f = val};
    dst[0] = u.b[3]; dst[1] = u.b[2];
    dst[2] = u.b[1]; dst[3] = u.b[0];
}

static void pack_int16_be(uint8_t *dst, int16_t val)
{
    dst[0] = (uint8_t)(val >> 8);
    dst[1] = (uint8_t)(val & 0xFF);
}

/* ==========================================================================
 * ORBITAL SIMULATION
 * ========================================================================== */

static bool is_in_eclipse(float t)
{
    float phase = fmodf(t, ORBIT_PERIOD_S) / ORBIT_PERIOD_S;
    float d = fabsf(phase - 0.5f);
    return d < (ECLIPSE_FRACTION / 2.0f);
}

static void update_sensor_data(void)
{
    float dt_real  = (float)SENSOR_UPDATE_MS / 1000.0f;
    float dt_sim   = dt_real * SIM_TIME_SCALE;
    sim_time_s    += dt_sim;

    float orbit_phase = fmodf(sim_time_s, ORBIT_PERIOD_S) / ORBIT_PERIOD_S;
    float orbit_angle = orbit_phase * 2.0f * M_PI;
    bool  eclipse     = is_in_eclipse(sim_time_s);

    /* GNSS */
    {
        float incl_rad = ORBIT_INCL_DEG * M_PI / 180.0f;
        float lat = asinf(sinf(incl_rad) * sinf(orbit_angle)) * 180.0f / M_PI;
        float lon_base = fmodf(sim_time_s * (360.0f / 86400.0f), 360.0f);
        float lon = fmodf(-9.14f - lon_base + cosf(orbit_angle) * 180.0f, 360.0f);
        if (lon >  180.0f) lon -= 360.0f;
        if (lon < -180.0f) lon += 360.0f;
        float alt = ORBIT_ALT_KM + 5.0f * sinf(orbit_angle * 2.0f) + rand_gauss(0.0f, 0.1f);
        float spd = ORBIT_SPEED_KMS + rand_gauss(0.0f, 0.005f);
        uint8_t fix_quality = 1;
        uint8_t sat_count   = (uint8_t)(8 + (int)(3.0f * sinf(orbit_angle)) + (rand() % 3));
        if (sat_count < 5)  sat_count = 5;
        if (sat_count > 14) sat_count = 14;
        pack_float_be(&buf_gnss[0],  lat + rand_gauss(0.0f, 0.0001f));
        pack_float_be(&buf_gnss[4],  lon + rand_gauss(0.0f, 0.0001f));
        pack_float_be(&buf_gnss[8],  alt);
        pack_float_be(&buf_gnss[12], spd);
        buf_gnss[16] = fix_quality;
        buf_gnss[17] = sat_count;
    }

    /* IMU */
    {
        bool spike = (rand() % 200) == 0;
        float ax, ay, az;
        if (spike) {
            ax = rand_gauss(0.0f, 0.3f);
            ay = rand_gauss(0.0f, 0.3f);
            az = rand_gauss(0.0f, 0.3f);
        } else {
            ax = rand_gauss(0.0f, 0.002f) + 1e-5f * sinf(orbit_angle);
            ay = rand_gauss(0.0f, 0.002f);
            az = rand_gauss(0.0f, 0.002f);
        }
        int16_t raw_ax = (int16_t)(ax / 9.81f * 16384.0f);
        int16_t raw_ay = (int16_t)(ay / 9.81f * 16384.0f);
        int16_t raw_az = (int16_t)(az / 9.81f * 16384.0f);
        pack_int16_be(&buf_imu_accel[0], raw_ax);
        pack_int16_be(&buf_imu_accel[2], raw_ay);
        pack_int16_be(&buf_imu_accel[4], raw_az);
        buf_imu_default[0] = 0x68;
    }

    /* Pressure */
    {
        uint32_t press_raw = (uint32_t)(3000 + rand() % 500);
        float board_temp = eclipse ? rand_gauss(5.0f, 2.0f) : rand_gauss(30.0f, 3.0f);
        uint32_t temp_raw  = (uint32_t)(board_temp * 256.0f + 32768.0f);
        buf_pressure[0] = (uint8_t)((press_raw >> 12) & 0xFF);
        buf_pressure[1] = (uint8_t)((press_raw >>  4) & 0xFF);
        buf_pressure[2] = (uint8_t)((press_raw <<  4) & 0xF0);
        buf_pressure[3] = (uint8_t)((temp_raw  >> 12) & 0xFF);
        buf_pressure[4] = (uint8_t)((temp_raw  >>  4) & 0xFF);
        buf_pressure[5] = (uint8_t)((temp_raw  <<  4) & 0xF0);
        buf_pressure_default[0] = 0x58;
        buf_pressure_default[1] = 0x00;
    }

    /* Temperature */
    {
        static float temp_current = 25.0f;
        float temp_target = eclipse ? rand_gauss(-10.0f, 5.0f) : rand_gauss(32.0f, 5.0f);
        temp_current += (temp_target - temp_current) * 0.02f;
        temp_current += rand_gauss(0.0f, 0.1f);
        int16_t raw_temp = (int16_t)(temp_current / 0.0625f);
        raw_temp <<= 4;
        pack_int16_be(buf_temperature, raw_temp);
    }

    /* EPS */
    {
        static float voltage = 28.0f;
        static float current = 0.5f;
        float v_target, i_target;
        if (eclipse) {
            v_target = rand_gauss(24.0f, 0.5f);
            i_target = rand_gauss(0.2f, 0.05f);
        } else {
            v_target = rand_gauss(30.0f, 0.5f);
            i_target = rand_gauss(1.0f, 0.2f);
        }
        voltage += (v_target - voltage) * 0.03f;
        current += (i_target - current) * 0.05f;
        voltage += rand_gauss(0.0f, 0.05f);
        current += rand_gauss(0.0f, 0.01f);
        if (voltage < 18.0f) voltage = 18.0f;
        if (voltage > 34.0f) voltage = 34.0f;
        if (current < 0.0f)  current = 0.0f;
        uint16_t v_raw = (uint16_t)(voltage * 100.0f);
        uint16_t i_raw = (uint16_t)(current * 100.0f);
        buf_eps_voltage[0] = (uint8_t)(v_raw >> 8);
        buf_eps_voltage[1] = (uint8_t)(v_raw & 0xFF);
        buf_eps_default[0] = (uint8_t)(i_raw >> 8);
        buf_eps_default[1] = (uint8_t)(i_raw & 0xFF);
    }
}

static void init_sensor_data(void)
{
    sim_time_s = 0.0f;
    update_sensor_data();
}

/* ==========================================================================
 * I2C HELPERS & HANDLERS
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
 * USB OTA RECEPTION
 *
 * The dashboard sends:
 *   "OTA_BEGIN" (9 B) | size (4 B BE) | crc32 (4 B BE) | version (4 B BE)
 *   | <size raw firmware bytes>
 *
 * This function is non-blocking unless it detects the magic — once the magic
 * is found it blocks until the full transfer is received (or it times out).
 * ========================================================================== */

/**
 * @brief Read exactly @p n bytes from USB stdin (pico_stdio_usb path).
 *
 * Uses getchar_timeout_us(500) so that each call pumps the USB task and
 * waits up to 0.5 ms for a byte.  The overall @p timeout_ms deadline ensures
 * we don't wait forever if the transfer stalls.
 *
 * Requires the host to have asserted DTR before sending — the dashboard
 * does this via port.assertDTR() before writing any data.
 *
 * @return true if all @p n bytes were received within @p timeout_ms.
 */
static bool usb_read_bytes(uint8_t *dst, uint32_t n, uint32_t timeout_ms)
{
    absolute_time_t deadline = make_timeout_time_ms(timeout_ms);
    uint32_t i = 0;
    while (i < n)
    {
        if (absolute_time_diff_us(deadline, get_absolute_time()) > 0)
            return false;
        /* 500 µs timeout: pumps the USB task and returns when a byte arrives */
        int c = getchar_timeout_us(500);
        if (c != PICO_ERROR_TIMEOUT)
            dst[i++] = (uint8_t)c;
    }
    return true;
}

/**
 * @brief Check USB stdin for an "OTA_BEGIN" transfer from the dashboard.
 *
 * Non-blocking unless the magic is detected — then blocks until the full
 * firmware transfer is received (or times out at 60 s).
 *
 * The dashboard asserts DTR before sending data (port.assertDTR()), which
 * makes tud_cdc_connected() return true on the Pico side so that
 * getchar_timeout_us() can receive data.
 */
static void check_usb_ota(void)
{
    /* -----------------------------------------------------------------------
     * NO printf() calls inside this function until AFTER all bytes are read.
     *
     * Reason: pico-sdk's stdio_usb uses a single mutex for both TX (printf)
     * and RX (getchar_timeout_us).  If printf blocks waiting for TX buffer
     * space (because nobody is reading the Pico's output), it holds that
     * mutex and prevents getchar_timeout_us from running, creating a
     * deadlock that starves the firmware reception loop.
     * ----------------------------------------------------------------------- */

    /* Non-blocking scan for 'O' — start of "OTA_BEGIN" */
    int c = getchar_timeout_us(0);
    if (c == PICO_ERROR_TIMEOUT || (char)c != 'O')
        return;

    /* Read remaining 8 bytes of magic "TA_BEGIN" */
    uint8_t rest[OTA_MAGIC_LEN - 1];
    if (!usb_read_bytes(rest, OTA_MAGIC_LEN - 1, 500U))
        return;
    if (memcmp(rest, "TA_BEGIN", 8) != 0)
        return;

    /* Read 12-byte metadata: size(4B BE) + crc32(4B BE) + version(4B BE) */
    uint8_t meta[12];
    if (!usb_read_bytes(meta, 12U, 2000U))
        return;

    uint32_t fw_size    = ((uint32_t)meta[0]  << 24) | ((uint32_t)meta[1]  << 16)
                        | ((uint32_t)meta[2]  <<  8) |  (uint32_t)meta[3];
    uint32_t fw_crc32   = ((uint32_t)meta[4]  << 24) | ((uint32_t)meta[5]  << 16)
                        | ((uint32_t)meta[6]  <<  8) |  (uint32_t)meta[7];
    uint32_t fw_version = ((uint32_t)meta[8]  << 24) | ((uint32_t)meta[9]  << 16)
                        | ((uint32_t)meta[10] <<  8) |  (uint32_t)meta[11];

    if (fw_size == 0 || fw_size > OTA_FW_BUF_SIZE)
        return;

    /* Receive firmware — no printf during this phase to avoid mutex contention */
    if (!usb_read_bytes(ota_fw_buf, fw_size, 60000U))
        return;

    /* All bytes received — safe to printf now (TX path no longer races RX) */
    ota_fw_size    = fw_size;
    ota_fw_crc32   = fw_crc32;
    ota_fw_version = fw_version;
    ota_fw_ready   = true;
    ota_total      = (uint16_t)((fw_size + OTA_PACKET_SIZE - 1U) / OTA_PACKET_SIZE);

    printf("[USB-OTA] OK: %lu B, crc=0x%08lX, %u pkts\n",
           fw_size, fw_crc32, ota_total);
}

/* ==========================================================================
 * UART helpers
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
        /* CMD_REQUEST_DATA (0x20) */
        tlm_timestamp++;
        cmd[0] = 0x20;
        cmd[1] = (uint8_t)(5 + rand() % 31);
        cmd[2] = (uint8_t)(tlm_timestamp >> 8);
        cmd[3] = (uint8_t)(tlm_timestamp & 0xFF);
        uart_write_blocking(TTC_UART, cmd, TTC_CMD_LEN);
    }
    else if (roll < 93)
    {
        /* CMD_ENTER_SAFE (0x01) */
        cmd[0] = 0x01; cmd[1] = 0; cmd[2] = 0; cmd[3] = 0;
        uart_write_blocking(TTC_UART, cmd, TTC_CMD_LEN);
    }
    else if (roll < 97)
    {
        /* CMD_REMOTE_CTRL (0x02) */
        cmd[0] = 0x02; cmd[1] = 0; cmd[2] = 0; cmd[3] = 0;
        uart_write_blocking(TTC_UART, cmd, TTC_CMD_LEN);
    }
    else
    {
        /* CMD_END_OTA (0x11) */
        cmd[0] = 0x11; cmd[1] = 0; cmd[2] = 0; cmd[3] = 0;
        uart_write_blocking(TTC_UART, cmd, TTC_CMD_LEN);
    }
}

/**
 * @brief Send one OTA packet (or the END marker) over UART to the OBC.
 *
 * Uses real firmware data from ota_fw_buf received via USB.
 * END marker payload[0..11] = size(4B BE) + crc32(4B BE) + version(4B BE).
 */
static void ota_send_packet(void)
{
    if (ota_seq >= ota_total)
    {
        /* END marker */
        ota_packet_buf[0] = (uint8_t)(OTA_SYNC_WORD >> 8);
        ota_packet_buf[1] = (uint8_t)(OTA_SYNC_WORD & 0xFF);
        ota_packet_buf[2] = 0xFF;
        ota_packet_buf[3] = 0xFF;
        ota_packet_buf[4] = 0x00;
        ota_packet_buf[5] = 0x0C;   /* payload_len = 12 bytes */

        /* size (4 B BE) */
        ota_packet_buf[OTA_HEADER_SIZE + 0] = (uint8_t)(ota_fw_size >> 24);
        ota_packet_buf[OTA_HEADER_SIZE + 1] = (uint8_t)(ota_fw_size >> 16);
        ota_packet_buf[OTA_HEADER_SIZE + 2] = (uint8_t)(ota_fw_size >>  8);
        ota_packet_buf[OTA_HEADER_SIZE + 3] = (uint8_t)(ota_fw_size & 0xFF);

        /* crc32 (4 B BE) */
        ota_packet_buf[OTA_HEADER_SIZE + 4] = (uint8_t)(ota_fw_crc32 >> 24);
        ota_packet_buf[OTA_HEADER_SIZE + 5] = (uint8_t)(ota_fw_crc32 >> 16);
        ota_packet_buf[OTA_HEADER_SIZE + 6] = (uint8_t)(ota_fw_crc32 >>  8);
        ota_packet_buf[OTA_HEADER_SIZE + 7] = (uint8_t)(ota_fw_crc32 & 0xFF);

        /* version (4 B BE) */
        ota_packet_buf[OTA_HEADER_SIZE + 8]  = (uint8_t)(ota_fw_version >> 24);
        ota_packet_buf[OTA_HEADER_SIZE + 9]  = (uint8_t)(ota_fw_version >> 16);
        ota_packet_buf[OTA_HEADER_SIZE + 10] = (uint8_t)(ota_fw_version >>  8);
        ota_packet_buf[OTA_HEADER_SIZE + 11] = (uint8_t)(ota_fw_version & 0xFF);

        memset(&ota_packet_buf[OTA_HEADER_SIZE + 12], 0x00, OTA_PACKET_SIZE - 12U);

        uart_write_blocking(TTC_UART, ota_packet_buf,
                            OTA_HEADER_SIZE + OTA_PACKET_SIZE);
        printf("[OTA] Sent END marker — size=%lu crc32=0x%08lX ver=%lu\n",
               ota_fw_size, ota_fw_crc32, ota_fw_version);

        ota_active   = false;
        ota_fw_ready = false;
        ota_seq      = 0;
        return;
    }

    /* Normal data packet — pull from firmware buffer */
    uint32_t offset  = (uint32_t)ota_seq * OTA_PACKET_SIZE;
    uint16_t pkt_len = OTA_PACKET_SIZE;
    if (offset + pkt_len > ota_fw_size)
        pkt_len = (uint16_t)(ota_fw_size - offset);

    ota_packet_buf[0] = (uint8_t)(OTA_SYNC_WORD >> 8);
    ota_packet_buf[1] = (uint8_t)(OTA_SYNC_WORD & 0xFF);
    ota_packet_buf[2] = (uint8_t)(ota_seq >> 8);
    ota_packet_buf[3] = (uint8_t)(ota_seq & 0xFF);
    ota_packet_buf[4] = (uint8_t)(pkt_len >> 8);
    ota_packet_buf[5] = (uint8_t)(pkt_len & 0xFF);

    memcpy(&ota_packet_buf[OTA_HEADER_SIZE], &ota_fw_buf[offset], pkt_len);
    if (pkt_len < OTA_PACKET_SIZE)
        memset(&ota_packet_buf[OTA_HEADER_SIZE + pkt_len], 0xFF,
               OTA_PACKET_SIZE - pkt_len);

    uart_write_blocking(TTC_UART, ota_packet_buf,
                        OTA_HEADER_SIZE + OTA_PACKET_SIZE);
    printf("[OTA] Sent packet %u/%u\n", ota_seq + 1U, ota_total);
    ota_seq++;
}

/* ==========================================================================
 * MAIN — Core 0: I2C PIO + UART + sensor update
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

    printf("========================================\n");
    printf("  CubeSat Multi-Slave Simulator (PIO)\n");
    printf("  Dynamic LEO Simulation Active\n");
    printf("  GNSS=0x%02X IMU=0x%02X PRESS=0x%02X\n",
           ADDR_GNSS, ADDR_IMU, ADDR_PRESSURE);
    printf("  TEMP=0x%02X EPS=0x%02X\n",
           ADDR_TEMPERATURE, ADDR_EPS);
    printf("  SDA=GP%d SCL=GP%d\n", SDA_PIN, SDA_PIN + 1);
    printf("  1 real sec = %.0f sim secs\n", SIM_TIME_SCALE);
    printf("  OTA buffer: %u KB\n", OTA_FW_BUF_SIZE / 1024U);
    printf("========================================\n");

    for (int i = 0; i < 3; i++)
    {
        gpio_put(LED_PIN, 1); sleep_ms(100);
        gpio_put(LED_PIN, 0); sleep_ms(100);
    }

    /* ===== Handshake de sincronização com o OBC =====
     *
     * O Pico envia [0xAA][0x55][0xAA][0x55] em loop até o OBC responder
     * com [0x55][0xAA][0x55][0xAA].  O OBC detecta a sequência lendo 1 byte
     * de cada vez, por isso funciona mesmo que o OBC tenha arrancado a meio
     * de um byte e esteja desalinhado.
     * ================================================ */
    {
        const uint8_t sync_frame[4] = {0xAA, 0x55, 0xAA, 0x55};
        uint8_t       resp[4]       = {0U, 0U, 0U, 0U};
        bool          synced        = false;

        printf("[SYNC] A aguardar OBC...\n");
        uart_flush_rx();

        while (!synced)
        {
            uart_write_blocking(TTC_UART, sync_frame, sizeof(sync_frame));

            /* Espera até 600 ms pela resposta [0x55][0xAA][0x55][0xAA] */
            absolute_time_t deadline = make_timeout_time_ms(600);
            uint8_t idx = 0U;
            while (idx < 4U &&
                   absolute_time_diff_us(deadline, get_absolute_time()) < 0)
            {
                if (uart_is_readable(TTC_UART))
                    resp[idx++] = uart_getc(TTC_UART);
            }

            if (idx == 4U       &&
                resp[0] == 0x55 && resp[1] == 0xAA &&
                resp[2] == 0x55 && resp[3] == 0xAA)
            {
                synced = true;
                printf("[SYNC] OBC pronto!\n");
            }
            else
            {
                printf("[SYNC] Sem resposta — a tentar de novo...\n");
                sleep_ms(100);
            }
        }

        uart_flush_rx();  /* descartar bytes residuais */
    }

    absolute_time_t next_cmd       = get_absolute_time();
    absolute_time_t next_sensor    = get_absolute_time();
    absolute_time_t next_heartbeat = make_timeout_time_ms(10000); /* first beat in 10 s */

    while (true)
    {
        /* Periodic alive print — helps confirm USB CDC is up and Pico is running */
        if (absolute_time_diff_us(next_heartbeat, get_absolute_time()) > 0)
        {
            printf("[PICO] alive | ota_fw_ready=%d ota_active=%d\n",
                   (int)ota_fw_ready, (int)ota_active);
            next_heartbeat = make_timeout_time_ms(10000);
        }

        /* Check for firmware arriving from the dashboard via USB */
        check_usb_ota();

        ttc_check_response();

        /* Update sensor data periodically */
        if (absolute_time_diff_us(next_sensor, get_absolute_time()) > 0)
        {
            update_sensor_data();
            next_sensor = make_timeout_time_ms(SENSOR_UPDATE_MS);
        }

        if (ota_active)
        {
            /* Send next firmware packet to OBC */
            ota_send_packet();
            sleep_ms(OTA_PACKET_INTERVAL_MS);
        }
        else if (ota_fw_ready)
        {
            /* Firmware buffered — initiate OTA handshake with OBC */
            uint8_t cmd[TTC_CMD_LEN] = {0x10, 0x01, 0x00, 0x00};
            printf("[OTA] === Starting OTA handshake with OBC ===\n");
            uart_flush_rx();
            uart_write_blocking(TTC_UART, cmd, TTC_CMD_LEN);

            if (ttc_wait_for_ack(OTA_ACK_WAIT_MS))
            {
                printf("[OTA] Handshake OK — sending %u packets\n", ota_total);
                uart_flush_rx();
                sleep_ms(500);
                ota_active = true;
                ota_seq    = 0;
            }
            else
            {
                printf("[OTA] ACK timeout — will retry next loop\n");
                sleep_ms(1000);
            }
        }
        else if (absolute_time_diff_us(next_cmd, get_absolute_time()) > 0)
        {
            ttc_send_command();
            next_cmd = make_timeout_time_ms(TTC_INTERVAL_MS);
        }
    }

    return 0;
}
