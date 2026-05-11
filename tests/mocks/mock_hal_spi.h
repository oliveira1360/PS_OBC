/**
 * @file mock_hal_spi.h
 * @brief Mock para hal_spi — controlo total do comportamento HAL em testes.
 *
 * O mock substitui totalmente hal_spi.c na compilação dos testes.
 * Fornece:
 *   - Controlo de valores de retorno (sequência + default)
 *   - Contagem de chamadas por função
 *   - Registo de argumentos enviados
 *
 * Utilização:
 *   1. Chama mock_spi_reset() no início de cada teste.
 *   2. Configura os valores de retorno desejados.
 *   3. Corre os ticks do driver.
 *   4. Verifica os resultados com TEST_ASSERT_*.
 */

#ifndef MOCK_HAL_SPI_H
#define MOCK_HAL_SPI_H

#include <stdint.h>
#include <string.h>

#define MOCK_SEQ_MAX  512   /* Máximo de valores em sequência */
#define MOCK_BUF_MAX  256   /* Máximo de bytes gravados       */

/* =========================================================================
 * Estado do mock SPI
 * ========================================================================= */
typedef struct {
    /* --- hal_spi_tx_ready() --- */
    uint8_t tx_ready_seq[MOCK_SEQ_MAX]; /* Sequência de valores a retornar */
    int     tx_ready_seq_len;           /* Número de valores na sequência  */
    int     tx_ready_call_count;        /* Total de chamadas               */
    int     tx_ready_default;           /* Valor após esgotar a sequência  */

    /* --- hal_spi_rx_ready() --- */
    uint8_t rx_ready_seq[MOCK_SEQ_MAX];
    int     rx_ready_seq_len;
    int     rx_ready_call_count;
    int     rx_ready_default;

    /* --- hal_spi_read_byte() --- */
    uint8_t rx_data_seq[MOCK_BUF_MAX];
    int     rx_data_seq_len;
    int     rx_data_call_count;
    uint8_t rx_data_default;

    /* --- hal_spi_init() --- */
    int     init_call_count;
    uint8_t init_return;

    /* --- hal_spi_cs_low() / hal_spi_cs_high() --- */
    int     cs_low_call_count;
    int     cs_high_call_count;
    uint8_t cs_low_last_pin;
    uint8_t cs_high_last_pin;

    /* --- hal_spi_send_byte() --- */
    int     send_byte_call_count;
    uint8_t sent_bytes[MOCK_BUF_MAX]; /* Registo de todos os bytes enviados */

    /* --- hal_spi_prepare_transfer() --- */
    int     prepare_call_count;

} mock_spi_t;

/* Instância global — acessível pelos testes */
extern mock_spi_t mock_spi;

/* =========================================================================
 * Funções de controlo do mock
 * ========================================================================= */

/** Reset completo: limpa toda a sequência e contadores. */
void mock_spi_reset(void);

/**
 * @brief Define a sequência de valores para hal_spi_tx_ready().
 * @param seq  Array de 0/1.
 * @param len  Comprimento do array.
 */
void mock_spi_set_tx_ready_seq(const uint8_t *seq, int len);

/**
 * @brief Define a sequência de valores para hal_spi_rx_ready().
 */
void mock_spi_set_rx_ready_seq(const uint8_t *seq, int len);

/**
 * @brief Define a sequência de bytes devolvidos por hal_spi_read_byte().
 */
void mock_spi_set_rx_data_seq(const uint8_t *seq, int len);

#endif /* MOCK_HAL_SPI_H */
