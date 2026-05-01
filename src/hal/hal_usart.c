/**
 * @file hal_usart.c
 * @brief Simulação da Hardware Abstraction Layer (HAL) para o periférico USART.
 *
 * Este módulo não interage com hardware real. Em vez disso, simula o 
 * comportamento de uma porta série bidirecional, gerando pacotes de dados 
 * fictícios (comandos da "Ground Station") para efeitos de teste do 
 * subsistema de Telemetria, Rastreio e Comando (TT&C).
 */

#include "hal/hal_usart.h"
#include "config/board.h"
#include <stdlib.h>  /* para simulacao apenas !!! */
#include <time.h>    /* para simulacao apenas !!! */

/**
 * @brief Estrutura do buffer de receção simulado.
 *
 * Contém os dados fictícios recebidos via USART e o respetivo comprimento.
 */
typedef struct
{
    uint8_t data[TTC_BUF_LEN]; /**< Array de dados recebidos */
    uint8_t len;               /**< Comprimento total da mensagem */
} usart_sim_t;

/**
 * @brief Variável estática que armazena o estado do pacote simulado atual.
 * Inicializada com um pacote padrão que simula um comando `CMD_REQUEST_DATA` (0x20).
 */
static usart_sim_t sim = {
    {0x20, 0x01, 0x00, 0x00},  /* 0x20 = CMD_REQUEST_DATA */
    4U
};

/**
 * @brief Índice de leitura atual do buffer de receção simulado.
 */
static uint8_t rx_index  = 0;

/**
 * @brief Flag que indica se o bus de escrita esta livre.
 *
 */
static uint8_t tx_ready  = 1;

/**
 * @brief Flag que indica se o bus de leitura esta livre.
 */
static uint8_t rx_ready  = 0;

/**
 * @brief Controla se o RX auto-regenera dados ao esgotar o buffer.
 * 1 = comportamento normal (dados contínuos da Ground Station).
 * 0 = desativado — útil para testar timeout de receção.
 */
static uint8_t rx_auto_regen = 1;

/**
 * @brief Gera pacotes de comandos simulados da Ground Station (GS) com
 *        distribuição de probabilidade realista para uma missão CubeSat.
 *
 * Distribuição simulada (por 100 uplinks):
 *   85% CMD_REQUEST_DATA (0x20) — telemetria periódica normal
 *    8% CMD_ENTER_SAFE   (0x01) — operador entra em safe mode
 *    4% CMD_START_OTA    (0x10) — início de atualização OTA
 *    2% CMD_REMOTE_CTRL  (0x02) — controlo remoto manual
 *    1% CMD_END_OTA      (0x11) — fim/confirmação de OTA
 *
 * Formato do frame de 4 bytes por comando:
 *
 *  CMD_REQUEST_DATA : [0x20][doppler_x10 kHz][timestamp_hi][timestamp_lo]
 *    doppler_x10: efeito Doppler em décimas de kHz (5–35 → 0.5–3.5 kHz)
 *    timestamp  : contador de ciclos de telemetria (big-endian 16-bit)
 *
 *  CMD_ENTER_SAFE   : [0x01][0x00][0x00][0x00]
 *
 *  CMD_START_OTA    : [0x10][ver_major][ver_minor][0x00]
 *    ver_major/minor: versão do firmware proposto (ex: 1.3)
 *
 *  CMD_REMOTE_CTRL  : [0x02][0x00][0x00][0x00]
 *
 *  CMD_END_OTA      : [0x11][0x00][0x00][0x00]
 */
static void hal_usart_randomize(void)
{
    static uint16_t tlm_timestamp = 0U; /* contador de ciclos de telemetria */
    int roll = rand() % 100;

    if (roll < 85)
    {
        /* ---- CMD_REQUEST_DATA (85%) ---- */
        tlm_timestamp++;
        sim.data[0] = 0x20U;                             /* CMD_REQUEST_DATA       */
        sim.data[1] = (uint8_t)(5U + rand() % 31U);     /* Doppler: 0.5–3.5 kHz  */
        sim.data[2] = (uint8_t)(tlm_timestamp >> 8U);   /* timestamp high byte    */
        sim.data[3] = (uint8_t)(tlm_timestamp & 0xFFU); /* timestamp low byte     */
    }
    else if (roll < 93)
    {
        /* ---- CMD_ENTER_SAFE (8%) ---- */
        sim.data[0] = 0x01U; /* CMD_ENTER_SAFE */
        sim.data[1] = 0x00U;
        sim.data[2] = 0x00U;
        sim.data[3] = 0x00U;
    }
    else if (roll < 97)
    {
        /* ---- CMD_START_OTA (4%) ---- */
        sim.data[0] = 0x10U;                         /* CMD_START_OTA     */
        sim.data[1] = 0x01U;                         /* ver_major = 1     */
        sim.data[2] = (uint8_t)(rand() % 10U);       /* ver_minor = 0–9   */
        sim.data[3] = 0x00U;
    }
    else if (roll < 99)
    {
        /* ---- CMD_REMOTE_CTRL (2%) ---- */
        sim.data[0] = 0x02U; /* CMD_REMOTE_CTRL */
        sim.data[1] = 0x00U;
        sim.data[2] = 0x00U;
        sim.data[3] = 0x00U;
    }
    else
    {
        /* ---- CMD_END_OTA (1%) ---- */
        sim.data[0] = 0x11U; /* CMD_END_OTA */
        sim.data[1] = 0x00U;
        sim.data[2] = 0x00U;
        sim.data[3] = 0x00U;
    }

    rx_index = 0U;
    rx_ready = 1U;
}

/**
 * @brief Inicializa o periférico USART (Simulação).
 *
 * Em hardware real, configuraria os pinos, baud rate e interrupções.
 * Na simulação, gera o primeiro pacote aleatório para testes.
 *
 * @return 1U indicando sucesso na inicialização.
 */
uint8_t hal_usart_init(void)
{
    hal_usart_randomize();
    return 1U;
}


uint8_t hal_rx_data_availible(){
    return rx_ready;
}

void hal_set_rx_set_one(){
    rx_ready = 1;
}

/**
 * @brief Verifica se o TX register está pronto para enviar.
 * @return 1 se pronto, 0 se ocupado.
 */
uint8_t hal_usart_tx_ready(void)
{
    return tx_ready;
}

/**
 * @brief Escreve um byte no TX register (simulação).
 * @param byte Byte a enviar.
 */
void hal_usart_write_byte(uint8_t byte)
{
    /* Em simulação, apenas consume o byte */
    tx_ready = 1U;
    (void)byte;
}

/**
 * @brief Lê um byte do RX register (simulação).
 * @return Próximo byte do buffer simulado.
 */
uint8_t hal_usart_read_byte(void)
{
    uint8_t byte = sim.data[rx_index];
    rx_index++;

    if (rx_index >= sim.len)
    {
        rx_ready = 0U;

        if (rx_auto_regen)
        {
            hal_usart_randomize();
        }
    }

    return byte;
}
