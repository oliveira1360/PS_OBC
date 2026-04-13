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
 *
 * Valores iniciais representativos de um CubeSat 3U em LEO (~520 km):
 *
 *  GNSS   : lat=38.71°N  lon=9.14°W  alt=520 km  speed=7.78 km/s
 *  IMU    : accel=[0.03,0.02,0.99]g  gyro=[0.08,0.05,0.10]°/s  mag=[0.21,0.14,0.44]G
 *  Pressão: 1013 hPa (cabin pressure, sealed structure)
 *  Temp   : 25°C (nominal electronics operating temperature)
 *  EPS    : 8.10 V  /  1.20 A  (2S LiPo, mid-charge, typical 3U load)
 */
static i2c_device_t devices[] = {
    /* GNSS: buf = [lat_int, lat_dec, lon_int, lon_dec, alt_hi, alt_lo, spd_int, spd_dec] */
    /*       lat=38.71°  lon=9.14°  alt=520km  speed=7.78km/s                            */
    {GNSS_ADDR, {38, 71, 9, 14, 0x02, 0x08, 7, 78}, GNSS_BUF_LEN},

    /* IMU: buf[0..8] = ax,ay,az,gx,gy,gz,mx,my,mz  (val = byte / 100.0)                */
    /*      accel near 1g on Z (ground test), small noise on X/Y                         */
    /*      gyro slow rotation, mag typical LEO Earth field in Gauss                     */
    {IMU_ADDR, { 3,  2, 99,                     /* ax=0.03g  ay=0.02g  az=0.99g  */
                 8,  5, 10,                     /* gx=0.08°/s gy=0.05°/s gz=0.10°/s */
                21, 14, 44,                     /* mx=0.21G  my=0.14G  mz=0.44G  */
                 0,  0,  0,  0,  0,  0,  0,  0, 0},  /* bytes 9-17 unused         */
     IMU_BUF_LEN},

    /* Pressure: 1013 hPa big-endian  (0x03F5 = 1013)                                   */
    {PRESS_ADDR, {0x03, 0xF5}, PRES_BUF_LEN},

    /* Temperature: 25°C big-endian  (0x0019 = 25)                                      */
    {TEMP_ADDR, {0x00, 0x19}, TEMP_BUF_LEN},

    /* EPS: voltage = 81 / 10.0 = 8.10 V  /  current = 120 / 100.0 = 1.20 A            */
    {EPS_ADDR, {81, 120}, EPS_BUF_LEN},
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
 * @brief Contador de passos orbitais para simulação de movimento do CubeSat.
 *
 * Incrementado a cada leitura do GNSS. Simula uma órbita LEO com inclinação
 * de ~51.6° (tipo ISS). Um passo ≈ 1 segundo de telemetria, período orbital
 * completo ≈ 6000 s (100 min).
 */
static uint16_t orbit_step = 0U;

/**
 * @brief Atualiza os buffers de um dispositivo simulado com valores realistas.
 *
 * Todos os valores foram calibrados para um CubeSat 3U em Low Earth Orbit (LEO):
 *
 *  GNSS   : movimento orbital contínuo, altitude 515-525 km, v=7.75-7.84 km/s
 *  IMU    : acelerómetro ~1g (eixo Z, teste em solo), giroscópio baixo ruído,
 *           magnetómetro com campo terrestre típico de LEO (~0.2-0.5 Gauss)
 *  Pressão: 1010-1016 hPa (pressão interna de caixa selada, variação térmica)
 *  Temp   : 22-37°C (temperatura de operação nominal da eletrónica)
 *  EPS    : 7.60-8.35 V (2S LiPo, ciclo carga/descarga), 0.80-1.60 A (carga típica)
 *
 * @param i Índice do dispositivo no array `devices` a ser atualizado.
 */
static void hal_i2c_randomize(uint8_t i)
{
    switch (devices[i].addr)
    {

    /*
     * GNSS — formato: [lat_int, lat_dec, lon_int, lon_dec, alt_hi, alt_lo, spd_int, spd_dec]
     *
     * Latitude : onda triangular 0→51→0 (inclinação 51.6°, tipo ISS)
     * Longitude: avança 2° por passo (satélite move-se ~500 km/min em solo)
     * Altitude : 515–525 km LEO, armazenada em km no uint16_t
     * Velocidade: 7.75–7.84 km/s (velocidade orbital LEO)
     */
    case GNSS_ADDR:
    {
        /* Passo orbital: período simulado = 512 passos por semi-órbita */
        orbit_step = (orbit_step + 1U) & 0x3FFU; /* 0..1023 */

        /* Latitude: triângulo 0°→51°→0° em 512 passos */
        uint16_t half = orbit_step & 0x1FFU;                   /* 0..511 */
        uint8_t  lat_int = (uint8_t)((half < 256U)
                           ? (half * 51U / 256U)               /* sobe 0→51 */
                           : (51U - (half - 256U) * 51U / 256U)); /* desce 51→0 */
        uint8_t  lat_dec = (uint8_t)(rand() % 100);

        /* Longitude: avança 2° por passo, wrap 0–179° */
        uint8_t  lon_int = (uint8_t)((orbit_step * 2U) % 180U);
        uint8_t  lon_dec = (uint8_t)(rand() % 100);

        /* Altitude: 515–525 km em big-endian */
        uint16_t alt_km  = 518U + (uint16_t)(rand() % 8);
        uint8_t  alt_hi  = (uint8_t)(alt_km >> 8U);
        uint8_t  alt_lo  = (uint8_t)(alt_km & 0xFFU);

        /* Velocidade orbital: 7.75–7.84 km/s */
        uint8_t  spd_dec = (uint8_t)(75U + rand() % 10U);

        devices[i].data[0] = lat_int;
        devices[i].data[1] = lat_dec;
        devices[i].data[2] = lon_int;
        devices[i].data[3] = lon_dec;
        devices[i].data[4] = alt_hi;
        devices[i].data[5] = alt_lo;
        devices[i].data[6] = 7U;      /* km/s inteiro */
        devices[i].data[7] = spd_dec;
        break;
    }

    /*
     * IMU — formato: buf[0..8] = ax,ay,az,gx,gy,gz,mx,my,mz  (val = byte / 100.0)
     *
     * Acelerómetro (g):
     *   ax,ay : ruído vibração ±0.05 g  → 0–5
     *   az    : ~1g (teste em solo)     → 97–102
     * Giroscópio (°/s):
     *   gx,gy,gz: rotação lenta         → 3–15
     * Magnetómetro (Gauss, campo terrestre LEO ~0.2–0.5 G):
     *   mx : 0.18–0.25 G               → 18–25
     *   my : 0.10–0.16 G               → 10–16
     *   mz : 0.40–0.48 G               → 40–48
     */
    case IMU_ADDR:
        devices[i].data[0] = (uint8_t)(1U + rand() % 5U);   /* ax: 0.01–0.05 g      */
        devices[i].data[1] = (uint8_t)(1U + rand() % 4U);   /* ay: 0.01–0.04 g      */
        devices[i].data[2] = (uint8_t)(97U + rand() % 6U);  /* az: 0.97–1.02 g (1g) */
        devices[i].data[3] = (uint8_t)(4U + rand() % 8U);   /* gx: 0.04–0.11 °/s   */
        devices[i].data[4] = (uint8_t)(3U + rand() % 6U);   /* gy: 0.03–0.08 °/s   */
        devices[i].data[5] = (uint8_t)(6U + rand() % 9U);   /* gz: 0.06–0.14 °/s   */
        devices[i].data[6] = (uint8_t)(18U + rand() % 8U);  /* mx: 0.18–0.25 G     */
        devices[i].data[7] = (uint8_t)(10U + rand() % 7U);  /* my: 0.10–0.16 G     */
        devices[i].data[8] = (uint8_t)(40U + rand() % 9U);  /* mz: 0.40–0.48 G     */
        /* bytes 9-17 não são parseados pelo imu_parse(), mantêm-se a 0 */
        break;

    /*
     * Pressure — pressão interna da caixa selada do CubeSat (hPa, big-endian)
     * Variação térmica ±3 hPa em torno de 1013 hPa atmosférico
     */
    case PRESS_ADDR:
    {
        uint16_t hpa = 1010U + (uint16_t)(rand() % 7U); /* 1010–1016 hPa */
        devices[i].data[0] = (uint8_t)(hpa >> 8U);
        devices[i].data[1] = (uint8_t)(hpa & 0xFFU);
        break;
    }

    /*
     * Temperature — temperatura interna da eletrónica (°C, big-endian)
     * Faixa nominal de operação: 22–37°C
     */
    case TEMP_ADDR:
    {
        uint16_t temp_c = 22U + (uint16_t)(rand() % 16U); /* 22–37°C */
        devices[i].data[0] = (uint8_t)(temp_c >> 8U);
        devices[i].data[1] = (uint8_t)(temp_c & 0xFFU);
        break;
    }

    /*
     * EPS — Electric Power System: bateria 2S LiPo, carga típica de CubeSat 3U
     *
     * Tensão  (V)  = byte[0] / 10.0   → 7.60–8.35 V  (75–84, range 2S LiPo)
     * Corrente(A)  = byte[1] / 100.0  → 0.80–1.60 A  (80–160, carga nominal)
     */
    case EPS_ADDR:
        devices[i].data[0] = (uint8_t)(33U + rand() % 10U); /* 7.50–8.40 V  */
        devices[i].data[1] = (uint8_t)(80U + rand() % 81U); /* 0.80–1.60 A  */
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