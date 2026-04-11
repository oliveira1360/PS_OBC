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
 * @brief Gera pacotes de dados simulados da Ground Station.
 *
 * Mantém o byte de cabeçalho `0x20` (ex: comando de requisição) e 
 * preenche o restante pacote com dados aleatórios para testar a 
 * robustez da camada aplicacional. Reinicia o índice de leitura.
 */
static void hal_usart_randomize(void)
{
    sim.data[0] = 0x20U;
    sim.data[1] = (uint8_t)(rand() % 256U);
    sim.data[2] = (uint8_t)(rand() % 256U);
    sim.data[3] = (uint8_t)(rand() % 256U);
    rx_index    = 0;
    rx_ready    = 1U;
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
 * do pacote tiverem sido lidos, baixa a flag `rx_ready`.
 *
 * @return O byte recebido, ou 0x00U se não houverem dados ou limite atingido.
 */
uint8_t hal_usart_read_char(void)
{
    uint8_t byte = 0x00U;

    if (rx_index < sim.len)
        byte = sim.data[rx_index++];

    if (rx_index >= sim.len)
        rx_ready = 0U;  /* ← sem randomize aqui para evitar loop infinito na leitura */

    return byte;
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