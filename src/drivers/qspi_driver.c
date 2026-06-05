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
#include <stdio.h>

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

    h->operation = QSPI_OP_READ;
    h->addr = addr;
    h->buf = buf;
    h->len = len;
    h->index = 0U;
    h->timeout = 0U;
    h->retry = 0U;
    h->error_code = 0U;
    h->callback = cb;
    h->state = QSPI_SEND_COMMAND; /* Read salta Write_Enable */
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

    h->operation = QSPI_OP_WRITE;
    h->addr = addr;
    h->buf = buf;
    h->len = len;
    h->index = 0U;
    h->timeout = 0U;
    h->retry = 0U;
    h->error_code = 0U;
    h->callback = cb;
    h->state = QSPI_WRITE_ENABLE;
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

    h->operation = QSPI_OP_ERASE;
    h->addr = addr;
    h->buf = (void *)0;
    h->len = 0U;
    h->index = 0U;
    h->timeout = 0U;
    h->retry = 0U;
    h->error_code = 0U;
    h->callback = cb;
    h->state = QSPI_WRITE_ENABLE;
}

static void qspi_handle_idle(qspi_handle_t *h)
{
    /* Nada a fazer, espera por pedidos async */
    (void)h;
}

static void qspi_handle_write_enable(qspi_handle_t *h)
{
    hal_qspi_send_command(W25Q_CMD_WRITE_ENABLE);
    h->timeout = 0U;
    h->state = QSPI_CHECK_WEL;
}

static void qspi_handle_check_wel(qspi_handle_t *h)
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
}

static void qspi_handle_send_command(qspi_handle_t *h)
{
    switch (h->operation)
    {
    case QSPI_OP_READ:
        hal_qspi_read_memory(h->addr, h->buf, h->len);
        h->index = h->len;
        h->state = QSPI_READING;
        break;

    case QSPI_OP_WRITE:
        hal_qspi_write_memory(h->addr, h->buf, h->len);
        h->index = h->len;

        /* Espera ativa que a flash levante o BUSY (programa interno arrancou).
         * Sem isto, WAIT_BUSY pode ler BUSY=0 antes do programa começar
         * e dar a página por terminada cedo demais. */
        {
            uint32_t _t = 0x40000U;
            while (!(hal_qspi_read_status() & W25Q_SR1_BUSY) && --_t)
                ;
        }

        h->state = QSPI_WAIT_BUSY;
        h->timeout = 0U;
        break;

    case QSPI_OP_ERASE:
        hal_qspi_send_command_addr(W25Q_CMD_SECTOR_ERASE, h->addr);

        /* DELAY DE SEGURANÇA: Garante que a flash já ativou o BUSY */
        {
            volatile uint32_t _d = 20000U;
            while (_d--)
            {
            }
        }

        h->state = QSPI_WAIT_BUSY;
        h->timeout = 0U;
        break;

    default:
        h->error_code = 2U;
        h->state = QSPI_ERROR;
        break;
    }
}

static void qspi_handle_reading(qspi_handle_t *h)
{
    if (h->index >= h->len)
    {
        /* Leitura completa — volta ao idle sem Wait_Busy */
        h->state = QSPI_IDLE;
        if (h->callback != (void *)0)
        {
            h->callback(1);
        }
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
}

static void qspi_handle_writing(qspi_handle_t *h)
{
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
}

static void qspi_handle_wait_busy(qspi_handle_t *h)
{
    if (!hal_qspi_is_busy())
    {
        /* Flash terminou a operação */
        h->state = QSPI_IDLE;
        if (h->callback != (void *)0)
        {
            h->callback(1);
        }
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
}

static void qspi_handle_error(qspi_handle_t *h)
{
    printf("\r\n[QSPI DRIVER] FSM abortou! error_code = %u\n", h->error_code);

    if (h->callback != (void *)0)
    {
        h->callback(0); /* 0 = erro */
    }

    /* Limpa estado */
    h->index = 0U;
    h->len = 0U;
    h->timeout = 0U;
    h->retry = 0U;
    h->operation = QSPI_OP_NONE;
    h->state = QSPI_IDLE;
}

/**
 * @brief FSM principal do driver QSPI.
 */
void qspi_tick(qspi_handle_t *h)
{
    switch (h->state)
    {
    case QSPI_IDLE:
        qspi_handle_idle(h);
        break;
    case QSPI_WRITE_ENABLE:
        qspi_handle_write_enable(h);
        break;
    case QSPI_CHECK_WEL:
        qspi_handle_check_wel(h);
        break;
    case QSPI_SEND_COMMAND:
        qspi_handle_send_command(h);
        break;
    case QSPI_READING:
        qspi_handle_reading(h);
        break;
    case QSPI_WRITING:
        qspi_handle_writing(h);
        break;
    case QSPI_WAIT_BUSY:
        qspi_handle_wait_busy(h);
        break;
    case QSPI_ERROR:
        qspi_handle_error(h);
        break;
    default:
        h->state = QSPI_IDLE;
        break;
    }
}