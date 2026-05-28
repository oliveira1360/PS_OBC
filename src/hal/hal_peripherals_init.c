/**
 * @file hal_peripherals_init.c
 * @brief Inicialização dos periféricos de hardware na camada HAL.
 *
 * Este módulo contém as rotinas de inicialização para os vários sensores
 * e subsistemas da placa (GNSS, IMU, Pressão, Temperatura, EPS, TT&C e 
 * Memória Externa). Na configuração atual (simulação/teste), as funções 
 * atuam como "stubs" e retornam sempre sucesso (1).
 */

#include "hal/hal_peripherals_init.h"
#include "hal/hal_i2c.h"
#include "drivers/i2c_driver.h"
#include "config/board.h"
#include "peripherals/ext_memory.h"
#include <stdint.h>

/**
 * @brief Inicializa o periférico do módulo GNSS.
 *
 * @return 1 indicando sucesso na inicialização.
 */
uint8_t hal_gnss_init(void)
{
    return 1;
}

/**
 * @brief Inicializa o periférico da IMU (Inertial Measurement Unit).
 *
 * @return 1 indicando sucesso na inicialização.
 */
uint8_t hal_imu_init(void)
{
    return 1;
}

/**
 * @brief Inicializa o sensor de pressão.
 *
 * @return 1 indicando sucesso na inicialização.
 */
uint8_t hal_pressure_init(void)
{
    return 1;
}

/**
 * @brief Inicializa o sensor de temperatura.
 *
 * @return 1 indicando sucesso na inicialização.
 */
uint8_t hal_temperature_init(void)
{
    return 1;
}

/**
 * @brief Inicializa o sistema de energia (EPS - Electrical Power System).
 *
 * @return 1 indicando sucesso na inicialização.
 */
uint8_t hal_eps_init(void)
{
    return 1;
}

/**
 * @brief Inicializa o subsistema de Telemetria, Rastreio e Comando (TT&C).
 *
 * @return 1 indicando sucesso na inicialização.
 */
uint8_t hal_ttc_init(void)
{
    return 1;
}

/**
 * @brief Inicializa o controlador da memória externa (Ex: Flash/EEPROM SPI ou I2C).
 *
 * @return 1 indicando sucesso na inicialização.
 */
uint8_t hal_ext_memory_init(void)
{
    ExtMem_Init();
}