/**
 * @file cubesat_slave.c
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

/* ========== USB↔UART passthrough ========== */
/* The Pico no longer builds OTA packets or sends random commands.
 * All TTC commands (CMD_REQUEST_DATA, CMD_START_OTA, OTA packets, etc.)
 * come from the dashboard via USB and are forwarded byte-for-byte to the OBC. */

/* ========== Bluetooth HC-05 (UART0) ========== */
/* Set to 1 to use HC-05 on UART0 (GP0 TX / GP1 RX) as the passthrough channel.
 * Set to 0 to use USB (stdio) as the passthrough channel.
 * Recompile after changing. */
#define USE_BLUETOOTH 0

#define BT_UART     uart0
#define BT_TX_PIN   16
#define BT_RX_PIN   17
#define BT_BAUDRATE TTC_BAUDRATE

/* ========== Timing (ms) ========== */
#define SENSOR_UPDATE_MS       500

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
/* (no persistent state needed — passthrough is stateless) */

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

    /* orbit_phase ∈ [0.0, 1.0)  — posição normalizada na órbita
     * orbit_angle ∈ [0.0, 2π)  — correspondente em radianos
     * eclipse     = true durante ~35% do período orbital (zona de sombra) */
    float orbit_phase = fmodf(sim_time_s, ORBIT_PERIOD_S) / ORBIT_PERIOD_S;
    float orbit_angle = orbit_phase * 2.0f * M_PI;
    bool  eclipse     = is_in_eclipse(sim_time_s);

    /* ------------------------------------------------------------------ */
    /* GNSS                                                                 */
    /* ------------------------------------------------------------------ */
    {
        float incl_rad = ORBIT_INCL_DEG * M_PI / 180.0f;

        /* lat  ∈ [-ORBIT_INCL_DEG, +ORBIT_INCL_DEG]  (e.g. ±98° SSO)
         *       + ruído gaussiano σ=0.0001°  → negligível                */
        float lat = asinf(sinf(incl_rad) * sinf(orbit_angle)) * 180.0f / M_PI;

        /* lon  ∈ [-180°, +180°]  — deriva com a rotação terrestre
         *       + componente cosseno para cobrir toda a gama longitudinal */
        float lon_base = fmodf(sim_time_s * (360.0f / 86400.0f), 360.0f);
        float lon = fmodf(-9.14f - lon_base + cosf(orbit_angle) * 180.0f, 360.0f);
        if (lon >  180.0f) lon -= 360.0f;
        if (lon < -180.0f) lon += 360.0f;

        /* alt  = ORBIT_ALT_KM ± 5 km (variação orbital) + ruído σ=0.1 km */
        float alt = ORBIT_ALT_KM + 5.0f * sinf(orbit_angle * 2.0f) + rand_gauss(0.0f, 0.1f);

        /* spd  = ORBIT_SPEED_KMS + ruído σ=0.005 km/s  (~±15 m/s)        */
        float spd = ORBIT_SPEED_KMS + rand_gauss(0.0f, 0.005f);

        /* fix_quality = 1  (fixo, sem variação)
         * sat_count   ∈ [5, 14]  — varia sinusoidalmente + jitter ±1     */
        uint8_t fix_quality = 1;
        uint8_t sat_count   = (uint8_t)(8 + (int)(3.0f * sinf(orbit_angle)) + (rand() % 3));
        if (sat_count < 5)  sat_count = 5;
        if (sat_count > 14) sat_count = 14;

        /* Packed big-endian: [lat(4B)][lon(4B)][alt(4B)][spd(4B)][fix(1B)][sats(1B)] */
        pack_float_be(&buf_gnss[0],  lat + rand_gauss(0.0f, 0.0001f));
        pack_float_be(&buf_gnss[4],  lon + rand_gauss(0.0f, 0.0001f));
        pack_float_be(&buf_gnss[8],  alt);
        pack_float_be(&buf_gnss[12], spd);
        buf_gnss[16] = fix_quality;
        buf_gnss[17] = sat_count;
    }

    /* ------------------------------------------------------------------ */
    /* IMU — acelerómetro                                                   */
    /* ------------------------------------------------------------------ */
    {
        /* Spike aleatório com probabilidade 1/200 por tick               */
        bool spike = (rand() % 200) == 0;
        float ax, ay, az;

        if (spike) {
            /* Spike:  ax/ay/az ∈ ~[-0.9, +0.9] m/s²  (3σ, gaussiano)   */
            ax = rand_gauss(0.0f, 0.3f);
            ay = rand_gauss(0.0f, 0.3f);
            az = rand_gauss(0.0f, 0.3f);
        } else {
            /* Normal: micro-gravidade
             *   ax ≈ ±0.006 m/s²  (σ=0.002) + modulação orbital ~1e-5  
             *   ay ≈ ±0.006 m/s²  (σ=0.002)
             *   az ≈ ±0.006 m/s²  (σ=0.002)                            */
            ax = rand_gauss(0.0f, 0.002f) + 1e-5f * sinf(orbit_angle);
            ay = rand_gauss(0.0f, 0.002f);
            az = rand_gauss(0.0f, 0.002f);
        }

        /* Conversão para raw int16 (escala ±2g = ±16384 LSB/g):
         *   normal: raw ≈ ±10 LSB
         *   spike:  raw ≈ ±502 LSB                                       */
        int16_t raw_ax = (int16_t)(ax / 9.81f * 16384.0f);
        int16_t raw_ay = (int16_t)(ay / 9.81f * 16384.0f);
        int16_t raw_az = (int16_t)(az / 9.81f * 16384.0f);

        /* Packed big-endian: [ax(2B)][ay(2B)][az(2B)] */
        pack_int16_be(&buf_imu_accel[0], raw_ax);
        pack_int16_be(&buf_imu_accel[2], raw_ay);
        pack_int16_be(&buf_imu_accel[4], raw_az);

        /* WHO_AM_I = 0x68  (fixo, byte de identificação MPU-6050/similar) */
        buf_imu_default[0] = 0x68;
    }

    /* ------------------------------------------------------------------ */
    /* Pressure (BMP280-like, formato raw 20-bit + 20-bit)                 */
    /* ------------------------------------------------------------------ */
    {
        /* press_raw ∈ [3000, 3499]  — valor ADC bruto (sem unidade física real) */
        uint32_t press_raw = (uint32_t)(3000 + rand() % 500);

        /* board_temp:
         *   eclipse → gaussiana μ=5°C,  σ=2°C  → típico ≈ [1, 9]°C
         *   sol     → gaussiana μ=30°C, σ=3°C  → típico ≈ [24, 36]°C  */
        float board_temp = eclipse ? rand_gauss(5.0f, 2.0f) : rand_gauss(30.0f, 3.0f);

        /* temp_raw = board_temp * 256 + 32768
         *   eclipse: ≈ [32896, 34560]  (board_temp ∈ [1,9]°C)
         *   sol:     ≈ [38912, 41984]  (board_temp ∈ [24,36]°C)         */
        uint32_t temp_raw  = (uint32_t)(board_temp * 256.0f + 32768.0f);

        /* Packed em 6 bytes: press_raw[19:0] nos bits [7:4] dos bytes 0-2
         *                    temp_raw [19:0] nos bits [7:4] dos bytes 3-5 */
        buf_pressure[0] = (uint8_t)((press_raw >> 12) & 0xFF);
        buf_pressure[1] = (uint8_t)((press_raw >>  4) & 0xFF);
        buf_pressure[2] = (uint8_t)((press_raw <<  4) & 0xF0);
        buf_pressure[3] = (uint8_t)((temp_raw  >> 12) & 0xFF);
        buf_pressure[4] = (uint8_t)((temp_raw  >>  4) & 0xFF);
        buf_pressure[5] = (uint8_t)((temp_raw  <<  4) & 0xF0);

        /* Chip ID / status (fixo) */
        buf_pressure_default[0] = 0x58;   /* BMP280 chip_id */
        buf_pressure_default[1] = 0x00;
    }

    /* ------------------------------------------------------------------ */
    /* Temperature (TMP102-like, raw int16 com resolução 0.0625°C/LSB)     */
    /* ------------------------------------------------------------------ */
    {
        static float temp_current = 25.0f;   /* estado persistente entre ticks */

        /* temp_target:
         *   eclipse → μ=-10°C, σ=5°C  → típico ≈ [-25, +5]°C
         *   sol     → μ=+32°C, σ=5°C  → típico ≈ [+17, +47]°C          */
        float temp_target = eclipse ? rand_gauss(-10.0f, 5.0f) : rand_gauss(32.0f, 5.0f);

        /* Filtro de 1ª ordem τ=50 ticks (≈50 × SENSOR_UPDATE_MS)
         *   temp_current converge lentamente para temp_target            */
        temp_current += (temp_target - temp_current) * 0.02f;
        temp_current += rand_gauss(0.0f, 0.1f);   /* ruído σ=0.1°C       */

        /* raw = (temp_current / 0.0625) << 4  — formato TMP102 big-endian */
        int16_t raw_temp = (int16_t)(temp_current / 0.0625f);
        raw_temp <<= 4;
        pack_int16_be(buf_temperature, raw_temp);
    }

    /* ------------------------------------------------------------------ */
    /* EPS — tensão e corrente do barramento de potência                    */
    /* ------------------------------------------------------------------ */
    {
        static float voltage = 5.0f;   /* estado persistente entre ticks */
        static float current = 1.5f;

        float v_target, i_target;

        /* eclipse → descarga:
         *   v_target ≈ 3.7V ± 0.15V  (3σ ≈ [3.25, 4.15] V)
         *   i_target ≈ 1.2A ± 0.05A  (3σ ≈ [1.05, 1.35] A)
         * sol     → carga:
         *   v_target ≈ 5.8V ± 0.15V  (3σ ≈ [5.35, 6.25] V)
         *   i_target ≈ 1.7A ± 0.08A  (3σ ≈ [1.46, 1.94] A)             */
        if (eclipse) {
            v_target = rand_gauss(3.7f, 0.15f);
            i_target = rand_gauss(1.2f, 0.05f);
        } else {
            v_target = rand_gauss(5.8f, 0.15f);
            i_target = rand_gauss(1.7f, 0.08f);
        }

        /* Filtros de 1ª ordem:
         *   voltage: τ ≈ 33 ticks  (constante 0.03)
         *   current: τ ≈ 20 ticks  (constante 0.05)                     */
        voltage += (v_target - voltage) * 0.03f;
        current += (i_target - current) * 0.05f;

        /* Ruído de medição: σ_v=0.02V, σ_i=0.005A                       */
        voltage += rand_gauss(0.0f, 0.02f);
        current += rand_gauss(0.0f, 0.005f);

        /* Hard clamps — limites seguros aceites pelo decoder OBC:
         *   voltage ∈ [3.1, 6.4] V
         *   current ∈ [1.0, 1.95] A                                     */
        if (voltage < 3.1f) voltage = 3.1f;
        if (voltage > 6.4f) voltage = 6.4f;
        if (current < 1.0f) current = 1.0f;
        if (current > 1.95f) current = 1.95f;

        /* Encoding para buf[]:
         *   buf[0] = (uint8_t)(voltage * 10)  → e.g. 5.8V → 58
         *   buf[1] = (uint8_t)(current * 100) → e.g. 1.7A → 170
         * Decoder OBC inverte: voltage = buf[0]/10, current = buf[1]/100 */
        buf_eps_voltage[0] = (uint8_t)(voltage * 10.0f);
        buf_eps_voltage[1] = (uint8_t)(current * 100.0f);

        buf_eps_default[0] = 0x00;
        buf_eps_default[1] = 0x00;
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
 * USB ↔ UART PASSTHROUGH
 *
 * All TTC commands (CMD_REQUEST_DATA, CMD_START_OTA, OTA packets, …) now
 * come from the dashboard via USB (COM6) and are forwarded byte-for-byte
 * to the OBC on TTC_UART.  OBC responses travel in the opposite direction.
 *
 * This replaces the old approach where the Pico independently sent random
 * commands and built OTA packets itself.
 * ========================================================================== */

/**
 * @brief Forward any bytes waiting on USB stdin to TTC_UART (dashboard → OBC).
 */
static void usb_to_uart(void)
{
    int c;
    while ((c = getchar_timeout_us(0)) != PICO_ERROR_TIMEOUT)
        uart_putc_raw(TTC_UART, (uint8_t)c);
}

/**
 * @brief Forward any bytes waiting on TTC_UART to USB stdout (OBC → dashboard).
 */
static void uart_to_usb(void) {
    bool any = false;
    while (uart_is_readable(TTC_UART)) { putchar_raw(uart_getc(TTC_UART)); any = true; }
    if (any) stdio_flush();   // empurra já para o USB, sem esperar pelo buffer encher
}

/**
 * @brief Forward any bytes waiting on BT_UART to TTC_UART (HC-05 → OBC).
 */
static void bt_to_uart(void)
{
    while (uart_is_readable(BT_UART))
        uart_putc_raw(TTC_UART, uart_getc(BT_UART));
}

/**
 * @brief Forward any bytes waiting on TTC_UART to BT_UART (OBC → HC-05).
 */
static void uart_to_bt(void)
{
    while (uart_is_readable(TTC_UART))
        uart_putc_raw(BT_UART, uart_getc(TTC_UART));
}

/* ==========================================================================
 * UART helpers
 * ========================================================================== */

static void uart_flush_rx(void)
{
    while (uart_is_readable(TTC_UART))
        (void)uart_getc(TTC_UART);
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

    /* ===== UART init (TTC — OBC) ===== */
    uart_init(TTC_UART, TTC_BAUDRATE);
    gpio_set_function(TTC_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(TTC_RX_PIN, GPIO_FUNC_UART);
    uart_set_hw_flow(TTC_UART, false, false);
    uart_set_format(TTC_UART, DATA_BITS, STOP_BITS, PARITY);
    uart_set_fifo_enabled(TTC_UART, true);
    uart_flush_rx();

#if USE_BLUETOOTH
    /* ===== UART0 init (HC-05 Bluetooth) ===== */
    uart_init(BT_UART, BT_BAUDRATE);
    gpio_set_function(BT_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(BT_RX_PIN, GPIO_FUNC_UART);
    uart_set_hw_flow(BT_UART, false, false);
    uart_set_format(BT_UART, DATA_BITS, STOP_BITS, PARITY);
    uart_set_fifo_enabled(BT_UART, true);
#endif


#if USE_BLUETOOTH
#else
#endif

    for (int i = 0; i < 3; i++)
    {
        gpio_put(LED_PIN, 1); sleep_ms(100);
        gpio_put(LED_PIN, 0); sleep_ms(100);
    }

    /* Sync removed — OBC boots directly in command mode (sync_state = SYNC_DONE).
     * Pico is now a pure passthrough: no sync ceremony needed. */
    uart_flush_rx();

    absolute_time_t next_sensor    = get_absolute_time();
    absolute_time_t next_heartbeat = make_timeout_time_ms(10000);

    while (true)
    {
        /* Alive heartbeat */
        if (absolute_time_diff_us(next_heartbeat, get_absolute_time()) > 0)
        {
            next_heartbeat = make_timeout_time_ms(10000);
        }

        /* Passthrough: dashboard ↔ OBC */
#if USE_BLUETOOTH
        bt_to_uart();
        uart_to_bt();
#else
        usb_to_uart();
        uart_to_usb();
#endif

        /* Update simulated sensor data periodically */
        if (absolute_time_diff_us(next_sensor, get_absolute_time()) > 0)
        {
            update_sensor_data();
            next_sensor = make_timeout_time_ms(SENSOR_UPDATE_MS);
        }
    }

    return 0;
}
