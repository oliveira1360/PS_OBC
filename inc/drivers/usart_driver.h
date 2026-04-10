#ifndef USART_DRIVER_H
#define USART_DRIVER_H

#include <stdint.h>

#define USART_TIMEOUT_MAX 200

// Os estados agora gerem o fluxo de dados, não os bits físicos!
typedef enum {
    SERIAL_IDLE,
    SERIAL_TX_BUSY,  // A enviar o tx_buf
    SERIAL_RX_BUSY,  // A encher o rx_buf
    SERIAL_ERROR
} usart_state_t;

typedef struct {
    usart_state_t state;
    
    uint8_t *tx_buf;
    uint8_t tx_len;
    uint8_t tx_index; // Quantos bytes já foram enviados
    
    uint8_t *rx_buf;
    uint8_t rx_len;
    uint8_t rx_index; // Quantos bytes já foram recebidos
    
    uint8_t timeout;
    void (*callback)(int status);
} usart_handle_t;

void usart_tick(usart_handle_t *h);
void usart_send_async(usart_handle_t *h, uint8_t *data, uint8_t len);

#endif