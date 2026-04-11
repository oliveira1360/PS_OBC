/**
 * @file propulsor.c
 * @brief Peripheral driver for the propulsion system via SPI.
 *
 * Implements non-blocking async communication with the propulsor
 * using the SPI driver layer. Handles telemetry requests, command
 * transmission and response parsing with checksum verification.
 */

#include "peripherals/propulsor.h"
#include "drivers/spi_driver.h"
#include "hal/hal_spi.h"
#include "config/board.h"

/* =========================================================================
   Estado interno
   ========================================================================= */
static spi_handle_t     spi        = {0};
static uint8_t          rx_buf[PROPULSOR_BUF_LEN];
static uint8_t          tx_buf[PROPULSOR_BUF_LEN];
static propulsor_data_t propulsor  = {0};
static uint8_t          waiting_tx = 0U;

/**
 * @brief Parses a telemetry response frame from the propulsor.
 *
 * Expected frame layout:
 * | Byte | Field         | Scale       |
 * |------|---------------|-------------|
 * |  0   | STATUS        | enum        |
 * |  1-2 | PRESSURE_H/L  | /100 = bar  |
 * |  3-4 | TEMP_H/L      | /10  = °C   |
 * |  5-6 | THRUST_H/L    | /10  = N    |
 * |  7   | VALVE_STATE   | 0/1         |
 * |  8   | CHECKSUM      | XOR [0..7]  |
 *
 * @param buf Pointer to received byte buffer of length PROPULSOR_BUF_LEN.
 */
static void propulsor_parse(uint8_t *buf)
{
    uint8_t chk = 0U;
    for (uint8_t i = 0U; i < PROPULSOR_BUF_LEN - 1U; i++)
        chk ^= buf[i];

    if (chk != buf[PROPULSOR_BUF_LEN - 1U])
        return;

    propulsor.status = (propulsor_status_t)buf[0];

    uint16_t press_raw         = ((uint16_t)buf[1] << 8U) | buf[2];
    propulsor.chamber_pressure = (float)press_raw / 100.0f;

    uint16_t temp_raw          = ((uint16_t)buf[3] << 8U) | buf[4];
    propulsor.temperature      = (float)temp_raw / 10.0f;

    uint16_t thrust_raw        = ((uint16_t)buf[5] << 8U) | buf[6];
    propulsor.thrust           = (float)thrust_raw / 10.0f;

    propulsor.valve_state      = buf[7];
}

/**
 * @brief SPI TX completion callback.
 *
 * Called by the SPI driver when a transmission completes.
 * Clears the waiting_tx flag and re-arms the RX path.
 *
 * @param result 0 on success, -1 on timeout or error.
 */
static void on_propulsor_tx_done(int result)
{
    (void)result;
    waiting_tx = 0U;
    propulsor_read_async();
}

/**
 * @brief SPI RX completion callback.
 *
 * Called by the SPI driver when a full frame has been received.
 * Parses the frame and re-arms reception for continuous polling.
 *
 * @param result 0 on success, -1 on timeout or error.
 */
static void on_propulsor_rx_done(int result)
{
    if (result == 0)
        propulsor_parse(rx_buf);

    if (!waiting_tx)
        propulsor_read_async();
}

/**
 * @brief Sends a telemetry request command to the propulsor.
 *
 * Builds a TX frame with PROP_CMD_REQUEST_TLM and a XOR checksum,
 * then initiates an async SPI transfer.
 */
static void propulsor_send_telemetry_request(void)
{
    uint8_t i = 0U;

    tx_buf[i++] = PROP_CMD_REQUEST_TLM;
    tx_buf[i++] = 0x00U;

    uint8_t chk = 0U;
    for (uint8_t j = 0U; j < i; j++)
        chk ^= tx_buf[j];
    tx_buf[i++] = chk;

    waiting_tx = 1U;
    spi_transfer_async(&spi, PROPULSOR_CS_PIN,
                       tx_buf, rx_buf, PROPULSOR_BUF_LEN,
                       on_propulsor_tx_done);
}

/**
 * @brief Initiates an async telemetry read from the propulsor.
 *
 * No-op if SPI is already busy. Should be called periodically
 * from the main loop or after system initialisation.
 */
void propulsor_read_async(void)
{
    if (spi.state != SPI_IDLE)
        return;

    propulsor_send_telemetry_request();
}

/**
 * @brief Sends a command to the propulsor.
 *
 * Builds a TX frame with the given command and payload,
 * appends a XOR checksum and initiates an async SPI transfer.
 *
 * @param cmd     Command to send — see propulsor_cmd_t.
 * @param payload Optional command parameter (0x00 if unused).
 */
void propulsor_send_cmd(propulsor_cmd_t cmd, uint8_t payload)
{
    if (spi.state != SPI_IDLE)
        return;

    uint8_t i = 0U;

    tx_buf[i++] = (uint8_t)cmd;
    tx_buf[i++] = payload;

    uint8_t chk = 0U;
    for (uint8_t j = 0U; j < i; j++)
        chk ^= tx_buf[j];
    tx_buf[i++] = chk;

    propulsor.last_command = cmd;
    waiting_tx             = 1U;

    spi_transfer_async(&spi, PROPULSOR_CS_PIN,
                       tx_buf, rx_buf, PROPULSOR_BUF_LEN,
                       on_propulsor_tx_done);
}

/**
 * @brief Advances the propulsor SPI state machine by one tick.
 *
 * Must be called every cycle from sensors_tick() or equivalent.
 */
void propulsor_tick(void)
{
    spi_tick(&spi);
}

/**
 * @brief Returns a pointer to the latest propulsor telemetry data.
 *
 * @return Pointer to internal propulsor_data_t — do not free.
 */
propulsor_data_t *propulsor_get(void)
{
    return &propulsor;
}