/**
 * @file hal_i2c.c
 * @brief Simulação da Hardware Abstraction Layer (HAL) para o barramento I2C.
 *
 * Este módulo não interage com hardware real. Em vez disso, simula o 
 * comportamento de vários dispositivos I2C (GNSS, IMU, Pressão, Temperatura, EPS) 
 * gerando dados aleatórios para efeitos de teste do software.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h> // para simulacao apenas !!!
#include <time.h>   // para simulacao apenas !!!
#include "hal/hal_i2c.h"
#include "config/board.h"

/**
 * @brief Macro que calcula o número de dispositivos I2C simulados.
 */
#define NUM_DEVICES (sizeof(devices) / sizeof(devices[0]))

/**
 * @brief Estrutura que representa um dispositivo I2C simulado.
 */
typedef struct
{
    uint8_t addr;      /**< Endereço I2C do dispositivo */
    uint8_t data[18];  /**< Buffer de dados simulados do dispositivo */
    uint8_t len;       /**< Comprimento dos dados a transmitir */
} i2c_device_t;

/**
 * @brief Tabela de dispositivos I2C simulados e os seus valores iniciais.
 */
static i2c_device_t devices[] = {
    {GNSS_ADDR, {0x15, 0x3F, 0x00, 0x12, 0x00, 0x03, 0x01, 0x02}, GNSS_BUF_LEN}, /* GNSS        */
    {IMU_ADDR, {0x00, 0x64, 0x00, 0xC8, 0x00, 0x32,                              /* ax=1.00  ay=2.00  az=0.50  */
                0x00, 0x0A, 0x00, 0x05, 0x00, 0x0F,                              /* gx=0.10  gy=0.05  gz=0.15  */
                0x00, 0x01, 0x00, 0x02, 0x00, 0x03},                             /* mx=0.01  my=0.02  mz=0.03  */
     IMU_BUF_LEN},                                                               /* IMU         */
    {PRESS_ADDR, {0x65, 0x00}, PRES_BUF_LEN},                                    /* Pressão     */
    {TEMP_ADDR, {0x19, 0x00}, TEMP_BUF_LEN},                                     /* Temperatura */
    {EPS_ADDR, {0xAA, 0x01}, EPS_BUF_LEN},                                       /* EPS         */
};

/**
 * @brief Estado atual do barramento I2C simulado.
 * 1 indica que o barramento está livre, 0 indica que está ocupado.
 */
static uint8_t bus_free = 1;

/**
 * @brief Regista o endereço do dispositivo com o qual o Master está a comunicar atualmente.
 */
static uint8_t active_addr = 0xFF;

/**
 * @brief Índice que controla qual o próximo byte do buffer a ser lido durante uma transação.
 */
static uint8_t read_index = 0;

/**
 * @brief Verifica se o barramento I2C está livre para uma nova comunicação.
 *
 * @return 1 se o barramento estiver livre, 0 caso contrário.
 */
int hal_i2c_bus_free(void)
{
    return bus_free;
}

/**
 * @brief Inicializa o periférico I2C (Simulação).
 *
 * Configura a *seed* para a geração de números aleatórios usados na simulação 
 * dos valores dos sensores.
 *
 * @return 1 indicando sucesso na inicialização.
 */
uint8_t hal_i2c_init(void)
{
    srand((unsigned int)time(NULL));
    return 1;
}

/**
 * @brief Atualiza os buffers de um dispositivo simulado com valores aleatórios plausíveis.
 *
 * @param i Índice do dispositivo no array `devices` a ser atualizado.
 */
static void hal_i2c_randomize(uint8_t i)
{
    switch (devices[i].addr)
    {

    case GNSS_ADDR:
        devices[i].data[0] = rand() % 90;  // latitude graus 0-90
        devices[i].data[1] = rand() % 100; // latitude decimais
        devices[i].data[2] = rand() % 180; // longitude graus
        devices[i].data[3] = rand() % 100;
        devices[i].data[4] = rand() % 4; // altitude high byte 0-1023m
        devices[i].data[5] = rand() % 256;
        devices[i].data[6] = rand() % 10; // speed
        devices[i].data[7] = rand() % 100;
        break;

    case IMU_ADDR:
        for (uint8_t j = 0; j < IMU_BUF_LEN; j++)
            devices[i].data[j] = rand() % 256;
        break;

    case PRESS_ADDR:
    {
        uint16_t hpa = 900 + rand() % 200;
        devices[i].data[0] = (hpa >> 8) & 0xFF;
        devices[i].data[1] = hpa & 0xFF;
        break;
    }

    case TEMP_ADDR:
    {
        uint16_t temp = 0 + rand() % 80; // 0-80°C
        devices[i].data[0] = (temp >> 8) & 0xFF;
        devices[i].data[1] = temp & 0xFF;
        break;
    }

    case EPS_ADDR:
        devices[i].data[0] = 60 + rand() % 60; // voltage: 6.0-12.0V (/10)
        devices[i].data[1] = rand() % 200;     // current: 0-2.0A (/100)
        break;
    }
}

/**
 * @brief Gera a condição de START no barramento I2C simulado.
 *
 * Marca o barramento como ocupado e reinicia o estado interno de comunicação.
 */
void hal_i2c_start(void)
{
    bus_free = 0;
    active_addr = 0xFF;
    read_index = 0;
}

/**
 * @brief Gera a condição de STOP no barramento I2C simulado.
 *
 * Liberta o barramento para novas transações.
 */
void hal_i2c_stop(void)
{
    bus_free = 1;
}

/**
 * @brief Lê um byte do dispositivo I2C ativo na simulação.
 *
 * Procura o dispositivo que corresponde ao `active_addr` e retorna o byte
 * atual do seu buffer, incrementando o índice de leitura.
 *
 * @return O byte lido, ou 0x00 se ultrapassar o limite, ou 0xFF se não houver dispositivo ativo.
 */
uint8_t hal_i2c_read_byte(void)
{
    for (uint8_t i = 0; i < NUM_DEVICES; i++)
    {
        if (devices[i].addr == active_addr)
        {
            if (read_index < devices[i].len)
                return devices[i].data[read_index++];
            return 0x00;
        }
    }
    return 0xFF;
}

/**
 * @brief Envia o endereço do dispositivo (com o bit R/W) para o barramento.
 *
 * Configura qual dispositivo irá comunicar e aproveita este momento para 
 * gerar dados aleatórios (`hal_i2c_randomize`) para esse sensor na simulação.
 *
 * @param byte Byte contendo o endereço I2C de 7 bits (formatado com o bit R/W no LSB).
 */
void hal_i2c_send_addr(uint8_t byte)
{
    uint8_t addr = byte >> 1;
    active_addr = addr;
    read_index = 0;

    for (uint8_t i = 0; i < NUM_DEVICES; i++)
    {
        if (devices[i].addr == addr)
        {
            hal_i2c_randomize(i);
            break;
        }
    }
}

/**
 * @brief Simula o envio de um byte de dados do Master para o Slave.
 *
 * @param byte O byte a ser transmitido (ignorado na simulação atual).
 */
void hal_i2c_send_byte(uint8_t byte)
{
    for (uint8_t i = 0; i < NUM_DEVICES; i++)
    {
        if (devices[i].addr == active_addr)
        {
            /* numa simulação de escrita podias guardar o byte aqui */
            (void)byte;
            break;
        }
    }
}

/**
 * @brief Simula a verificação do sinal de ACK (Acknowledge) do Slave.
 *
 * @return 1 se o dispositivo existir no array simulado (ACK recebido), 0 caso contrário (NACK).
 */
int hal_i2c_get_ack(void)
{
    for (uint8_t i = 0; i < NUM_DEVICES; i++)
    {
        if (devices[i].addr == active_addr)
            return 1;
    }
    return 0;
}

/**
 * @brief Simula o envio de um sinal ACK pelo Master (deixado vazio na simulação).
 */
void hal_i2c_send_ack(void)
{
}

/**
 * @brief Simula o envio de um sinal NACK pelo Master (deixado vazio na simulação).
 */
void hal_i2c_send_nack(void)
{
}

/**
 * @brief Verifica se o periférico de hardware I2C está pronto para transmitir dados.
 *
 * @return Sempre 1 na simulação (a transmissão é considerada instantânea).
 */
int hal_i2c_tx_ready(void)
{
    return 1; /* simulação: transmissão é instantânea */
}

/**
 * @brief Verifica se o periférico de hardware I2C tem um dado disponível na receção.
 *
 * @return Sempre 1 na simulação (os dados estão sempre imediatamente disponíveis).
 */
int hal_i2c_rx_ready(void)
{
    return 1; /* simulação: dado sempre disponível */
}

/**
 * @brief Simula a requisição de leitura de um byte (ativação do *clock*).
 */
void hal_i2c_request_byte(void)
{
    /* simulação: no hardware activaria o clock para receber */
}