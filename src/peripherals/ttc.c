/**
 * @file ttc.c
 * @brief Subsistema de Telemetria, Rastreio e Comando (TT&C).
 *
 * Utiliza o driver USART com FSMs separadas de TX e RX.
 */

#include <stdio.h>
#include "hal/hal_usart.h"
#include "drivers/usart_driver.h"
#include "config/board.h"
#include "app/sensors.h"

static usart_handle_t usart = {0};
static uint8_t rx_buf[TTC_BUF_LEN];
static uint8_t tx_buf[TTC_BUF_LEN];

static uint8_t waiting_tx = 0U;

static void ttc_parse(uint8_t *buf);
void        ttc_read_async(void);

static void on_ttc_tx_done(int result)
{
    (void)result;
    waiting_tx = 0U;
    ttc_read_async();
}

static void on_ttc_done(int result)
{
    if (result == 1)
        ttc_parse(rx_buf);
    if (!waiting_tx)
        ttc_read_async();
}

static void ttc_parse(uint8_t *buf)
{
    ground_command_t cmd = (ground_command_t)buf[0];
    ttc.last_command = cmd;

    switch (cmd)
    {
    case CMD_REQUEST_DATA:
        ttc.doppler = (float)buf[1] / 10.0f;
        ttc_send_telemetry();
        break;
    case CMD_ENTER_SAFE:
        ttc.cmd_status = ACK_SUCCESS;
        break;

    case CMD_START_OTA:
        ttc.ota_active = true;
        ttc.cmd_status = ACK_SUCCESS;
        break;

    case CMD_END_OTA:
        ttc.ota_active = false;
        ttc.cmd_status = ACK_SUCCESS;
        break;

    case CMD_RECEIVING_OTA:
        ttc.cmd_status = ACK_SUCCESS;
        break;

    case CMD_REMOTE_CTRL:
        ttc.cmd_status = ACK_SUCCESS;
        break;

    case CMD_NONE:
    default:
        ttc.cmd_status = ACK_FAILED;
        break;
    }
}

/**
 * @brief Inicia receção assíncrona de um comando da Ground Station.
 */
void ttc_read_async(void)
{
    if (usart.rx_state != UART_RX_IDLE)
        return;

    usart_recv_async(&usart, rx_buf, TTC_BUF_LEN, on_ttc_done);
}

/**
 * @brief Tick do TT&C — chama as duas FSMs (TX e RX).
 *
 * Deve ser chamado em cada iteração do super-loop.
 */
void ttc_tick(void)
{
    usart_tx_tick(&usart);
    usart_rx_tick(&usart);
}

/**
 * @brief Envia frame de telemetria para a Ground Station.
 */
void ttc_send_telemetry(void)
{
    uint8_t i = 0U;

    tx_buf[i++] = CMD_REQUEST_DATA;
    tx_buf[i++] = (uint8_t)(eps.voltage  * 10.0f);
    tx_buf[i++] = (uint8_t)(eps.current * 100.0f);
    tx_buf[i++] = (uint8_t)gnss.latitude;
    tx_buf[i++] = (uint8_t)((gnss.latitude  - (uint8_t)gnss.latitude)  * 100.0f);
    tx_buf[i++] = (uint8_t)gnss.longitude;
    tx_buf[i++] = (uint8_t)((gnss.longitude - (uint8_t)gnss.longitude) * 100.0f);

    uint16_t press = (uint16_t)pressure.pressure;
    tx_buf[i++] = (uint8_t)(press >> 8U);
    tx_buf[i++] = (uint8_t)(press & 0xFFU);

    uint16_t temp = (uint16_t)temperature.temperature;
    tx_buf[i++] = (uint8_t)(temp >> 8U);
    tx_buf[i++] = (uint8_t)(temp & 0xFFU);

    tx_buf[i++] = ttc.current_state;

    uint8_t chk = 0U;
    for (uint8_t j = 0U; j < i; j++)
        chk ^= tx_buf[j];
    tx_buf[i++] = chk;

    waiting_tx = 1U;
    usart_send_async(&usart, tx_buf, i);
}