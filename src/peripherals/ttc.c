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
static uint8_t rx_buf[OTA_FULL_PACKET];
static uint8_t tx_buf[TTC_BUF_LEN];
static uint8_t ota_ack_buf[TTC_CMD_LEN];

static uint8_t waiting_tx = 0U;
static uint8_t ota_receiving = 0U;
static uint8_t ota_data_started = 0U; /* primeiro pacote OTA real recebido */
static uint8_t ota_partial = 0U;      /* bytes já recebidos do primeiro pacote OTA */

/* =========================================================================
 * Buffer de staging de pacotes OTA
 * Acedido por otaMode() via ttc_ota_* API.
 * ========================================================================= */
static uint8_t  s_ota_pkt_ready   = 0U;
static uint16_t s_ota_pkt_seq     = 0U;
static uint16_t s_ota_pkt_len     = 0U;
static uint8_t  s_ota_pkt_buf[OTA_PACKET_SIZE];

static void ttc_parse(uint8_t *buf);
void ttc_read_async(void);
static void ttc_parse_ota(uint8_t *buf, uint8_t len);

static void on_ttc_tx_done(int result)
{
    // printf("result %d \n", result);
    waiting_tx = 0U;
    ttc_read_async();
}

static void on_ota_drain_done(int result)
{
    if (result == 1)
    {
        printf("[OTA] First packet drained (%d bytes discarded) — ready for full packets\n",
               OTA_FULL_PACKET - TTC_CMD_LEN);
    }
    else
    {
        printf("[OTA] Drain failed — continuing anyway\n");
    }

    /* Agora sim, pede o próximo pacote completo de 134 bytes */
    if (!waiting_tx)
        ttc_read_async();
}

static void on_ttc_done(int result)
{
    //printf("result ttc %d", result);
    if (result == 1)
    {
        if (ota_data_started)
        {
            ttc_parse_ota(rx_buf, OTA_FULL_PACKET);
        }
        else
        {
            ttc_parse(rx_buf);
        }
    }

    if (!waiting_tx)
        ttc_read_async();
}
static void ttc_parse(uint8_t *buf)
{
    //printf("parse: 0x%02X 0x%02X 0x%02X 0x%02X\n", buf[0], buf[1], buf[2], buf[3]);

    ground_command_t cmd = (ground_command_t)buf[0];
    ttc.last_command = cmd;

    switch (cmd)
    {
    case CMD_REQUEST_DATA:
        //printf("cmd0\n");
        ttc.doppler = (float)buf[1] / 10.0f;
        ttc_send_telemetry();
        break;

    case CMD_ENTER_SAFE:
        printf("cmd1\n");
        ttc.cmd_status = ACK_SUCCESS;
        break;

    case CMD_START_OTA:
        printf("[TTC] START_OTA received — sending ACK\n");
        ttc.ota_active = true;
        ttc.cmd_status = ACK_SUCCESS;
        ota_receiving = 1U;
        ota_data_started = 1U;

        ota_ack_buf[0] = 0x10;
        ota_ack_buf[1] = 0xAC;
        ota_ack_buf[2] = 0x4B;
        ota_ack_buf[3] = 0x00;
        waiting_tx = 1U;
        usart_send_async(&usart, ota_ack_buf, TTC_CMD_LEN);
        break;

    case CMD_END_OTA:
        printf("cmd3 — OTA mode OFF\n");
        ttc.ota_active = false;
        ttc.cmd_status = ACK_SUCCESS;
        ota_receiving = 0U;
        break;

    case CMD_REMOTE_CTRL:
        printf("cmd5\n");
        ttc.cmd_status = ACK_SUCCESS;
        break;

    case CMD_NONE:
        break;
    default:
        ttc.cmd_status = ACK_FAILED;
        break;
    }
}
static void ttc_parse_ota(uint8_t *buf, uint8_t len)
{
    uint16_t sync = ((uint16_t)buf[0] << 8) | buf[1];
    if (sync != OTA_SYNC_WORD)
    {
        return;
    }

    uint16_t seq         = ((uint16_t)buf[2] << 8) | buf[3];
    uint16_t payload_len = ((uint16_t)buf[4] << 8) | buf[5];

    if (seq == 0xFFFF)
    {
        /* Marcador de fim: payload contém [4B size][4B crc32][4B version] */
        ota_receiving    = 0U;
        ota_data_started = 0U;
        ttc.ota_active   = false;
        ttc.cmd_status   = ACK_SUCCESS;

        /* Copia END payload para o buffer de staging (otaMode() vai ler) */
        s_ota_pkt_seq   = 0xFFFFU;
        s_ota_pkt_len   = (payload_len < OTA_PACKET_SIZE) ? payload_len
                                                           : OTA_PACKET_SIZE;
        for (uint16_t i = 0U; i < s_ota_pkt_len; i++)
        {
            s_ota_pkt_buf[i] = buf[OTA_HEADER_SIZE + i];
        }
        s_ota_pkt_ready = 1U;
        return;
    }

    /* Pacote de dados normal: copia payload para buffer de staging */
    if (!s_ota_pkt_ready)   /* Não sobrepõe pacote que ainda não foi consumido */
    {
        s_ota_pkt_seq = seq;
        s_ota_pkt_len = (payload_len < OTA_PACKET_SIZE) ? payload_len
                                                         : OTA_PACKET_SIZE;
        for (uint16_t i = 0U; i < s_ota_pkt_len; i++)
        {
            s_ota_pkt_buf[i] = buf[OTA_HEADER_SIZE + i];
        }
        s_ota_pkt_ready = 1U;
    }

    ttc.cmd_status = ACK_SUCCESS;
}

/* =========================================================================
 * ttc_ota_* — API pública de acesso ao staging buffer
 * ========================================================================= */

uint8_t ttc_ota_packet_ready(void)
{
    return s_ota_pkt_ready;
}

uint16_t ttc_ota_get_seq(void)
{
    return s_ota_pkt_seq;
}

const uint8_t *ttc_ota_get_payload(void)
{
    return s_ota_pkt_buf;
}

uint16_t ttc_ota_get_payload_len(void)
{
    return s_ota_pkt_len;
}

void ttc_ota_clear_ready(void)
{
    s_ota_pkt_ready = 0U;
}

/**
 * @brief Inicia receção assíncrona de um comando da Ground Station.
 */
void ttc_read_async(void)
{
    if (usart.rx_state != UART_RX_IDLE)
        return;

    uint8_t expected_len = ota_data_started ? OTA_FULL_PACKET : TTC_CMD_LEN;
    usart_recv_async(&usart, rx_buf, expected_len, on_ttc_done);
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

    if (waiting_tx && usart.tx_state == UART_TX_IDLE && usart.tx_index >= usart.tx_len && usart.tx_len > 0)
    {
        // printf("[TTC TX] ACK sent (%d bytes)\n", usart.tx_len);
        waiting_tx = 0U;
        ttc_read_async();
    }
}

/**
 * @brief Envia frame de telemetria para a Ground Station.
 */
void ttc_send_telemetry(void)
{
    uint8_t i = 0U;

    tx_buf[i++] = CMD_REQUEST_DATA;
    tx_buf[i++] = (uint8_t)(eps.voltage * 10.0f);
    tx_buf[i++] = (uint8_t)(eps.current * 100.0f);
    tx_buf[i++] = (uint8_t)gnss.latitude;
    tx_buf[i++] = (uint8_t)((gnss.latitude - (uint8_t)gnss.latitude) * 100.0f);
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