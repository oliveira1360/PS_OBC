#include "hal/hal_usart.h"
#include "config/board.h"
#include <stdlib.h>  /* para simulacao apenas !!! */
#include <time.h>    /* para simulacao apenas !!! */

/* simulacao — buffer de recepcao com dados ficticios do ground station */
typedef struct
{
    uint8_t data[TTC_BUF_LEN];
    uint8_t len;
} usart_sim_t;

static usart_sim_t sim = {
    {0x20, 0x01, 0x00, 0x00},  /* 0x20 = CMD_REQUEST_DATA */
    4U
};

static uint8_t rx_index  = 0;
static uint8_t tx_ready  = 1;
static uint8_t rx_ready  = 0;

static void hal_usart_randomize(void)
{
    sim.data[0] = 0x20U;
    sim.data[1] = (uint8_t)(rand() % 256U);
    sim.data[2] = (uint8_t)(rand() % 256U);
    sim.data[3] = (uint8_t)(rand() % 256U);
    rx_index    = 0;
    rx_ready    = 1U;
}

uint8_t hal_usart_init(void)
{
    hal_usart_randomize();
    return 1U;
}

uint8_t hal_usart_is_tx_ready(void)
{
    return tx_ready;  /* simulacao: sempre pronto para enviar */
}

void hal_usart_write_char(uint8_t data)
{
    (void)data;  /* simulacao: descarta o byte enviado */
    tx_ready = 1U;
}

uint8_t hal_usart_data_available(void)
{
    return rx_ready;
}

uint8_t hal_usart_read_char(void)
{
    uint8_t byte = 0x00U;

    if (rx_index < sim.len)
        byte = sim.data[rx_index++];

    if (rx_index >= sim.len)
        rx_ready = 0U;  /* ← sem randomize aqui */

    return byte;
}

void hal_usart_prepare_rx(void)
{
    hal_usart_randomize();
}