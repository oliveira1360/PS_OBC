/**
 * @file hal_spi.c
 * @brief Simulação da Hardware Abstraction Layer (HAL) para o barramento SPI.
 *
 * Este módulo não interage com o hardware físico SPI. Serve apenas para simular
 * o comportamento do barramento para efeitos de teste (gerando dados aleatórios
 * na receção). O código para o hardware real encontra-se comentado dentro
 * das funções como referência.
 */

#include "hal/hal_spi.h"
#include "config/board.h"
#include <stdlib.h>  /* simulacao apenas !!! */

/**
 * @brief Flag que indica se o barramento SPI está pronto para transmitir (Tx).
 */
static uint8_t tx_ready = 1U;

/**
 * @brief Flag que indica se há dados prontos a ser lidos na receção (Rx).
 */
static uint8_t rx_ready = 0U;

/**
 * @brief Buffer interno para guardar o byte simulado recebido via SPI.
 */
static uint8_t rx_data  = 0x00U;

/**
 * @brief Inicializa o periférico SPI (Simulação).
 *
 * Na versão simulada, esta função apenas retorna sucesso. Na versão real,
 * configuraria os registos do microcontrolador (Power Management, Baud Rate, etc.).
 *
 * @return 1U indicando sucesso na inicialização.
 */
uint8_t hal_spi_init(void)
{
    /* hardware real:
       PMC_PCER0 |= (1U << ID_SPI0);
       SPI0_CR   = SPI_CR_SWRST;
       SPI0_MR   = SPI_MR_MSTR | SPI_MR_MODFDIS;
       SPI0_CSR0 = (SPI_BAUD_DIV8 << 8U);
       SPI0_CR   = SPI_CR_SPIEN; */
    return 1U;
}

/**
 * @brief Verifica se o SPI está pronto para transmitir um novo byte.
 *
 * @return 1U se estiver pronto para transmitir, 0U caso contrário.
 */
uint8_t hal_spi_tx_ready(void) { return tx_ready; }

/**
 * @brief Verifica se o SPI tem um byte recebido pronto a ser lido.
 *
 * @return 1U se houver dados disponíveis, 0U caso contrário.
 */
uint8_t hal_spi_rx_ready(void) { return rx_ready; }

/**
 * @brief Coloca o pino de Chip Select (CS) em nível lógico baixo (Ativo).
 *
 * Simula a ativação de um dispositivo escravo no barramento SPI.
 *
 * @param cs_pin O pino/identificador do Chip Select a ser ativado.
 */
void hal_spi_cs_low(uint8_t cs_pin)
{
    (void)cs_pin;
    /* hardware: PIO_CODR = (1U << cs_pin) */
}

/**
 * @brief Coloca o pino de Chip Select (CS) em nível lógico alto (Inativo).
 *
 * Simula a desativação de um dispositivo escravo no barramento SPI.
 *
 * @param cs_pin O pino/identificador do Chip Select a ser desativado.
 */
void hal_spi_cs_high(uint8_t cs_pin)
{
    (void)cs_pin;
    /* hardware: PIO_SODR = (1U << cs_pin) */
}

/**
 * @brief Simula o envio de um byte pelo barramento SPI (Full-Duplex).
 *
 * Como o SPI é full-duplex, enviar um byte significa também receber um byte.
 * Esta função ignora o byte enviado na simulação e gera automaticamente
 * um byte aleatório (usando `rand()`) para simular a resposta do escravo.
 *
 * @param data O byte de dados a ser transmitido (ignorado na simulação).
 */
void hal_spi_send_byte(uint8_t data)
{
    (void)data;
    /* hardware: SPI0_TDR = data */
    tx_ready = 1U;
    rx_data  = (uint8_t)(rand() % 256U);  /* simulacao */
    rx_ready = 1U;
}

/**
 * @brief Lê o último byte recebido pelo barramento SPI.
 *
 * Limpa a flag de receção (`rx_ready`) e retorna o valor gerado internamente
 * pela função de envio.
 *
 * @return O byte lido do registo de dados (simulado).
 */
uint8_t hal_spi_read_byte(void)
{
    rx_ready = 0U;
    return rx_data;
    /* hardware: return (uint8_t)(SPI0_RDR & 0xFFU) */
}

/**
 * @brief Prepara o periférico SPI para uma transferência.
 *
 * Esta função atua como um "stub" (função vazia) na simulação, podendo ser
 * útil em hardware real para esvaziar FIFOs ou limpar flags de erro antes 
 * de iniciar uma nova comunicação.
 */
void hal_spi_prepare_transfer(void) { }