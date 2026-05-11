#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

#define PROP_SPI spi0
#define PROP_SCK_PIN 2
#define PROP_MISO_PIN 3
#define PROP_MOSI_PIN 4
#define PROP_CS_PIN 5
#define PROP_FRAME_LEN 9

static uint8_t prop_tx_frame[PROP_FRAME_LEN];
static uint8_t prop_valve_state = 0U;
static uint16_t prop_thrust_setpoint = 0U;

static void prop_generate_frame(void)
{
    uint8_t *f = prop_tx_frame;
    f[0] = prop_valve_state ? 0x01U : 0x00U;
    uint16_t press = 150U + (uint16_t)(rand() % 101U);
    f[1] = (uint8_t)(press >> 8U);
    f[2] = (uint8_t)(press & 0xFFU);
    uint16_t temp = 220U + (uint16_t)(rand() % 61U);
    f[3] = (uint8_t)(temp >> 8U);
    f[4] = (uint8_t)(temp & 0xFFU);
    f[5] = 0x00U;
    f[6] = 0x00U;
    f[7] = prop_valve_state;
    uint8_t chk = 0U;
    for (uint8_t j = 0U; j < 8U; j++)
        chk ^= f[j];
    f[8] = chk;
}

int main(void)
{
    stdio_init_all();
    sleep_ms(2000);
    srand(12345);

    spi_init(PROP_SPI, 1000000);
    spi_set_slave(PROP_SPI, true);
    spi_set_format(PROP_SPI, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(PROP_MISO_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PROP_MOSI_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PROP_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PROP_CS_PIN, GPIO_FUNC_SPI);

    spi_hw_t *hw = spi_get_hw(PROP_SPI);

    printf("[PICO2] Ready\n");

    while (true)
    {
        /* === Transação 1: recebe comando (1 byte) === */
        uint8_t cmd;
        uint8_t dummy_tx = 0x00U;
        spi_write_read_blocking(PROP_SPI, &dummy_tx, &cmd, 1);

        printf("[PICO2] CMD=0x%02X\n", cmd);

        /* Processa comando */
        if (cmd == 0x02)
        {
            prop_valve_state = 1U;
            prop_thrust_setpoint = 100U;
        }
        if (cmd == 0x03)
        {
            prop_valve_state = 0U;
            prop_thrust_setpoint = 0U;
        }

        /* Gera frame de resposta */
        prop_generate_frame();

        /* Limpa FIFOs */
        while (hw->sr & 0x04)
            (void)hw->dr;

        /* Pré-carrega o frame no TX FIFO (8 bytes max) */
        for (uint8_t i = 0; i < 8 && i < PROP_FRAME_LEN; i++)
            hw->dr = prop_tx_frame[i];

        /* === Transação 2: envia frame (9 bytes) === */
        uint8_t rx_dummy[PROP_FRAME_LEN];
        uint8_t tx_idx = 8;

        for (uint8_t i = 0; i < PROP_FRAME_LEN; i++)
        {
            while (!(hw->sr & 0x04))
            {
                if (tx_idx < PROP_FRAME_LEN && (hw->sr & 0x02))
                    hw->dr = prop_tx_frame[tx_idx++];
                tight_loop_contents();
            }
            if (tx_idx < PROP_FRAME_LEN && (hw->sr & 0x02))
                hw->dr = prop_tx_frame[tx_idx++];
            rx_dummy[i] = (uint8_t)hw->dr;
        }

        printf("[PICO2] Sent frame\n");
    }
    uint8_t led_state = 0;

    while (true)
    {
        /* Gera frame */
        prop_generate_frame();

        /* Limpa FIFOs */
        while (hw->sr & 0x04)
            (void)hw->dr;

        /* Pré-carrega EXATAMENTE 8 bytes no TX FIFO */
        hw->dr = 0x00U; /* dummy byte 0 */
        for (uint8_t i = 0; i < 8; i++)
            hw->dr = prop_tx_frame[i];

        /* Espera bloqueante: lê 8 bytes do master */
        uint8_t rx_buf[8];
        uint8_t tx_idx = 7;

        for (uint8_t i = 0; i < 8; i++)
        {
            while (!(hw->sr & 0x04))
                tight_loop_contents();
            rx_buf[i] = (uint8_t)hw->dr;
        }

        /* Processa comando */
        uint8_t cmd = rx_buf[0];
        if (cmd == 0x02)
        {
            prop_valve_state = 1U;
            prop_thrust_setpoint = 100U;
        }
        if (cmd == 0x03)
        {
            prop_valve_state = 0U;
            prop_thrust_setpoint = 0U;
        }

        led_state = !led_state;
        gpio_put(25, led_state);
    }
    return 0;
}