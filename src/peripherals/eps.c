/**
 * @file eps.c
 * @brief Driver para o Electrical Power System (EPS).
 *
 * Gere a comunicação I2C assíncrona e a conversão de dados brutos 
 * recebidos do sistema de energia.
 */

#include "peripherals/eps.h"
#include "drivers/i2c_driver.h"
#include "config/board.h"
#include "app/sensors.h"
#include <stdio.h>


/**
 * @brief Handle de controlo para a comunicação I2C com o EPS.
 */
static i2c_handle_t i2c = {0};

/**
 * @brief Buffer para armazenar os dados brutos recebidos via I2C do EPS.
 */
static uint8_t buf[EPS_BUF_LEN];

/**
 * @brief Analisa os dados brutos recebidos do EPS.
 *
 * Converte os bytes lidos em grandezas físicas (Tensão em Volts e 
 * Corrente em Amperes) e atualiza a estrutura global `eps`.
 *
 * @param buf Ponteiro para o buffer de dados a ser analisado.
 */
static void eps_parse(uint8_t *buf)
{
    eps.voltage = (float)buf[0] / 10.0f;  // 0xAA = 170 → 17.0V
    eps.current = (float)buf[1] / 100.0f; // 0x01 = 1   → 0.01A
}

/**
 * @brief Callback executado após a conclusão da leitura I2C.
 *
 * Se a comunicação for bem-sucedida, aciona a função de parsing
 * para interpretar os dados recebidos.
 *
 * @param result O resultado da operação I2C (0 indica sucesso).
 */
static void on_eps_done(int result)
{
    printf("EPS cb: result=%d buf=%02X %02X\n", result, buf[0], buf[1]);
    if (result == 0)
        eps_parse(buf);
}

/**
 * @brief Inicia uma leitura assíncrona do EPS via I2C.
 *
 * Configura o handle I2C e inicia a máquina de estados para ler
 * os dados do EPS. Se já existir uma leitura em curso, a função 
 * retorna silenciosamente.
 */
void eps_read_async(void)
{
    if (i2c.state != I2C_IDLE)
    {
        printf("EPS: busy state=%d\n", i2c.state);
        return;
    }

    printf("EPS: starting read addr=0x%02X reg=0x%02X\n", EPS_ADDR, 0x09);

    i2c.addr     = EPS_ADDR;
    i2c.buf      = buf;
    i2c.len      = EPS_BUF_LEN;
    i2c.rw       = 1;
    i2c.reg      = 0x09;
    i2c.use_reg  = 1;
    i2c.index    = 0;
    i2c.timeout  = 0;
    i2c.callback = on_eps_done;
    i2c.state    = I2C_STARTING;
}

/**
 * @brief Atualiza a máquina de estados I2C do EPS.
 *
 * Esta função deve ser chamada periodicamente (por exemplo, no loop 
 * principal) para processar os eventos I2C em background.
 */
void eps_tick(void)
{
    i2c_tick(&i2c);
}