#ifndef QSPI_DRIVER_H
#define QSPI_DRIVER_H

#include <stdint.h>

/* =========================================================================
   Estados da FSM QSPI — seguem o diagrama de estados
   ========================================================================= */
typedef enum {
    QSPI_IDLE,           /** À espera de pedido de transferência            */
    QSPI_WRITE_ENABLE,   /** Envia WREN instruction                        */
    QSPI_CHECK_WEL,      /** Verifica WEL bit no status register           */
    QSPI_SEND_COMMAND,   /** Envia instruction + address + dummy cycles    */
    QSPI_READING,        /** Lê dados do RX buffer para memória            */
    QSPI_WRITING,        /** Escreve dados do buffer para TX               */
    QSPI_WAIT_BUSY,      /** Poll do busy bit no status register           */
    QSPI_ERROR           /** Erro — limpa flags e volta ao idle            */
} qspi_state_t;

/* =========================================================================
   Tipo de operação pedida
   ========================================================================= */
typedef enum {
    QSPI_OP_NONE,
    QSPI_OP_READ,
    QSPI_OP_WRITE,
    QSPI_OP_ERASE
} qspi_op_t;

/* =========================================================================
   Handle QSPI — um por instância
   ========================================================================= */
typedef struct {
    qspi_state_t  state;       /** Estado atual da FSM                     */
    qspi_op_t     operation;   /** Tipo de operação em curso               */
    uint8_t      *buf;         /** Ponteiro para buffer de dados            */
    uint32_t      addr;        /** Endereço na flash                        */
    uint32_t      len;         /** Número total de bytes                    */
    uint32_t      index;       /** Bytes já transferidos                    */
    uint16_t      timeout;     /** Contador de timeout                      */
    uint8_t       retry;       /** Contador de retries (WREN/WEL)           */
    uint8_t       error_code;  /** Código de erro (0 = sem erro)            */
    void        (*callback)(int); /** Callback: 1=sucesso, 0=erro           */
} qspi_handle_t;

/* =========================================================================
   Constantes de configuração
   ========================================================================= */
#define QSPI_TIMEOUT_MAX     5000U   /** Ciclos de main loop até timeout   */
#define QSPI_RETRY_MAX       50U     /** Tentativas de WREN antes de erro  */

/* =========================================================================
   Protótipos — API do driver
   ========================================================================= */

/**
 * @brief FSM tick — chamar em cada iteração do super-loop.
 */
void qspi_tick(qspi_handle_t *h);

/**
 * @brief Inicia leitura assíncrona da flash.
 *
 * @param h    Handle QSPI.
 * @param addr Endereço na flash.
 * @param buf  Buffer de destino.
 * @param len  Bytes a ler.
 * @param cb   Callback chamado no fim (1=ok, 0=erro).
 */
void qspi_read_async(qspi_handle_t *h, uint32_t addr,
                      uint8_t *buf, uint32_t len,
                      void (*cb)(int));

/**
 * @brief Inicia escrita assíncrona na flash.
 *
 * @param h    Handle QSPI.
 * @param addr Endereço na flash (max 256 bytes, alinhado à página).
 * @param buf  Buffer de origem.
 * @param len  Bytes a escrever (max W25Q_PAGE_SIZE).
 * @param cb   Callback chamado no fim (1=ok, 0=erro).
 */
void qspi_write_async(qspi_handle_t *h, uint32_t addr,
                       uint8_t *buf, uint32_t len,
                       void (*cb)(int));

/**
 * @brief Inicia erase assíncrono de um sector (4KB).
 *
 * @param h    Handle QSPI.
 * @param addr Endereço dentro do sector a apagar.
 * @param cb   Callback chamado no fim (1=ok, 0=erro).
 */
void qspi_erase_sector_async(qspi_handle_t *h, uint32_t addr,void (*cb)(int));

#endif