/**
 * @file usart_driver.c
 * @brief Driver USART com FSMs non-blocking para TX e RX.
 */

#include "drivers/usart_driver.h"
#include "hal/hal_usart.h"
#include "config/board.h"
#include <stdio.h>

/**
 * @brief Inicia uma transmissão assíncrona.
 *
 * Configura o handle para começar a enviar dados.
 * O envio efetivo acontece em usart_tx_tick().
 *
 * @param h    Ponteiro para o handle USART.
 * @param data Ponteiro para o buffer de dados a enviar.
 * @param len  Número de bytes a enviar.
 */
void usart_send_async(usart_handle_t *h, uint8_t *data, uint8_t len)
{
    if (h->tx_state != UART_TX_IDLE)
    {
        return; /* já está a transmitir, ignora */
    }

    h->tx_buf = data;
    h->tx_len = len;
    h->tx_index = 0U;
    h->timeout = 0U;
    h->tx_state = UART_TX_TRANSMITTING;
}

/**
 * @brief Inicia uma receção assíncrona.
 *
 * Configura o handle para começar a receber dados.
 * A receção efetiva acontece em usart_rx_tick().
 *
 * @param h   Ponteiro para o handle USART.
 * @param buf Ponteiro para o buffer onde armazenar dados recebidos.
 * @param len Número de bytes esperados.
 * @param cb  Callback chamado quando a receção termina (1=sucesso, 0=erro).
 */
void usart_recv_async(usart_handle_t *h, uint8_t *buf, uint8_t len, void (*cb)(int))
{
    if (h->rx_state != UART_RX_IDLE)
    {
        return; /* já está a receber, ignora */
    }

    h->rx_buf   = buf;
    h->rx_len   = len;
    h->rx_index = 0U;
    h->timeout  = 0U;
    h->callback = cb;
    h->rx_state = UART_RX_RECEIVING;
    /* Leitura byte a byte via circular buffer (interrupt RXRDY).
     * Usado para comandos TTC (4 B) e frame scanner OTA (1 B).  */
}

/**
 * @brief FSM de transmissão — chamar em cada ciclo do main loop.
 *
 * Estados:
 *   UART_TX_IDLE         — à espera de pedido de transmissão
 *   UART_TX_TRANSMITTING — a enviar bytes um a um
 *   UART_TX_ERROR        — erro de timeout
 *
 * @param h Ponteiro para o handle USART.
 */
void usart_tx_tick(usart_handle_t *h)
{
    switch (h->tx_state)
    {
    case UART_TX_IDLE:
        /* Nada a fazer — espera por usart_send_async() */
        break;

    case UART_TX_TRANSMITTING:
        if (hal_usart_tx_ready())
        {
            /* TX register livre — envia próximo byte */
            hal_usart_write_byte(h->tx_buf[h->tx_index]);
            h->tx_index++;
            h->timeout = 0U;

            if (h->tx_index >= h->tx_len)
            {
                /* Último byte enviado — volta ao idle */
                h->tx_state = UART_TX_IDLE;
            }
        }
        else
        {
            /* TX register ocupado — verifica timeout */
            h->timeout++;
            if (h->timeout >= USART_TIMEOUT_MAX)
            {
                h->tx_state = UART_TX_ERROR;
            }
        }
        break;

    case UART_TX_ERROR:
        /* Limpa erro e volta ao idle */
        h->tx_index = 0U;
        h->tx_len = 0U;
        h->timeout = 0U;
        h->tx_state = UART_TX_IDLE;
        break;

    default:
        h->tx_state = UART_TX_IDLE;
        break;
    }
}

/**
 * @brief FSM de receção — chamar em cada ciclo do main loop.
 *
 * Estados:
 *   UART_RX_IDLE      — a monitorizar se há dados disponíveis
 *   UART_RX_RECEIVING — a receber bytes um a um
 *   UART_RX_ERROR     — erro de framing, overrun ou timeout
 *
 * @param h Ponteiro para o handle USART.
 */
void usart_rx_tick(usart_handle_t *h)
{
    switch (h->rx_state)
    {
    case UART_RX_IDLE:
        if (hal_rx_data_availible())
        {
            h->rx_index = 0U;
            h->timeout = 0U;
            h->rx_state = UART_RX_RECEIVING;
        }
        break;

    case UART_RX_RECEIVING:
        /* Lê bytes do circular buffer (preenchido por USART0_Handler).
         * Funciona igual em HW real e simulação — zero perda de bytes. */
        if (hal_rx_data_availible())
        {
            h->rx_buf[h->rx_index] = hal_usart_read_byte();
            h->rx_index++;
            h->timeout = 0U;

            if (h->rx_index >= h->rx_len)
            {
                h->rx_state = UART_RX_IDLE;
                if (h->callback != (void *)0)
                    h->callback(1);
            }
        }
        else
        {
            if (h->rx_index > 0)
            {
                h->timeout++;
                if (h->timeout >= USART_TIMEOUT_MAX)
                    h->rx_state = UART_RX_ERROR;
            }
        }
        break;

    case UART_RX_ERROR:
        /* Notifica erro e limpa estado */
        if (h->callback != (void *)0)
        {
            h->callback(0); /* 0 = erro */
        }

        h->rx_index = 0U;
        h->rx_len = 0U;
        h->timeout = 0U;
        h->rx_state = UART_RX_IDLE;
        break;

    default:
        h->rx_state = UART_RX_IDLE;
        break;
    }
}

/**
 * @brief Inicia receção DMA bulk — para o payload OTA (132 bytes).
 *
 * Ao contrário de usart_recv_async (que usa o circular buffer byte a byte),
 * esta função delega ao XDMAC toda a transferência de @p len bytes.
 * O XDMAC desabilita o interrupt RXRDY durante a transferência e
 * reabilita-o em XDMAC_Handler. A callback é chamada pelo usart_rx_tick
 * quando o DMA termina (contexto do main loop).
 *
 * @note Só faz sentido em hardware real (USE_REAL_HW=1).
 *       Em simulação cai para usart_recv_async normal.
 */
void usart_recv_dma(usart_handle_t *h, uint8_t *buf, uint16_t len, void (*cb)(int))
{
    if (h->rx_state != UART_RX_IDLE)
        return;

    h->rx_buf   = buf;
    h->rx_len   = len;
    h->rx_index = 0U;
    h->timeout  = 0U;
    h->callback = cb;
    h->rx_state = UART_RX_RECEIVING;

#if USE_REAL_HW
    hal_usart_dma_recv(buf, len);
#endif
    /* Em simulação: usart_rx_tick drena o buffer simulado normalmente */
}

/**
 * @brief Tick DMA — deve ser chamado em cada iteração do main loop
 *        quando uma transferência DMA está activa.
 *
 * Detecta o fim da transferência DMA (flag s_dma_rx_done setada por
 * XDMAC_Handler) e chama a callback no contexto do main loop.
 */
void usart_dma_tick(usart_handle_t *h)
{
#if USE_REAL_HW
    if (h->rx_state == UART_RX_RECEIVING && hal_usart_dma_done())
    {
        hal_usart_dma_clear();
        h->rx_index = h->rx_len;
        h->rx_state = UART_RX_IDLE;
        if (h->callback != (void *)0)
            h->callback(1);
    }
#else
    /* Em simulação o usart_rx_tick já trata tudo */
    (void)h;
#endif
}