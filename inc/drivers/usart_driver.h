#ifndef USART_DRIVER_H
#define USART_DRIVER_H

#include <stdint.h>

<<<<<<< HEAD
#define USART_TIMEOUT_MAX 200
=======
#define USART_TIMEOUT_MAX 1000000UL
>>>>>>> origin/OBC_board

// Os estados agora gerem o fluxo de dados, não os bits físicos!
typedef enum {
    SERIAL_IDLE,
    SERIAL_TX_BUSY,  // A enviar o tx_buf
    SERIAL_RX_BUSY,  // A encher o rx_buf
    SERIAL_ERROR
} usart_state_t;

<<<<<<< HEAD
typedef struct {
    usart_state_t state;
=======

typedef enum {
    UART_TX_IDLE,
    UART_TX_TRANSMITTING,
    UART_TX_ERROR
} uart_tx_state_t;

typedef enum {
    UART_RX_IDLE,
    UART_RX_RECEIVING,
    UART_RX_ERROR
} uart_rx_state_t;

typedef struct {
    uart_rx_state_t rx_state;
    uart_tx_state_t tx_state;
>>>>>>> origin/OBC_board
    
    uint8_t *tx_buf;
    uint8_t tx_len;
    uint8_t tx_index; // Quantos bytes já foram enviados
    
    uint8_t *rx_buf;
<<<<<<< HEAD
    uint8_t rx_len;
    uint8_t rx_index; // Quantos bytes já foram recebidos
    
    uint8_t timeout;
    void (*callback)(int status);
} usart_handle_t;

void usart_tick(usart_handle_t *h);
=======
    uint16_t  rx_len;
    uint16_t  rx_index; 
    
    uint32_t timeout;
    void (*callback)(int status);
} usart_handle_t;





void usart_tx_tick(usart_handle_t *h);
void usart_rx_tick(usart_handle_t *h);
>>>>>>> origin/OBC_board
void usart_send_async(usart_handle_t *h, uint8_t *data, uint8_t len);
void usart_recv_async(usart_handle_t *h, uint8_t *buf, uint8_t len, void (*cb)(int));

#endif