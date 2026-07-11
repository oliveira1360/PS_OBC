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
#include "app/mission.h"

#define TTC_SYNC_A 0xAAU
#define TTC_SYNC_B 0x55U
#define TTC_SYNC_TIMEOUT_BYTES 32U

static usart_handle_t usart = {0};
static uint8_t rx_buf[OTA_FULL_PACKET];
static uint8_t tx_buf[TTC_BUF_LEN];
static uint8_t ota_ack_buf[TTC_CMD_LEN];

/* Códigos do protocolo OTA */
#define OTA_ACK_CODE   0xACU   /* pacote recebido com sucesso  */
#define OTA_NACK_CODE  0x4EU   /* sync inválido — retransmitir */
#define OTA_READY_CODE 0xE0U   /* flash apagada — pronto para receber */

static uint8_t waiting_tx = 0U;
static uint8_t ota_receiving = 0U;
static uint8_t ota_data_started = 0U; /* primeiro pacote OTA real recebido */
static uint8_t ota_partial = 0U;      /* bytes já recebidos do primeiro pacote OTA */

typedef enum
{
    SYNC_WAIT_A1 = 0, /* à espera de 0xAA (1º byte) */
    SYNC_WAIT_B1,     /* à espera de 0x55 (2º byte) */
    SYNC_WAIT_A2,     /* à espera de 0xAA (3º byte) */
    SYNC_WAIT_B2,     /* à espera de 0x55 (4º byte) */
    SYNC_DONE         /* sincronizado — modo normal  */
} ttc_sync_state_t;

typedef enum
{
    OTA_FRAME_HUNT_AA = 0,
    OTA_FRAME_HUNT_55,
    OTA_FRAME_FOUND,
} ota_frame_t;

/* With passthrough Pico, no sync sequence is sent at boot — start in SYNC_DONE */
static ttc_sync_state_t sync_state = SYNC_DONE;
static uint8_t sync_rx_byte = 0U;
static uint16_t sync_byte_cnt = 0U; /* bytes recebidos sem sync */
static const uint8_t sync_ready[4] = {TTC_SYNC_B, TTC_SYNC_A,
                                      TTC_SYNC_B, TTC_SYNC_A};
static ota_frame_t ota_frame_state = OTA_FRAME_HUNT_AA;
static uint8_t ota_frame_byte = 0U;

/* =========================================================================
 * Buffer de staging de pacotes OTA
 * Acedido por otaMode() via ttc_ota_* API.
 * ========================================================================= */
static uint8_t s_ota_pkt_ready = 0U;
static uint16_t s_ota_pkt_seq = 0U;
static uint16_t s_ota_pkt_len = 0U;
static uint8_t s_ota_pkt_buf[OTA_PACKET_SIZE];

static void ttc_parse(uint8_t *buf);
void ttc_read_async(void);
static void ttc_parse_ota(uint8_t *buf, uint8_t len);
static void on_ttc_sync_byte(int result);
static void on_ota_frame_byte(int result);
static void on_ttc_done(int result);
static void ttc_send_ota_ack(uint16_t seq);
static void ttc_send_ota_nack(uint16_t next_seq);
void        ttc_send_ota_ready(void);

/* =========================================================================
 * Callback de sincronização — chamado após receber 1 byte em modo sync
 * ========================================================================= */
static void on_ttc_sync_byte(int result)
{
    if (result == 1)
    {
        sync_byte_cnt++;

        if (sync_byte_cnt >= TTC_SYNC_TIMEOUT_BYTES)
        {
            sync_state = SYNC_DONE;
            printf("[TTC] Sync timeout (%u bytes) — modo normal directo\n",
                   (unsigned)sync_byte_cnt);
            ttc_read_async(); /* começa a ler 4 bytes normais */
            return;
        }

        switch (sync_state)
        {
        case SYNC_WAIT_A1:
            sync_state = (sync_rx_byte == TTC_SYNC_A) ? SYNC_WAIT_B1 : SYNC_WAIT_A1;
            break;
        case SYNC_WAIT_B1:
            sync_state = (sync_rx_byte == TTC_SYNC_B) ? SYNC_WAIT_A2 : SYNC_WAIT_A1;
            break;
        case SYNC_WAIT_A2:
            sync_state = (sync_rx_byte == TTC_SYNC_A) ? SYNC_WAIT_B2 : SYNC_WAIT_A1;
            break;
        case SYNC_WAIT_B2:
            if (sync_rx_byte == TTC_SYNC_B)
            {
                sync_state = SYNC_DONE;
                printf("[TTC] Sync OK — a enviar READY\n");
                waiting_tx = 1U;
                usart_send_async(&usart, (uint8_t *)sync_ready, sizeof(sync_ready));
                return;
            }
            sync_state = SYNC_WAIT_A1;
            break;
        default:
            sync_state = SYNC_WAIT_A1;
            break;
        }
    }
    /* Ainda não sincronizado — continua a ler 1 byte de cada vez */
    ttc_read_async();
}

/* =========================================================================
 * on_ota_frame_byte — scans for [0xAA][0x55] before each OTA packet
 * ========================================================================= */
static void on_ota_frame_byte(int result)
{
    if (result == 1)
    {
        switch (ota_frame_state)
        {
        case OTA_FRAME_HUNT_AA:
            if (ota_frame_byte == 0xAAU)
                ota_frame_state = OTA_FRAME_HUNT_55;
            break;

        case OTA_FRAME_HUNT_55:
            if (ota_frame_byte == 0x55U)
            {
                /* Encontrou [0xAA][0x55] — lê os 132 bytes restantes via
                 * circular buffer (RXRDY interrupt). Sem conflito com DMA.  */
                ota_frame_state = OTA_FRAME_FOUND;
                rx_buf[0] = 0xAAU;
                rx_buf[1] = 0x55U;
                usart_recv_async(&usart, rx_buf + 2U,
                                 (uint8_t)(OTA_FULL_PACKET - 2U), on_ttc_done);
                return;
            }
            /* 0xAA may start a new sequence */
            ota_frame_state = (ota_frame_byte == 0xAAU)
                                  ? OTA_FRAME_HUNT_55
                                  : OTA_FRAME_HUNT_AA;
            break;

        default:
            ota_frame_state = OTA_FRAME_HUNT_AA;
            break;
        }
    }
    if (!waiting_tx)
        ttc_read_async();
}

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
    if (result == 1)
    {
        if (ota_data_started)
        {
            ttc_parse_ota(rx_buf, OTA_FULL_PACKET);
            /* Reset frame scanner so the next packet is hunted fresh */
            ota_frame_state = OTA_FRAME_HUNT_AA;
        }
        else
        {
            printf("result ttc %d", result);
            ttc_parse(rx_buf);
        }
    }

    if (!waiting_tx)
        ttc_read_async();
}
static void ttc_parse(uint8_t *buf)
{
    printf("parse: 0x%02X 0x%02X 0x%02X 0x%02X\n", buf[0], buf[1], buf[2], buf[3]);

    ground_command_t cmd = (ground_command_t)buf[0];

    switch (cmd)
    {
    case CMD_REQUEST_DATA:
        // printf("cmd0\n");
        COMM_WINDOW_OPEN = 1;
        ttc.doppler = (float)buf[1] / 10.0f;
        ttc_send_telemetry();
        break;

    case CMD_START_OTA:
        /* Reset completo do estado OTA (limpa sessão anterior).
         * Inclui a sub-FSM de modes.c: sem isto, uma tentativa anterior
         * interrompida a meio (timeout/abort do backend) deixava s_state
         * preso (ex: OTA_SM_WAIT_PKT), fazendo com que este novo START
         * fosse ignorado pela FSM e o erase + READY nunca mais fossem
         * repetidos. */
        ttc_ota_abort();
        ota_fsm_reset();
        s_ota_pkt_seq = 0U;
        printf("[TTC] START_OTA recebido — a preparar memoria...\n");
        COMM_WINDOW_OPEN = 1;
        OTA_REQUESTED = 1;
        ttc.ota_active = true;
        ttc.cmd_status = ACK_SUCCESS;
        break;

    case CMD_END_OTA:
        printf("cmd3 — OTA mode OFF\n");
        OTA_REQUESTED = 0;
        ttc.ota_active = false;
        ttc.cmd_status = ACK_SUCCESS;
        ota_receiving = 0U;
        ota_data_started = 0U;
        ota_frame_state = OTA_FRAME_HUNT_AA;
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
        printf("[OTA RX] sync invalido: 0x%04X (esperado 0x%04X)\n",
               sync, OTA_SYNC_WORD);
        /* NACK — backend deve retransmitir o mesmo pacote */
        ttc_send_ota_nack(s_ota_pkt_seq + 1U);
        return;
    }

    uint16_t seq = ((uint16_t)buf[2] << 8) | buf[3];
    uint16_t payload_len = ((uint16_t)buf[4] << 8) | buf[5];

    if (seq == 0xFFFFU)
    {
        /* Marcador de fim: payload contém [4B size][4B crc32] */
        uint32_t fw_size = ((uint32_t)buf[OTA_HEADER_SIZE + 0] << 24) |
                           ((uint32_t)buf[OTA_HEADER_SIZE + 1] << 16) |
                           ((uint32_t)buf[OTA_HEADER_SIZE + 2] << 8) |
                           (uint32_t)buf[OTA_HEADER_SIZE + 3];
        uint32_t fw_crc = ((uint32_t)buf[OTA_HEADER_SIZE + 4] << 24) |
                          ((uint32_t)buf[OTA_HEADER_SIZE + 5] << 16) |
                          ((uint32_t)buf[OTA_HEADER_SIZE + 6] << 8) |
                          (uint32_t)buf[OTA_HEADER_SIZE + 7];
        printf("[OTA RX] END marker — size=%lu crc=0x%08lX\n",
               (unsigned long)fw_size, (unsigned long)fw_crc);

        ota_receiving = 0U;
        ota_data_started = 0U;
        ttc.ota_active = false;
        ttc.cmd_status = ACK_SUCCESS;

        s_ota_pkt_seq = 0xFFFFU;
        s_ota_pkt_len = (payload_len < OTA_PACKET_SIZE) ? payload_len : OTA_PACKET_SIZE;
        for (uint16_t i = 0U; i < s_ota_pkt_len; i++)
            s_ota_pkt_buf[i] = buf[OTA_HEADER_SIZE + i];
        s_ota_pkt_ready = 1U;

        /* ACK do pacote END */
        ttc_send_ota_ack(seq);
        return;
    }

    /* --- Pacote de dados normal --- */
    uint16_t cur_len = (payload_len < OTA_PACKET_SIZE) ? payload_len : OTA_PACKET_SIZE;

    printf("[OTA RX] pkt seq=%u len=%u ready=%u\n", seq, cur_len, s_ota_pkt_ready);

    if (!s_ota_pkt_ready) 
    {
        s_ota_pkt_seq = seq;
        s_ota_pkt_len = cur_len;
        for (uint16_t i = 0U; i < cur_len; i++)
            s_ota_pkt_buf[i] = buf[OTA_HEADER_SIZE + i];
        s_ota_pkt_ready = 1U;
        
        /* ONLY send ACK if we actually accepted the packet into the buffer */
        ttc_send_ota_ack(seq);
    }
    else
    {
        /* Buffer is full (Flash is still writing). Do not send ACK. 
         * The backend will timeout and retry this sequence later. */
        printf("[OTA RX] Drop seq=%u (Flash busy)\n", seq);
    }

    ttc.cmd_status = ACK_SUCCESS;

    /* ACK — backend só envia o próximo chunk após receber este */
}

/* =========================================================================
 * ttc_send_ota_ack — envia ACK [0xAC][seq_hi][seq_lo][checksum]
 * ========================================================================= */
static void ttc_send_ota_ack(uint16_t seq)
{
    ota_ack_buf[0] = OTA_ACK_CODE;
    ota_ack_buf[1] = (uint8_t)(seq >> 8U);
    ota_ack_buf[2] = (uint8_t)(seq & 0xFFU);
    ota_ack_buf[3] = ota_ack_buf[0] ^ ota_ack_buf[1] ^ ota_ack_buf[2];
    waiting_tx = 1U;
    usart_send_async(&usart, ota_ack_buf, TTC_CMD_LEN);
}

/* =========================================================================
 * ttc_send_ota_nack — envia NACK [0x4E][next_hi][next_lo][checksum]
 *
 * @param next_seq  Seq do próximo pacote esperado (backend retransmite a partir daí).
 * ========================================================================= */
static void ttc_send_ota_nack(uint16_t next_seq)
{
    ota_ack_buf[0] = OTA_NACK_CODE;
    ota_ack_buf[1] = (uint8_t)(next_seq >> 8U);
    ota_ack_buf[2] = (uint8_t)(next_seq & 0xFFU);
    ota_ack_buf[3] = ota_ack_buf[0] ^ ota_ack_buf[1] ^ ota_ack_buf[2];
    waiting_tx = 1U;
    usart_send_async(&usart, ota_ack_buf, TTC_CMD_LEN);
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
 * @brief Repõe o estado OTA do TTC após um abort.
 *        Deve ser chamado quando otaMode() entra em erro,
 *        para que o próximo CMD_START_OTA seja processado
 *        como comando e não engolido pelo frame scanner.
 */
void ttc_ota_abort(void)
{
    ota_receiving = 0U;
    ota_data_started = 0U;
    ota_frame_state = OTA_FRAME_HUNT_AA;
    s_ota_pkt_ready = 0U;
}

/**
 * @brief Inicia receção assíncrona de um comando da Ground Station.
 *
 * Em modo sync (arranque): lê 1 byte de cada vez via on_ttc_sync_byte
 * até detectar o handshake do Pico.
 * Em modo normal: lê TTC_CMD_LEN ou OTA_FULL_PACKET bytes via on_ttc_done.
 */
void ttc_read_async(void)
{
    if (usart.rx_state != UART_RX_IDLE)
        return;

    if (sync_state != SYNC_DONE)
    {
        /* Modo sync — lê 1 byte de cada vez para detectar a sequência */
        usart_recv_async(&usart, &sync_rx_byte, 1U, on_ttc_sync_byte);
        return;
    }

    if (ota_data_started && ota_frame_state != OTA_FRAME_FOUND)
    {
        /* Scan byte-by-byte for [0xAA][0x55] frame marker */
        usart_recv_async(&usart, &ota_frame_byte, 1U, on_ota_frame_byte);
        return;
    }

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
/* Escreve um float em big-endian (o SAMV71 e little-endian, por isso inverte). */
static void ttc_put_float_be(uint8_t *p, float v)
{
    union { float f; uint8_t b[4]; } u;
    u.f = v;
    p[0] = u.b[3];
    p[1] = u.b[2];
    p[2] = u.b[1];
    p[3] = u.b[0];
}

/*
 * Frame de telemetria alargado (75 bytes):
 *   [0]      0x20  marcador (CMD_REQUEST_DATA)
 *   [1..72]  18 floats big-endian, por esta ordem:
 *            voltage, current, latitude, longitude, altitude, speed,
 *            ax, ay, az, gx, gy, gz, mx, my, mz, pressure, temperature, doppler
 *   [73]     estado do TT&C
 *   [74]     checksum = XOR dos bytes [0..73]
 */
void ttc_send_telemetry(void)
{
    uint8_t i = 0U;

    tx_buf[i++] = CMD_REQUEST_DATA;

    ttc_put_float_be(&tx_buf[i], eps.voltage);             i += 4U;
    ttc_put_float_be(&tx_buf[i], eps.current);             i += 4U;
    ttc_put_float_be(&tx_buf[i], gnss.latitude);           i += 4U;
    ttc_put_float_be(&tx_buf[i], gnss.longitude);          i += 4U;
    ttc_put_float_be(&tx_buf[i], gnss.altitude);           i += 4U;
    ttc_put_float_be(&tx_buf[i], gnss.speed);              i += 4U;
    ttc_put_float_be(&tx_buf[i], imu.ax);                  i += 4U;
    ttc_put_float_be(&tx_buf[i], imu.ay);                  i += 4U;
    ttc_put_float_be(&tx_buf[i], imu.az);                  i += 4U;
    ttc_put_float_be(&tx_buf[i], imu.gx);                  i += 4U;
    ttc_put_float_be(&tx_buf[i], imu.gy);                  i += 4U;
    ttc_put_float_be(&tx_buf[i], imu.gz);                  i += 4U;
    ttc_put_float_be(&tx_buf[i], imu.mx);                  i += 4U;
    ttc_put_float_be(&tx_buf[i], imu.my);                  i += 4U;
    ttc_put_float_be(&tx_buf[i], imu.mz);                  i += 4U;
    ttc_put_float_be(&tx_buf[i], pressure.pressure);       i += 4U;
    ttc_put_float_be(&tx_buf[i], temperature.temperature); i += 4U;
    ttc_put_float_be(&tx_buf[i], ttc.doppler);             i += 4U;

    tx_buf[i++] = ttc.current_state;

    uint8_t chk = 0U;
    for (uint8_t j = 0U; j < i; j++)
        chk ^= tx_buf[j];
    tx_buf[i++] = chk;

    waiting_tx = 1U;
    usart_send_async(&usart, tx_buf, i);
}



void ttc_send_ota_ready(void)
{
    /* Cancela qualquer receive pendente (ex: receive de 4B do modo normal
     * que ainda estava activo durante o erase). Sem isto o primeiro chunk
     * seria consumido pelo receive errado antes do frame scanner arrancar. */
    usart.rx_state = UART_RX_IDLE;

    ota_receiving    = 1U;
    ota_data_started = 1U;
    ota_frame_state  = OTA_FRAME_HUNT_AA;

    ota_ack_buf[0] = 0x10;
    ota_ack_buf[1] = 0xAC;
    ota_ack_buf[2] = 0x4B;
    ota_ack_buf[3] = 0xF7;
    waiting_tx = 1U;
    usart_send_async(&usart, ota_ack_buf, TTC_CMD_LEN);
}