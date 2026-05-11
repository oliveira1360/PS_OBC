<<<<<<< HEAD
#include "drivers/qspi_driver.h"

void qspi_tick(qspi_handle_t *h)
{
    switch(h->state)
    {
        case QSPI_STARTING:
        case QSPI_TRANSFER:
        case QSPI_WAIT_TX:
        case QSPI_STOP:
        case QSPI_IDLE:
    }
}
=======
/**
 * @file qspi_driver.c
 * @brief Driver QSPI com FSM non-blocking para W25Q128.
 *
 * Segue o diagrama de estados:
 *   Idle → [read]  → Send_Command → Reading → Idle
 *   Idle → [write] → Write_Enable → Check_WEL → Send_Command → Writing → Wait_Busy → Idle
 *   Idle → [erase] → Write_Enable → Check_WEL → Send_Command → Wait_Busy → Idle
 *   Qualquer estado → [timeout/max retry] → Error → Idle
 */

#include "drivers/qspi_driver.h"
#include "hal/hal_qspi.h"

/* ==========================================================================
 * API — Iniciar operações assíncronas
 * ========================================================================== */

/**
 * @brief Inicia leitura assíncrona.
 *        Read não precisa de Write Enable — vai direto para Send_Command.
 */
void qspi_read_async(qspi_handle_t *h, uint32_t addr,
                      uint8_t *buf, uint32_t len,
                      void (*cb)(int))
{
    if (h->state != QSPI_IDLE)
        return;

    h->operation  = QSPI_OP_READ;
    h->addr       = addr;
    h->buf        = buf;
    h->len        = len;
    h->index      = 0U;
    h->timeout    = 0U;
    h->retry      = 0U;
    h->error_code = 0U;
    h->callback   = cb;
    h->state      = QSPI_SEND_COMMAND;  /* Read salta Write_Enable */
}

/**
 * @brief Inicia escrita assíncrona (max 256 bytes, uma página).
 *        Write precisa de Write Enable → Check WEL antes de Send_Command.
 */
void qspi_write_async(qspi_handle_t *h, uint32_t addr,
                       uint8_t *buf, uint32_t len,
                       void (*cb)(int))
{
    if (h->state != QSPI_IDLE)
        return;

    h->operation  = QSPI_OP_WRITE;
    h->addr       = addr;
    h->buf        = buf;
    h->len        = len;
    h->index      = 0U;
    h->timeout    = 0U;
    h->retry      = 0U;
    h->error_code = 0U;
    h->callback   = cb;
    h->state      = QSPI_WRITE_ENABLE;
}

/**
 * @brief Inicia erase de sector (4KB).
 *        Erase precisa de Write Enable → Check WEL antes de Send_Command.
 */
void qspi_erase_sector_async(qspi_handle_t *h, uint32_t addr,
                              void (*cb)(int))
{
    if (h->state != QSPI_IDLE)
        return;

    h->operation  = QSPI_OP_ERASE;
    h->addr       = addr;
    h->buf        = (void *)0;
    h->len        = 0U;
    h->index      = 0U;
    h->timeout    = 0U;
    h->retry      = 0U;
    h->error_code = 0U;
    h->callback   = cb;
    h->state      = QSPI_WRITE_ENABLE;
}

/* ==========================================================================
 * FSM Tick — chamar em cada iteração do super-loop
 * ========================================================================== */

/**
 * @brief FSM principal do driver QSPI.
 *
 * Estados:
 *   QSPI_IDLE         — à espera de pedido
 *   QSPI_WRITE_ENABLE — envia WREN, transita para Check_WEL
 *   QSPI_CHECK_WEL    — verifica WEL bit; retenta ou erro
 *   QSPI_SEND_COMMAND — configura e envia instrução ao flash
 *   QSPI_READING      — lê dados da flash para o buffer
 *   QSPI_WRITING      — escreve dados do buffer para a flash
 *   QSPI_WAIT_BUSY    — poll do busy bit até flash terminar
 *   QSPI_ERROR        — notifica erro e volta ao idle
 */
void qspi_tick(qspi_handle_t *h)
{
    switch (h->state)
    {

    /* ------------------------------------------------------------------
     * IDLE — nada a fazer, espera por qspi_read/write/erase_async()
     * ------------------------------------------------------------------ */
    case QSPI_IDLE:
        break;

    /* ------------------------------------------------------------------
     * WRITE_ENABLE — envia WREN (0x06) para habilitar escrita/erase
     * Transição: → Check_WEL (sucesso) | → Error (timeout)
     * ------------------------------------------------------------------ */
    case QSPI_WRITE_ENABLE:
        hal_qspi_send_command(W25Q_CMD_WRITE_ENABLE);
        h->timeout = 0U;
        h->state = QSPI_CHECK_WEL;
        break;

    /* ------------------------------------------------------------------
     * CHECK_WEL — lê status register e verifica WEL bit
     * Transição: → Send_Command (WEL=1)
     *            → Write_Enable (WEL=0, retry < MAX)
     *            → Error (WEL=0, retry >= MAX)
     * ------------------------------------------------------------------ */
    case QSPI_CHECK_WEL:
    {
        uint8_t sr = hal_qspi_read_status();

        if (sr & W25Q_SR1_WEL)
        {
            /* WEL bit set — avança para enviar o comando */
            h->timeout = 0U;
            h->state = QSPI_SEND_COMMAND;
        }
        else
        {
            h->retry++;
            if (h->retry >= QSPI_RETRY_MAX)
            {
                h->error_code = 1U; /* WEL failed after max retries */
                h->state = QSPI_ERROR;
            }
            else
            {
                /* Retenta WREN */
                h->state = QSPI_WRITE_ENABLE;
            }
        }
        break;
    }

    /* ------------------------------------------------------------------
     * SEND_COMMAND — envia instruction + address + dummy cycles
     * Transição: → Reading (read operation)
     *            → Writing (write operation)
     *            → Wait_Busy (erase operation — sem dados)
     *            → Error (timeout)
     * ------------------------------------------------------------------ */
    case QSPI_SEND_COMMAND:
        switch (h->operation)
        {
        case QSPI_OP_READ:
            /* Read: o HAL configura Fast Read com dummy cycles e
             * lê tudo de uma vez via memory-mapped access */
            hal_qspi_read_memory(h->addr, h->buf, h->len);
            h->index = h->len;
            h->state = QSPI_READING;
            break;

        case QSPI_OP_WRITE:
            /* Write: o HAL configura Page Program e escreve tudo */
            hal_qspi_write_memory(h->addr, h->buf, h->len);
            h->index = h->len;
            h->state = QSPI_WRITING;
            break;

        case QSPI_OP_ERASE:
            /* Erase: envia comando + endereço, sem dados */
            hal_qspi_send_command_addr(W25Q_CMD_SECTOR_ERASE, h->addr);
            h->state = QSPI_WAIT_BUSY;
            h->timeout = 0U;
            break;

        default:
            h->error_code = 2U; /* Unknown operation */
            h->state = QSPI_ERROR;
            break;
        }
        break;

    /* ------------------------------------------------------------------
     * READING — dados já foram lidos pelo HAL em Send_Command
     * Transição: → Idle (bytes_remaining == 0, sucesso)
     *
     * Nota: no SAM V71 Serial Memory Mode, o read é feito todo de uma
     * vez via memory-mapped access no Send_Command. Este estado existe
     * para compatibilidade com o diagrama e para futuras implementações
     * byte-a-byte.
     * ------------------------------------------------------------------ */
    case QSPI_READING:
        if (h->index >= h->len)
        {
            /* Leitura completa — volta ao idle sem Wait_Busy */
            h->state = QSPI_IDLE;
            if (h->callback != (void *)0)
                h->callback(1);
        }
        else
        {
            /* Timeout: não devia chegar aqui */
            h->timeout++;
            if (h->timeout >= QSPI_TIMEOUT_MAX)
            {
                h->error_code = 3U; /* Read timeout */
                h->state = QSPI_ERROR;
            }
        }
        break;

    /* ------------------------------------------------------------------
     * WRITING — dados já foram escritos pelo HAL em Send_Command
     * Transição: → Wait_Busy (bytes_remaining == 0)
     *            → Error (timeout)
     * ------------------------------------------------------------------ */
    case QSPI_WRITING:
        if (h->index >= h->len)
        {
            /* Escrita completa — agora espera que a flash termine */
            h->timeout = 0U;
            h->state = QSPI_WAIT_BUSY;
        }
        else
        {
            h->timeout++;
            if (h->timeout >= QSPI_TIMEOUT_MAX)
            {
                h->error_code = 4U; /* Write timeout */
                h->state = QSPI_ERROR;
            }
        }
        break;

    /* ------------------------------------------------------------------
     * WAIT_BUSY — poll do busy bit no status register
     * Transição: → Idle (busy == 0, sucesso)
     *            → Wait_Busy (busy == 1, timeout não atingido)
     *            → Error (timeout atingido)
     * ------------------------------------------------------------------ */
    case QSPI_WAIT_BUSY:
        if (!hal_qspi_is_busy())
        {
            /* Flash terminou a operação */
            h->state = QSPI_IDLE;
            if (h->callback != (void *)0)
                h->callback(1);
        }
        else
        {
            h->timeout++;
            if (h->timeout >= QSPI_TIMEOUT_MAX)
            {
                h->error_code = 5U; /* Busy timeout */
                h->state = QSPI_ERROR;
            }
        }
        break;

    /* ------------------------------------------------------------------
     * ERROR — notifica erro via callback, limpa estado, volta ao idle
     * ------------------------------------------------------------------ */
    case QSPI_ERROR:
        if (h->callback != (void *)0)
        {
            h->callback(0); /* 0 = erro */
        }

        /* Limpa estado */
        h->index      = 0U;
        h->len        = 0U;
        h->timeout    = 0U;
        h->retry      = 0U;
        h->operation  = QSPI_OP_NONE;
        h->state      = QSPI_IDLE;
        break;

    /* ------------------------------------------------------------------
     * DEFAULT — proteção contra estados inválidos (ex: SEU)
     * ------------------------------------------------------------------ */
    default:
        h->state = QSPI_IDLE;
        break;
    }
}
>>>>>>> origin/OBC_board
