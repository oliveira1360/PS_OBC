#include "peripherals/propulsor.h"
#include "drivers/spi_driver.h"
#include "hal/hal_spi.h"
#include "config/board.h"

/* =========================================================================
   Estado interno
   ========================================================================= */
static spi_handle_t      spi          = {0};
static uint8_t           rx_buf[PROPULSOR_BUF_LEN];
static uint8_t           tx_buf[PROPULSOR_BUF_LEN];
static propulsor_data_t  propulsor    = {0};
static uint8_t           waiting_tx  = 0U;


static void propulsor_parse(uint8_t *buf);
static void propulsor_send_telemetry_request(void);

/* 
   frame de resposta do propulsor:
   [STATUS] [PRESSURE_H] [PRESSURE_L] [TEMP_H] [TEMP_L]
   [THRUST_H] [THRUST_L] [VALVE_STATE] [CHECKSUM]
    */
static void propulsor_parse(uint8_t *buf)
{
    /* verifica checksum — XOR de todos os bytes excepto o ultimo */
    uint8_t chk = 0U;
    for (uint8_t i = 0U; i < PROPULSOR_BUF_LEN - 1U; i++)
        chk ^= buf[i];

    if (chk != buf[PROPULSOR_BUF_LEN - 1U])
        return;  /* frame corrompido — descarta */

    propulsor.status           = (propulsor_status_t)buf[0];

    uint16_t press_raw         = ((uint16_t)buf[1] << 8U) | buf[2];
    propulsor.chamber_pressure = (float)press_raw / 100.0f;  /* bar */

    uint16_t temp_raw          = ((uint16_t)buf[3] << 8U) | buf[4];
    propulsor.temperature      = (float)temp_raw / 10.0f;    /* °C  */

    uint16_t thrust_raw        = ((uint16_t)buf[5] << 8U) | buf[6];
    propulsor.thrust           = (float)thrust_raw / 10.0f;  /* N   */

    propulsor.valve_state      = buf[7];
}

/* =========================================================================
   Callbacks
   ========================================================================= */
static void on_propulsor_tx_done(int result)
{
    (void)result;
    waiting_tx = 0U;
    propulsor_read_async();  /* após TX, arma RX para resposta */
}

static void on_propulsor_rx_done(int result)
{
    if (result == 0)
        propulsor_parse(rx_buf);

    if (!waiting_tx)
        propulsor_read_async();  /* relança leitura continua */
}

/* =========================================================================
   Pede telemetria ao propulsor — envia PROP_CMD_REQUEST_TLM
   ========================================================================= */
static void propulsor_send_telemetry_request(void)
{
    uint8_t i = 0U;

    tx_buf[i++] = PROP_CMD_REQUEST_TLM;
    tx_buf[i++] = 0x00U;  /* padding */

    /* checksum */
    uint8_t chk = 0U;
    for (uint8_t j = 0U; j < i; j++)
        chk ^= tx_buf[j];
    tx_buf[i++] = chk;

    waiting_tx = 1U;
    spi_transfer_async(&spi, PROPULSOR_CS_PIN,
                       tx_buf, rx_buf, PROPULSOR_BUF_LEN,
                       on_propulsor_tx_done);
}

/* =========================================================================
   Interface pública
   ========================================================================= */
void propulsor_read_async(void)
{
    if (spi.state != SPI_IDLE)
        return;

    propulsor_send_telemetry_request();
}

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

    waiting_tx = 1U;
    spi_transfer_async(&spi, PROPULSOR_CS_PIN,
                       tx_buf, rx_buf, PROPULSOR_BUF_LEN,
                       on_propulsor_tx_done);
}

void propulsor_tick(void)
{
    spi_tick(&spi);
}

propulsor_data_t *propulsor_get(void)
{
    return &propulsor;
}