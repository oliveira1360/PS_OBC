/**
 * @file temperature.c
 * @brief Driver para o sensor de temperatura.
 *
 * Gere a comunicação I2C assíncrona com o sensor de temperatura e a 
 * conversão dos dados brutos num valor real de temperatura.
 */

#include <stdio.h>
#include "peripherals/temperature.h"
#include "drivers/i2c_driver.h"
#include "config/board.h"
#include "app/sensors.h"

/**
 * @brief Handle de controlo para a comunicação I2C com o sensor de temperatura.
 */
static i2c_handle_t i2c = {0};

/**
 * @brief Buffer para armazenar os dados brutos recebidos via I2C do sensor.
 */
static uint8_t buf[TEMP_BUF_LEN];

/**
 * @brief Analisa os dados brutos recebidos do sensor de temperatura.
 *
 * Junta os dois bytes lidos (byte mais significativo e menos significativo)
 * num valor de 16 bits, e converte-o para `float`, atualizando a 
 * estrutura global `temperature`.
 *
 * @param buf Ponteiro para o buffer de dados a ser analisado.
 */
static void temperature_parse(uint8_t *buf)
{
<<<<<<< HEAD
=======
    // printf("raw temperature: %02X", *buf);
>>>>>>> origin/OBC_board
    temperature.temperature = (float)((buf[0] << 8) | buf[1]);
}

/**
 * @brief Callback executado após a conclusão da leitura I2C.
 *
 * Se a comunicação for bem-sucedida, aciona a função de parsing
 * para interpretar os dados de temperatura recebidos.
 *
 * @param result O resultado da operação I2C (0 indica sucesso).
 */
static void on_temp_done(int result)
{
<<<<<<< HEAD
=======
    //printf("TEMP cb: result=%d buf=%02X %02X\n", result, buf[0], buf[1]);
>>>>>>> origin/OBC_board
    if (result == 0)
        temperature_parse(buf);
}

/**
 * @brief Inicia uma leitura assíncrona do sensor de temperatura via I2C.
 *
 * Configura o handle I2C e inicia a máquina de estados para ler
 * os dados do sensor. Se já existir uma leitura em curso, a função 
 * retorna silenciosamente.
 */
<<<<<<< HEAD
void temperature_read_async()
{
    if (i2c.state != I2C_IDLE)
        return; // já está a ler

    i2c.addr = TEMP_ADDR;
    i2c.buf = buf;
    i2c.len = TEMP_BUF_LEN;
    i2c.rw = 1;
    i2c.index = 0;
    i2c.callback = on_temp_done;
    i2c.state = I2C_STARTING;
=======
void temperature_read_async(void)
{
    if (i2c.state != I2C_IDLE)
        return;

    i2c.addr     = TEMP_ADDR;
    i2c.buf      = buf;
    i2c.len      = TEMP_BUF_LEN;
    i2c.rw       = 1;
    i2c.reg      = 0;
    i2c.use_reg  = 1;
    i2c.index    = 0;
    i2c.timeout  = 0;
    i2c.callback = on_temp_done;
    i2c.state    = I2C_STARTING;
>>>>>>> origin/OBC_board
}

/**
 * @brief Atualiza a máquina de estados I2C do sensor de temperatura.
 *
 * Esta função deve ser chamada periodicamente (por exemplo, no loop 
 * principal) para processar os eventos I2C em background.
 */
void temperature_tick(void)
{
    i2c_tick(&i2c);
}