/**
 * @file gnss.c
 * @brief Driver para o módulo GNSS (Global Navigation Satellite System).
 *
 * Gere a comunicação I2C assíncrona com o sensor GNSS e a conversão dos 
 * dados brutos recebidos (coordenadas, altitude e velocidade).
 */
#include <stdio.h>
#include "peripherals/gnss.h"
#include "drivers/i2c_driver.h"
#include "config/board.h"
#include "app/sensors.h"

/**
 * @brief Handle de controlo para a comunicação I2C com o módulo GNSS.
 */
static i2c_handle_t i2c = {0};

/**
 * @brief Buffer para armazenar os dados brutos recebidos via I2C do GNSS.
 */
static uint8_t buf[GNSS_BUF_LEN];


static float bytes_to_float(uint8_t *b)
{
    union { float f; uint8_t bytes[4]; } u;
    u.bytes[3] = b[0];  /* big-endian → little-endian */
    u.bytes[2] = b[1];
    u.bytes[1] = b[2];
    u.bytes[0] = b[3];
    return u.f;
}

/**
 * @brief Analisa os dados brutos recebidos do GNSS.
 *
 * Converte o array de bytes em coordenadas geográficas (latitude e longitude),
 * altitude e velocidade, atualizando a estrutura global `gnss`.
 *
 * @param buf Ponteiro para o buffer de dados a ser analisado.
 */
static void gnss_parse(uint8_t *buf)
{
    printf("GNSS raw:");
    for (int i = 0; i < 18; i++)
        printf(" %02X", buf[i]);
    printf("\n");

    gnss.latitude  = bytes_to_float(&buf[0]);
    gnss.longitude = bytes_to_float(&buf[4]);
    gnss.altitude  = bytes_to_float(&buf[8]);
    gnss.speed     = bytes_to_float(&buf[12]);
}

/**
 * @brief Callback executado após a conclusão da leitura I2C.
 *
 * Se a comunicação for bem-sucedida, aciona a função de parsing
 * para interpretar os dados de localização recebidos.
 *
 * @param result O resultado da operação I2C (0 indica sucesso).
 */
static void on_gnss_done(int result)
{
    if (result == 0)
    {
        gnss_parse(buf);
        printf("GNSS: lat=%.2f lon=%.2f alt=%.2f spd=%.2f\n",
               gnss.latitude, gnss.longitude, gnss.altitude, gnss.speed);
    }
    else
    {
        printf("GNSS: I2C erro=%d\n", result);
    }
}

/**
 * @brief Inicia uma leitura assíncrona do GNSS via I2C.
 *
 * Configura o handle I2C e inicia a máquina de estados para ler
 * os dados do sensor GNSS. Se já existir uma leitura em curso, 
 * a função retorna silenciosamente.
 */
void gnss_read_async(void)
{
    if (i2c.state != I2C_IDLE)
        return;

    i2c.addr = GNSS_ADDR;
    i2c.buf = buf;
    i2c.len = GNSS_BUF_LEN;
    i2c.rw = 1;
    i2c.reg = 0xFF;      /* registo default = leitura completa */
    i2c.use_reg = 1;     /* ativa write-restart-read */
    i2c.index = 0;
    i2c.timeout = 0;
    i2c.callback = on_gnss_done;
    i2c.state = I2C_STARTING;
}

/**
 * @brief Atualiza a máquina de estados I2C do GNSS.
 *
 * Esta função deve ser chamada periodicamente (por exemplo, no loop 
 * principal) para processar os eventos I2C em background.
 */
void gnss_tick(void)
{
    i2c_tick(&i2c);
}