/**
 * @file pressure.c
 * @brief Driver para o sensor de pressão.
 *
<<<<<<< HEAD
 * Gere a comunicação I2C assíncrona com o sensor de pressão e a 
=======
 * Gere a comunicação I2C assíncrona com o sensor de pressão e a
>>>>>>> origin/OBC_board
 * conversão dos dados brutos em valores de pressão reais.
 */

#include <stdio.h>
#include "peripherals/pressure.h"
#include "drivers/i2c_driver.h"
#include "config/board.h"
#include "app/sensors.h"

/**
 * @brief Handle de controlo para a comunicação I2C com o sensor de pressão.
 */
static i2c_handle_t i2c = {0};

/**
 * @brief Buffer para armazenar os dados brutos recebidos via I2C do sensor.
 */
static uint8_t buf[PRES_BUF_LEN];

/**
 * @brief Analisa os dados brutos recebidos do sensor de pressão.
 *
 * Junta os dois bytes lidos (o byte mais significativo e o menos significativo)
<<<<<<< HEAD
 * num valor de 16 bits, e converte-o para `float`, atualizando a 
=======
 * num valor de 16 bits, e converte-o para `float`, atualizando a
>>>>>>> origin/OBC_board
 * estrutura global `pressure`.
 *
 * @param buf Ponteiro para o buffer de dados a ser analisado.
 */
static void pressure_parse(uint8_t *buf)
{
    float val = (float)((buf[0] << 8) | buf[1]);
    pressure.pressure = val;
}

/**
 * @brief Callback executado após a conclusão da leitura I2C.
 *
 * Se a comunicação for bem-sucedida, aciona a função de parsing
 * para interpretar os dados de pressão recebidos.
 *
 * @param result O resultado da operação I2C (0 indica sucesso).
 */
static void on_pressure_done(int result)
{
<<<<<<< HEAD
=======
    //printf("pressure done status: %d", result);
>>>>>>> origin/OBC_board
    if (result == 0)
        pressure_parse(buf);
}

/**
 * @brief Inicia uma leitura assíncrona do sensor de pressão via I2C.
 *
 * Configura o handle I2C e inicia a máquina de estados para ler
<<<<<<< HEAD
 * os dados do sensor. Se já existir uma leitura em curso, a função 
=======
 * os dados do sensor. Se já existir uma leitura em curso, a função
>>>>>>> origin/OBC_board
 * retorna silenciosamente.
 */
void pressure_read_async()
{
    if (i2c.state != I2C_IDLE)
<<<<<<< HEAD
        return;  // já está a ler 

    i2c.addr     = PRESS_ADDR;
    i2c.buf      = buf;
    i2c.len      = PRES_BUF_LEN;
    i2c.rw       = 1;
    i2c.index    = 0;
    i2c.callback = on_pressure_done;
    i2c.state    = I2C_STARTING;
=======
        return; // já está a ler

    i2c.addr = PRESS_ADDR;
    i2c.buf = buf;
    i2c.len = PRES_BUF_LEN;
    i2c.rw = 1;
    i2c.reg = 0xF7;
    i2c.use_reg = 1;
    i2c.index = 0;
    i2c.timeout = 0;
    i2c.callback = on_pressure_done;
    i2c.state = I2C_STARTING;
>>>>>>> origin/OBC_board
}

/**
 * @brief Atualiza a máquina de estados I2C do sensor de pressão.
 *
<<<<<<< HEAD
 * Esta função deve ser chamada periodicamente (por exemplo, no loop 
=======
 * Esta função deve ser chamada periodicamente (por exemplo, no loop
>>>>>>> origin/OBC_board
 * principal) para processar os eventos I2C em background.
 */
void pressure_tick(void)
{
    i2c_tick(&i2c);
}