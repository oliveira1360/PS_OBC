#ifndef HAL_DEBUG_UART_H
#define HAL_DEBUG_UART_H

/**
 * @brief Inicializa USART1 (EDBG) para printf a 115200 baud.
 * Chamar uma vez no início do main().
 */
void debug_uart_init(void);

#endif /* HAL_DEBUG_UART_H */