/**
 * @file imu.c
 * @brief Driver para a IMU (Inertial Measurement Unit).
 *
 * Gere a comunicação I2C assíncrona e a conversão de dados brutos
 * recebidos do sensor inercial (Acelerómetro, Giroscópio e Magnetómetro).
 */

#include <stdio.h>
#include "peripherals/imu.h"
#include "drivers/i2c_driver.h"
#include "config/board.h"
#include "app/sensors.h"

/**
 * @brief Handle de controlo para a comunicação I2C com a IMU.
 */
static i2c_handle_t i2c = {0};

/**
 * @brief Buffer para armazenar os dados brutos recebidos via I2C da IMU.
 */
static uint8_t buf[IMU_BUF_LEN];

/**
 * @brief Analisa os dados brutos recebidos da IMU.
 *
 * Converte o array de bytes em valores físicos (float) para os eixos X, Y e Z
 * do acelerómetro (ax, ay, az), giroscópio (gx, gy, gz) e magnetómetro (mx, my, mz),
 * atualizando a estrutura global `imu`.
 *
 * @param buf Ponteiro para o buffer de dados a ser analisado.
 */
static void imu_parse(uint8_t *buf)
{
    imu.ax = (float)buf[0] / 100.0f;
    imu.ay = (float)buf[1] / 100.0f;
    imu.az = (float)buf[2] / 100.0f;

    imu.gx = (float)buf[3] / 100.0f;
    imu.gy = (float)buf[4] / 100.0f;
    imu.gz = (float)buf[5] / 100.0f;

    imu.mx = (float)buf[6] / 100.0f;
    imu.my = (float)buf[7] / 100.0f;
    imu.mz = (float)buf[8] / 100.0f;
}

/**
 * @brief Callback executado após a conclusão da leitura I2C.
 *
 * Se a comunicação for bem-sucedida, aciona a função de parsing
 * para interpretar os dados inerciais recebidos.
 *
 * @param result O resultado da operação I2C (0 indica sucesso).
 */
static void on_imu_done(int result)
{
    printf("IMU cb: result=%d buf=%02X %02X %02X %02X %02X %02X\n",
           result, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
    if (result == 0)
        imu_parse(buf);
}

/**
 * @brief Inicia uma leitura assíncrona da IMU via I2C.
 *
 * Configura o handle I2C e inicia a máquina de estados para ler
 * os dados dos sensores. Se já existir uma leitura em curso, a função
 * retorna silenciosamente.
 */
void imu_read_async(void)
{
    if (i2c.state != I2C_IDLE)
        return;

    i2c.addr     = IMU_ADDR;
    i2c.buf      = buf;
    i2c.len      = 6;        /* accel: 6 bytes a partir de 0x3B */
    i2c.rw       = 1;
    i2c.reg      = 0x3B;
    i2c.use_reg  = 1;
    i2c.index    = 0;
    i2c.timeout  = 0;
    i2c.callback = on_imu_done;
    i2c.state    = I2C_STARTING;
}

/**
 * @brief Atualiza a máquina de estados I2C da IMU.
 *
 * Esta função deve ser chamada periodicamente (por exemplo, no loop
 * principal) para processar os eventos I2C em background.
 */
void imu_tick(void)
{
    i2c_tick(&i2c);
}