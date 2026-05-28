/**
 * @file propulsor.c
 * @brief Periférico do subsistema de propulsão (cold-gas thruster).
 *
 * Responsabilidades:
 *   - Iniciar leituras assíncronas via SPI driver
 *   - Fazer parse dos dados brutos recebidos
 *   - Preencher a struct propulsor_data_t global
 *
 * NÃO acede ao HAL diretamente — usa apenas o spi_driver.
 */

#include <stdio.h>
#include "peripherals/propulsor.h"
#include "drivers/spi_driver.h"
#include "config/board.h"
#include "app/sensors.h"

/** @brief Handle de controlo SPI. */
static spi_handle_t spi = {0};

/** @brief Buffer TX: comando + dummies. */
static uint8_t tx_buf[1 + PROPULSOR_BUF_LEN];

/** @brief Buffer RX: 1 dummy + 9 bytes de telemetria. */
static uint8_t rx_buf[1 + PROPULSOR_BUF_LEN];

/* ==========================================================================
 * PARSING
 * ========================================================================== */

/**
 * @brief Verifica o checksum XOR do frame.
 * @param data 9 bytes do frame.
 * @return 1 se válido, 0 se inválido.
 */
static uint8_t propulsor_verify_checksum(const uint8_t *data)
{
    uint8_t chk = 0U;
    for (uint8_t i = 0U; i < 8U; i++)
        chk ^= data[i];
    return (chk == data[8]) ? 1U : 0U;
}

/**
 * @brief Analisa os dados brutos e preenche a struct global.
 *
 * Frame (9 bytes):
 *   [0]   STATUS:   0x00=IDLE, 0x01=FIRING, 0x02=ERROR
 *   [1-2] PRESSURE: raw /100 = bar
 *   [3-4] TEMP:     raw /10  = °C
 *   [5-6] THRUST:   raw /10  = N
 *   [7]   VALVE:    0=fechada, 1=aberta
 *   [8]   CHECKSUM: XOR [0..7]
 */
static void propulsor_parse(const uint8_t *data)
{
    if (!propulsor_verify_checksum(data))
    {
        // printf("[PROP] Checksum error\n");
        return;
    }

    propulsor.status = data[0];
    propulsor.pressure = (float)((data[1] << 8) | data[2]) / 100.0f;
    propulsor.temp = (float)((data[3] << 8) | data[4]) / 10.0f;
    propulsor.thrust = (float)((data[5] << 8) | data[6]) / 10.0f;
    propulsor.valve = data[7];
}

/* ==========================================================================
 * CALLBACK
 * ========================================================================== */

/**
 * @brief Callback executado após conclusão da transferência SPI.
 *
 * rx_buf[0] é dummy — dados úteis começam em rx_buf[1].
 *
 * @param result 0=sucesso, -1=erro/timeout.
 */
static void on_propulsor_done(int result)
{
    //printf("[PROP] result=%d RX: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n", result, rx_buf[0], rx_buf[1], rx_buf[2], rx_buf[3], rx_buf[4], rx_buf[5], rx_buf[6], rx_buf[7], rx_buf[8], rx_buf[9]);

    if (result == 0)
        propulsor_parse(&rx_buf[1]);
}

/* ==========================================================================
 * API PÚBLICA
 * ========================================================================== */

/**
 * @brief Inicia leitura assíncrona de telemetria do propulsor.
 */
void propulsor_read_async(void)
{
    if (spi.state != SPI_IDLE)
        return;

    tx_buf[0] = 0x01U;
    for (uint8_t i = 1U; i < sizeof(tx_buf); i++)
        tx_buf[i] = 0x00U; /* envia 0x00 em vez de 0xFF como dummy */

    spi_transfer_async(&spi, PROPULSOR_CS_PIN,
                       tx_buf, rx_buf,
                       1U + PROPULSOR_BUF_LEN,
                       on_propulsor_done);
}

/**
 * @brief Envia comando para abrir a válvula.
 */
void propulsor_valve_open(void)
{
    if (spi.state != SPI_IDLE)
        return;

    tx_buf[0] = 0x02U; /* CMD_VALVE_OPEN */
    for (uint8_t i = 1U; i < sizeof(tx_buf); i++)
        tx_buf[i] = 0xFFU;

    spi_transfer_async(&spi, PROPULSOR_CS_PIN,
                       tx_buf, rx_buf,
                       1U + PROPULSOR_BUF_LEN,
                       on_propulsor_done);
}

/**
 * @brief Envia comando para fechar a válvula.
 */
void propulsor_valve_close(void)
{
    if (spi.state != SPI_IDLE)
        return;

    tx_buf[0] = 0x03U; /* CMD_VALVE_CLOSE */
    for (uint8_t i = 1U; i < sizeof(tx_buf); i++)
        tx_buf[i] = 0xFFU;

    spi_transfer_async(&spi, PROPULSOR_CS_PIN,
                       tx_buf, rx_buf,
                       1U + PROPULSOR_BUF_LEN,
                       on_propulsor_done);
}

/**
 * @brief Envia comando para definir o thrust setpoint.
 * @param thrust_raw Valor raw (raw/10 = Newtons).
 */
void propulsor_set_thrust(uint16_t thrust_raw)
{
    if (spi.state != SPI_IDLE)
        return;

    tx_buf[0] = 0x04U; /* CMD_SET_THRUST */
    tx_buf[1] = (uint8_t)(thrust_raw >> 8);
    tx_buf[2] = (uint8_t)(thrust_raw & 0xFFU);
    for (uint8_t i = 3U; i < sizeof(tx_buf); i++)
        tx_buf[i] = 0xFFU;

    spi_transfer_async(&spi, PROPULSOR_CS_PIN,
                       tx_buf, rx_buf,
                       1U + PROPULSOR_BUF_LEN,
                       on_propulsor_done);
}

/**
 * @brief Tick da FSM SPI — chamar no super-loop.
 */
void propulsor_tick(void)
{
    spi_tick(&spi);
}