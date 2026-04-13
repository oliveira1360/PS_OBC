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
 * @brief Flag que indica se o USART está pronto para transmitir dados.
 */
static uint8_t tx_ready  = 1;

/**
 * @brief Flag que indica se existem dados disponíveis para leitura.
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

/**
 * @brief Verifica se o hardware USART está pronto para transmitir.
 *
 * @return Sempre 1U na simulação (o envio é imediato e inesgotável).
 */
uint8_t hal_usart_is_tx_ready(void)
{
    return tx_ready;  /* simulacao: sempre pronto para enviar */
}

/**
 * @brief Simula a transmissão de um caractere pelo USART.
 *
 * Como é uma simulação de Ground Station, os bytes enviados pelo 
 * satélite são simplesmente descartados.
 *
 * @param data O byte a ser transmitido.
 */
void hal_usart_write_char(uint8_t data)
{
    (void)data;  /* simulacao: descarta o byte enviado */
    tx_ready = 1U;
}

/**
 * @brief Verifica se há dados recebidos no buffer USART.
 *
 * @return 1U se houverem dados não lidos, 0U caso contrário.
 */
uint8_t hal_usart_data_available(void)
{
    return rx_ready;
}

/**
 * @brief Lê o próximo caractere do buffer de receção simulado.
 *
 * Retorna o próximo byte da estrutura `sim`. Quando todos os bytes
 * do pacote tiverem sido lidos, baixa a flag `rx_ready`. Se
 * `rx_auto_regen` estiver ativo, gera automaticamente um novo pacote.
 *
 * @return O byte recebido, ou 0x00U se não houverem dados ou limite atingido.
 */
uint8_t hal_usart_read_char(void)
{
    uint8_t byte = 0x00U;

    if (rx_index < sim.len)
        byte = sim.data[rx_index++];

    if (rx_index >= sim.len)
    {
        rx_ready = 0U;
        if (rx_auto_regen)
            hal_usart_randomize(); /* gera novo pacote da GS automaticamente */
    }

    return byte;
}

/**
 * @brief Controla manualmente o estado de prontidão do transmissor.
 *        Útil para simular condições de timeout em testes.
 *
 * @param v 1U = TX pronto; 0U = TX ocupado/bloqueado.
 */
void hal_usart_set_tx_ready(uint8_t v)
{
    tx_ready = v;
}

/**
 * @brief Ativa ou desativa a regeneração automática de pacotes RX.
 *        Quando desativada, o buffer esgota-se e `rx_ready` fica a 0,
 *        simulando ausência de dados (útil para testar timeouts de receção).
 *
 * @param v 1U = auto-regen ativo (padrão); 0U = desativado.
 */
void hal_usart_set_rx_auto_regen(uint8_t v)
{
    rx_auto_regen = v;
}

/**
 * @brief Prepara o USART para uma nova receção.
 *
 * Na simulação, aciona a geração de um novo pacote de comandos 
 * aleatórios provenientes da "Ground Station". Em hardware real, 
 * serviria para limpar FIFOs ou rearmar interrupções de receção.
 */
void hal_usart_prepare_rx(void)
{
    hal_usart_randomize();
}