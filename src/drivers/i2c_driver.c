#include "drivers/i2c_driver.h"
#include "hal/hal_i2c.h"
<<<<<<< HEAD
#include <string.h>

/* Definições globais declaradas em i2c_driver.h */
i2c_handle_t i2c_master;
i2c_queue_t  i2c_queue;

/**
 * @brief Enfileira um pedido I2C na fila circular.
 *        Se a fila estiver cheia, o pedido é descartado (proteção ECSS 5.5.3.2c).
 */
void i2c_enqueue(uint8_t addr, uint8_t *buf, uint8_t len, uint8_t rw, void (*cb)(int))
{
    if (i2c_queue.count >= I2C_QUEUE_SIZE)
        return; /* fila cheia — descarta */

    i2c_request_t *req = &i2c_queue.requests[i2c_queue.tail];
    req->addr     = addr;
    req->buf      = buf;
    req->len      = len;
    req->rw       = rw;
    req->callback = cb;

    i2c_queue.tail = (i2c_queue.tail + 1) % I2C_QUEUE_SIZE;
    i2c_queue.count++;
=======
#include <stdio.h>

static uint8_t bus_locked = 0U;

static void i2c_fail(i2c_handle_t *h)
{
    hal_i2c_stop();
    h->state = I2C_IDLE;
    h->timeout = 0;
    h->error_count++;

    /* Se falhar várias vezes seguidas, o bus pode estar preso */
    if (h->error_count >= 3)
    {
        printf("bus recovery for addr=0x%02X\n", h->addr);
        hal_i2c_bus_recovery();
        h->error_count = 0;
    }

    bus_locked = 0U;
    if (h->callback)
        h->callback(-1);
}

static void i2c_success(i2c_handle_t *h)
{
    hal_i2c_stop();
    h->state = I2C_IDLE;
    h->error_count = 0; /* reset no sucesso */
    bus_locked = 0U;
    if (h->callback)
        h->callback(0);
}

static void handle_starting(i2c_handle_t *h)
{
    if (!hal_i2c_bus_free() || bus_locked)
        return;

    bus_locked = 1U;
    hal_i2c_start();

    if (h->use_reg)
        hal_i2c_send_addr((h->addr << 1) | 0);
    else
        hal_i2c_send_addr((h->addr << 1) | (h->rw ? 1 : 0));

    h->state = I2C_SELECT_MODE;
}

static void handle_select_mode(i2c_handle_t *h)
{
    if (!hal_i2c_get_ack())
    {
        i2c_fail(h);
        return;
    }

    h->index = 0;

    if (h->use_reg)
    {
        hal_i2c_send_byte(h->reg);
        h->state = I2C_RESTART;
    }
    else
    {
        h->state = h->rw ? I2C_READ : I2C_WRITE;
    }
}

static void handle_restart(i2c_handle_t *h)
{
    if (!hal_i2c_tx_ready())
    {
        if (++h->timeout > I2C_TIMEOUT_MAX)
            i2c_fail(h);
        return;
    }
    h->timeout = 0;

    if (!hal_i2c_get_ack())
    {
        i2c_fail(h);
        return;
    }

    hal_i2c_restart_read(h->addr);
    h->use_reg = 0;
    h->state = I2C_WAIT_RESTART;
}

static void handle_wait_restart(i2c_handle_t *h)
{
    if (!hal_i2c_get_ack())
    {
        i2c_fail(h);
        return;
    }
    h->index = 0;
    h->state = I2C_READ;
}

static void handle_write(i2c_handle_t *h)
{
    hal_i2c_send_byte(h->buf[h->index]);
    h->state = I2C_WAIT_TX;
}

static void handle_wait_tx(i2c_handle_t *h)
{
    if (!hal_i2c_tx_ready())
    {
        if (++h->timeout > I2C_TIMEOUT_MAX)
            i2c_fail(h);
        return;
    }
    h->timeout = 0;

    if (!hal_i2c_get_ack())
    {
        i2c_fail(h);
        return;
    }

    h->index++;
    h->state = (h->index == h->len) ? I2C_STOP : I2C_WRITE;
}

static void handle_read(i2c_handle_t *h)
{
    hal_i2c_request_byte();
    h->state = I2C_WAIT_RX;
}

static void handle_wait_rx(i2c_handle_t *h)
{
    if (!hal_i2c_rx_ready())
    {
        if (++h->timeout > I2C_TIMEOUT_MAX)
            i2c_fail(h);
        return;
    }
    h->timeout = 0;

    h->buf[h->index] = hal_i2c_read_byte();
    h->index++;

    if (h->index == h->len)
    {
        hal_i2c_send_nack();
        h->state = I2C_STOP;
    }
    else
    {
        hal_i2c_send_ack();
        h->state = I2C_READ;
    }
>>>>>>> origin/OBC_board
}

void i2c_tick(i2c_handle_t *h)
{
    switch (h->state)
    {
    case I2C_STARTING:
<<<<<<< HEAD
        // Se o barramento I2C estiver ocupado por outra comunicação, espera.
        if (!hal_i2c_bus_free())
            break;
            
        hal_i2c_start();
        
        // O endereço I2C tem 7 bits. Deslocamos 1 bit para a esquerda (<< 1) 
        // e no espaço vazio colocamos o bit de Direção (1 = LER/RX, 0 = ESCREVER/TX)
        hal_i2c_send_addr((h->addr << 1) | (h->rw ? 1 : 0));
        h->state = I2C_SELECT_MODE;
        break;

    case I2C_SELECT_MODE:
        // Espera que o sensor reconheça o seu próprio endereço (Acknowledge)
        if (!hal_i2c_get_ack())
        {
            // O sensor não está ligado, está estragado ou o endereço está errado.
            hal_i2c_stop(); // liberta o bus para outros dispositivos
            h->state = I2C_IDLE;
            if (h->callback)
                h->callback(-1); // -1 indica erro à aplicação
            break;
        }
        
        // O sensor respondeu! Prepara o índice e segue para Ler ou Escrever
        h->index = 0;
        h->state = h->rw ? I2C_READ : I2C_WRITE;
        break;

    case I2C_WRITE:
        // Pega no próximo byte do buffer e atira-o para o hardware de Transmissão (TX)
        hal_i2c_send_byte(h->buf[h->index]);
        h->state = I2C_WAIT_TX;
        break;

    case I2C_WAIT_TX:
        // Se o hardware ainda está ocupado a enviar o byte fisicamente no fio...
        if (!hal_i2c_tx_ready())
        {
            // Proteção contra bloqueio: Se demorar demasiado, aborta a operação
            if (++h->timeout > I2C_TIMEOUT_MAX)
            {
                hal_i2c_stop();
                h->timeout = 0;
                h->state = I2C_IDLE;
                if (h->callback)
                    h->callback(-1);
            }
            break; // Retorna e deixa o ciclo principal (super-loop) rodar
        }
        
        h->timeout = 0; // O byte foi enviado, reinicia o cronómetro de segurança
        
        // Confirma se o sensor recebeu o byte de dados corretamente
        if (!hal_i2c_get_ack())
        {
            hal_i2c_stop();
            h->state = I2C_IDLE;
            if (h->callback)
                h->callback(-1);
            break;
        }
        
        // Prepara o próximo byte. Se chegámos ao fim do tamanho (len), terminamos.
        h->index++;
        h->state = (h->index == h->len) ? I2C_STOP : I2C_WRITE;
        break;

    case I2C_READ:
        // Pede ao hardware para gerar os pulsos de relógio e "sugar" um byte (RX)
        hal_i2c_request_byte();
        h->state = I2C_WAIT_RX;
        break;

    case I2C_WAIT_RX:
        // Se o hardware ainda está a receber os bits fisicamente...
        if (!hal_i2c_rx_ready())
        {
            // Proteção contra bloqueio (mesma lógica do TX)
            if (++h->timeout > I2C_TIMEOUT_MAX)
            {
                hal_i2c_stop();
                h->timeout = 0;
                h->state = I2C_IDLE;
                if (h->callback)
                    h->callback(-1);
            }
            break;
        }
        
        h->timeout = 0;
        
        // O byte chegou! Guarda-o no nosso buffer
        h->buf[h->index] = hal_i2c_read_byte();
        h->index++;
        
        // Regra de Ouro do I2C: O Mestre DEVE enviar um NACK após o ÚLTIMO byte lido 
        // para avisar o escravo que não quer ler mais nada. Caso contrário, envia ACK.
        if (h->index == h->len)
        {
            hal_i2c_send_nack(); 
            h->state = I2C_STOP;
        }
        else
        {
            hal_i2c_send_ack();
            h->state = I2C_READ; // Volta para ir buscar mais um byte
        }
        break;

    case I2C_STOP:
        // Gera a condição de paragem no barramento para o libertar para outros
        hal_i2c_stop();
        
        // Operação terminada com sucesso (0)! Avisa o sensor/aplicação que os dados estão prontos
        if (h->callback)
            h->callback(0);

        h->state = I2C_IDLE;
        break;

    case I2C_IDLE:
        // O driver está livre. Não faz nada enquanto não houver novos pedidos.
=======
        handle_starting(h);
        break;
    case I2C_SELECT_MODE:
        handle_select_mode(h);
        break;
    case I2C_RESTART:
        handle_restart(h);
        break;
    case I2C_WAIT_RESTART:
        handle_wait_restart(h);
        break;
    case I2C_WRITE:
        handle_write(h);
        break;
    case I2C_WAIT_TX:
        handle_wait_tx(h);
        break;
    case I2C_READ:
        handle_read(h);
        break;
    case I2C_WAIT_RX:
        handle_wait_rx(h);
        break;
    case I2C_STOP:
        i2c_success(h);
        break;
    case I2C_IDLE:
>>>>>>> origin/OBC_board
        break;
    }
}