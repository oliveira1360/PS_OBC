/**
 * @file hal_i2c.c
<<<<<<< HEAD
 * @brief Simulação da Hardware Abstraction Layer (HAL) para o barramento I2C.
 *
 * Este módulo não interage com hardware real. Em vez disso, simula o 
 * comportamento de vários dispositivos I2C (GNSS, IMU, Pressão, Temperatura, EPS) 
 * gerando dados aleatórios para efeitos de teste do software.
=======
 * @brief Hardware Abstraction Layer para o barramento I2C.
 *
 * Usa USE_REAL_HW em board.h:
 *   0 → dados simulados internamente (sem hardware)
 *   1 → hardware real via TWIHS0 do ATSAMV71Q21 (EXT1: PA03=SDA, PA04=SCL)
 *
 * Todos os nomes de registos e bits vêm de hal_i2c.h.
>>>>>>> origin/OBC_board
 */

#include <stdio.h>
#include <stdint.h>
<<<<<<< HEAD
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
=======
#include "hal/hal_i2c.h"
#include "config/board.h"

#if !USE_REAL_HW
#include <stdlib.h>
#include <time.h>
#endif

#if USE_REAL_HW

/* ============================================================
 * Definições adicionais para hardware real
 *
 * PA03 = TWD0  (SDA) — Peripheral A
 * PA04 = TWCK0 (SCL) — Peripheral A
 * ============================================================ */

/* PMC */
#define PMC_BASE        0x400E0600UL
#define PMC_PCER0       (*(volatile uint32_t *)(PMC_BASE + 0x10U))

/* PIOA */
#define PIOA_BASE       0x400E0E00UL
#define PIOA_PER        (*(volatile uint32_t *)(PIOA_BASE + 0x00U))
#define PIOA_PDR        (*(volatile uint32_t *)(PIOA_BASE + 0x04U))
#define PIOA_OER        (*(volatile uint32_t *)(PIOA_BASE + 0x10U))
#define PIOA_ODR        (*(volatile uint32_t *)(PIOA_BASE + 0x14U)) /* <-- ADICIONAR ESTA LINHA */
#define PIOA_SODR       (*(volatile uint32_t *)(PIOA_BASE + 0x30U))
#define PIOA_CODR       (*(volatile uint32_t *)(PIOA_BASE + 0x34U))
#define PIOA_PDSR       (*(volatile uint32_t *)(PIOA_BASE + 0x3CU)) /* <-- E ADICIONAR ESTA LINHA */
#define PIOA_ABCDSR0    (*(volatile uint32_t *)(PIOA_BASE + 0x70U))
#define PIOA_ABCDSR1    (*(volatile uint32_t *)(PIOA_BASE + 0x74U))

/* Pin masks */
#define PIO_SDA         (1UL << 3)   /* PA03 */
#define PIO_SCL         (1UL << 4)   /* PA04 */

/* TWIHS_MMR */
#define TWI_MMR_DADR_SHIFT  16U
#define TWI_MMR_MREAD       (1UL << 12)

/* TWIHS_CWGR */
#define TWI_CWGR_CKDIV_SHIFT  16U
#define TWI_CWGR_CHDIV_SHIFT   8U
#define TWI_CWGR_CLDIV_SHIFT   0U

/* Clock: MCK ~4 MHz, CKDIV=0, CLDIV=CHDIV=39 → ~50 kHz */
#define MCK_HZ              4000000UL
#define I2C_SPEED_HZ        ((uint32_t)I2C_SPEED_KHZ * 1000UL)
#define TWI_CKDIV           0UL
#define TWI_CLDIV           ((MCK_HZ / I2C_SPEED_HZ) - 3UL)
#define TWI_CHDIV           TWI_CLDIV

#define TWI_CWGR_VALUE      ((TWI_CKDIV << TWI_CWGR_CKDIV_SHIFT) | \
                             (TWI_CHDIV << TWI_CWGR_CHDIV_SHIFT) | \
                             (TWI_CLDIV << TWI_CWGR_CLDIV_SHIFT))

/* Timeouts */
#define TWI_STOP_TIMEOUT    100000UL
#define BUS_RECOVERY_DELAY  100

#endif /* USE_REAL_HW */
#if !USE_REAL_HW 
#define NUM_DEVICES (sizeof(devices) / sizeof(devices[0]))

typedef struct
{
    uint8_t addr;
    uint8_t data[18];
    uint8_t len;
} i2c_device_t;

static i2c_device_t devices[] = {
    {GNSS_ADDR, {38, 71, 9, 14, 0x02, 0x08, 7, 78, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, GNSS_BUF_LEN},
    {IMU_ADDR, {3, 2, 99, 8, 5, 10, 21, 14, 44, 0, 0, 0, 0, 0, 0, 0, 0, 0}, IMU_BUF_LEN},
    {PRESS_ADDR, {0x03, 0xF5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, PRES_BUF_LEN},
    {TEMP_ADDR, {0x00, 0x19, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, TEMP_BUF_LEN},
    {EPS_ADDR, {81, 120, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, EPS_BUF_LEN},
};

static uint8_t bus_free = 1U;
static uint8_t active_addr = 0xFFU;
static uint8_t read_index = 0U;
static uint16_t orbit_step = 0U;

>>>>>>> origin/OBC_board
static void hal_i2c_randomize(uint8_t i)
{
    switch (devices[i].addr)
    {
<<<<<<< HEAD

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
=======
    case GNSS_ADDR:
    {
        orbit_step = (orbit_step + 1U) & 0x3FFU;
        uint16_t half = orbit_step & 0x1FFU;
        uint8_t lat_int = (half < 256U)
                              ? (uint8_t)(half * 51U / 256U)
                              : (uint8_t)(51U - (half - 256U) * 51U / 256U);
        uint16_t alt_km = 518U + (uint16_t)(rand() % 8);
        devices[i].data[0] = lat_int;
        devices[i].data[1] = (uint8_t)(rand() % 100);
        devices[i].data[2] = (uint8_t)((orbit_step * 2U) % 180U);
        devices[i].data[3] = (uint8_t)(rand() % 100);
        devices[i].data[4] = (uint8_t)(alt_km >> 8U);
        devices[i].data[5] = (uint8_t)(alt_km & 0xFFU);
        devices[i].data[6] = 7U;
        devices[i].data[7] = (uint8_t)(75U + rand() % 10U);
        break;
    }
    case IMU_ADDR:
        devices[i].data[0] = (uint8_t)(1U + rand() % 5U);
        devices[i].data[1] = (uint8_t)(1U + rand() % 4U);
        devices[i].data[2] = (uint8_t)(97U + rand() % 6U);
        devices[i].data[3] = (uint8_t)(4U + rand() % 8U);
        devices[i].data[4] = (uint8_t)(3U + rand() % 6U);
        devices[i].data[5] = (uint8_t)(6U + rand() % 9U);
        devices[i].data[6] = (uint8_t)(18U + rand() % 8U);
        devices[i].data[7] = (uint8_t)(10U + rand() % 7U);
        devices[i].data[8] = (uint8_t)(40U + rand() % 9U);
        break;
    case PRESS_ADDR:
    {
        uint16_t hpa = 1010U + (uint16_t)(rand() % 7U);
>>>>>>> origin/OBC_board
        devices[i].data[0] = (uint8_t)(hpa >> 8U);
        devices[i].data[1] = (uint8_t)(hpa & 0xFFU);
        break;
    }
<<<<<<< HEAD

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
=======
    case TEMP_ADDR:
    {
        uint16_t temp = 22U + (uint16_t)(rand() % 16U);
        devices[i].data[0] = (uint8_t)(temp >> 8U);
        devices[i].data[1] = (uint8_t)(temp & 0xFFU);
        break;
    }
    case EPS_ADDR:
        devices[i].data[0] = (uint8_t)(33U + rand() % 10U);
        devices[i].data[1] = (uint8_t)(80U + rand() % 81U);
        break;
    default:
>>>>>>> origin/OBC_board
        break;
    }
}

<<<<<<< HEAD
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
=======
#endif /* !USE_REAL_HW */

/* ==========================================================================
 * IMPLEMENTAÇÕES DAS FUNÇÕES HAL
 * ========================================================================== */

/**
 * @brief Inicializa o periférico I2C.
 *
 * USE_REAL_HW=1: Configura TWIHS0 como master a 400 kHz nos pinos PA03/PA04.
 * USE_REAL_HW=0: Inicializa seed para simulação.
 *
 * @return 1 em caso de sucesso.
 */
uint8_t hal_i2c_init(void)
{
#if USE_REAL_HW
    PMC_PCER0 = (1UL << ID_TWI0);

    PIOA_PDR = PIO_SDA | PIO_SCL;
    PIOA_ABCDSR0 &= ~(PIO_SDA | PIO_SCL);
    PIOA_ABCDSR1 &= ~(PIO_SDA | PIO_SCL);

    TWI0_CR = TWI_CR_SWRST;
    TWI0_CR = TWI_CR_MSEN | TWI_CR_SVDIS;
    (void)TWI0_SR;

    TWI0_CWGR = TWI_CWGR_VALUE;
    return 1U;
#else
    srand((unsigned int)time(NULL));
    return 1U;
#endif
}

/**
 * @brief Verifica se o barramento I2C está livre.
 * @return 1 se livre, 0 se ocupado.
 */
int hal_i2c_bus_free(void)
{
#if USE_REAL_HW
    uint32_t sr = TWI0_SR;
    /* TXCOMP=1 ou TXRDY=1 indica bus livre */
    return ((sr & (TWI_SR_TXCOMP | TWI_SR_TXRDY)) != 0U) ? 1 : 0;
#else
    return (int)bus_free;
#endif
}

/**
 * @brief Gera condição de START no barramento.
 *
 * No TWIHS do SAM V71 o START é gerado automaticamente ao configurar
 * o MMR e escrever no THR (TX) ou ao setar CR.START (RX).
 */
void hal_i2c_start(void)
{
#if USE_REAL_HW
    /* Limpa byte residual e flags */
    (void)(TWI0_RHR);
    (void)(TWI0_SR);
#else
    bus_free = 0U;
    active_addr = 0xFFU;
    read_index = 0U;
#endif
}

/**
 * @brief Gera condição de STOP no barramento.
 */
void hal_i2c_stop(void)
{
#if USE_REAL_HW
    TWI0_CR = TWI_CR_STOP;
    uint32_t timeout = TWI_STOP_TIMEOUT;
    while ((TWI0_SR & TWI_SR_TXCOMP) == 0U)
    {
        if (--timeout == 0U)
        {
            TWI0_CR = TWI_CR_SWRST;
            TWI0_CR = TWI_CR_MSEN | TWI_CR_SVDIS;
            break;
        }
    }
#else
    bus_free = 1U;
#endif
}

/**
 * @brief Envia o endereço do dispositivo com bit R/W.
 *
 * No hardware real configura o TWIHS_MMR com o endereço de 7 bits
 * e a direção (MREAD para leitura).
 *
 * @param byte Endereço I2C (7 bits << 1) | bit R/W no LSB.
 */
void hal_i2c_send_addr(uint8_t byte)
{
#if USE_REAL_HW
    uint8_t addr = byte >> 1U;
    uint8_t rw = byte & 0x01U;

    /* Configura MMR: endereço slave + direção */
    TWI0_MMR = ((uint32_t)addr << TWI_MMR_DADR_SHIFT) | (rw ? TWI_MMR_MREAD : 0UL);

    if (rw)
    {
        /* Modo leitura: gera START */
        TWI0_CR = TWI_CR_START;
    }
#else
    uint8_t addr = byte >> 1U;
    active_addr = addr;
    read_index = 0U;
    for (uint8_t i = 0U; i < NUM_DEVICES; i++)
>>>>>>> origin/OBC_board
    {
        if (devices[i].addr == addr)
        {
            hal_i2c_randomize(i);
            break;
        }
    }
<<<<<<< HEAD
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
=======
#endif
}

/**
 * @brief Envia um byte de dados para o slave.
 * @param byte Byte a transmitir.
 */
void hal_i2c_send_byte(uint8_t byte)
{
#if USE_REAL_HW
    TWI0_THR = (uint32_t)byte;
#else
    (void)byte;
#endif
}

/**
 * @brief Verifica se o TX está pronto para o próximo byte.
 * @return 1 se pronto, 0 se ocupado.
 */
int hal_i2c_tx_ready(void)
{
#if USE_REAL_HW
    return ((TWI0_SR & TWI_SR_TXRDY) != 0U) ? 1 : 0;
#else
    return 1;
#endif
}

/**
 * @brief Pede ao hardware para gerar clock e receber um byte.
 *
 * No TWIHS do SAM V71 a receção é automática após o START+MMR(MREAD).
 * O hardware gera clock e deposita bytes em RHR continuamente.
 */
void hal_i2c_request_byte(void)
{
#if USE_REAL_HW
    /* Receção é automática no TWIHS após START com MREAD=1 */
#else
    /* simulação: dado sempre disponível */
#endif
}

/**
 * @brief Verifica se há um byte recebido pronto para leitura.
 * @return 1 se RXRDY, 0 caso contrário.
 */
int hal_i2c_rx_ready(void)
{
#if USE_REAL_HW
    return ((TWI0_SR & TWI_SR_RXRDY) != 0U) ? 1 : 0;
#else
    return 1;
#endif
}

/**
 * @brief Lê um byte recebido do barramento.
 * @return Byte lido do RHR.
 */
uint8_t hal_i2c_read_byte(void)
{
#if USE_REAL_HW
    return (uint8_t)(TWI0_RHR & 0xFFU);
#else
    for (uint8_t i = 0U; i < NUM_DEVICES; i++)
    {
        if (devices[i].addr == active_addr)
        {
            if (read_index < devices[i].len)
                return devices[i].data[read_index++];
            return 0x00U;
        }
    }
    return 0xFFU;
#endif
}

/**
 * @brief Verifica se o slave respondeu com ACK.
 *
 * Lê o bit NACK do Status Register. Se NACK=1 o slave não respondeu.
 *
 * @return 1 se ACK (slave respondeu), 0 se NACK.
 */
int hal_i2c_get_ack(void)
{
#if USE_REAL_HW
    return ((TWI0_SR & TWI_SR_NACK) != 0U) ? 0 : 1;
#else
    for (uint8_t i = 0U; i < NUM_DEVICES; i++)
>>>>>>> origin/OBC_board
    {
        if (devices[i].addr == active_addr)
            return 1;
    }
    return 0;
<<<<<<< HEAD
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
=======
#endif
}

/**
 * @brief Envia ACK ao slave (continuação de leitura).
 *
 * No TWIHS o ACK é automático enquanto não se envia STOP.
 */
void hal_i2c_send_ack(void)
{
#if USE_REAL_HW
    /* ACK é automático no TWIHS durante receção contínua */
#else
    /* simulação: nada */
#endif
}

/**
 * @brief Envia NACK ao slave (último byte da leitura).
 *
 * No TWIHS deve-se enviar STOP antes de ler o último byte.
 * Isso gera automaticamente o NACK no bus.
 */
void hal_i2c_send_nack(void)
{
#if USE_REAL_HW
    /* STOP antes do último byte gera NACK implícito */
    TWI0_CR = TWI_CR_STOP;
#else
    /* simulação: nada */
#endif
}

void hal_i2c_restart_read(uint8_t addr)
{
#if USE_REAL_HW
    /* Limpa qualquer byte residual no RHR */
    (void)(TWI0_RHR);

    /* Reconfigura MMR para leitura e gera novo START */
    TWI0_MMR = ((uint32_t)addr << TWI_MMR_DADR_SHIFT) | TWI_MMR_MREAD;
    TWI0_CR = TWI_CR_START;
#else
    active_addr = addr;
    read_index = 0U;
    for (uint8_t i = 0U; i < NUM_DEVICES; i++)
    {
        if (devices[i].addr == addr)
        {
            hal_i2c_randomize(i);
            break;
        }
    }
#endif
}

void hal_i2c_bus_recovery(void)
{
#if USE_REAL_HW
    /* Desativa o hardware TWI temporariamente */
    TWI0_CR = TWI_CR_SWRST;

    /* Toma controlo dos pinos SCL e SDA como GPIOs manuais */
    PIOA_PER = PIO_SCL | PIO_SDA;
    
    /* Configura SCL como saída e SDA como entrada (para lermos o que o Slave está a fazer) */
    PIOA_OER = PIO_SCL;
    PIOA_ODR = PIO_SDA; // Input

    /* Um delay decente para gerar ~10kHz a 50kHz (Aumenta este valor se necessário) */
    #define SLOW_DELAY 10000 

    /* Passo 1: Enviar até 9 clocks para o Slave largar a linha */
    for (int i = 0; i < 9; i++)
    {
        /* SCL LOW */
        PIOA_CODR = PIO_SCL;
        for (volatile int d = 0; d < SLOW_DELAY; d++) {}
        
        /* SCL HIGH */
        PIOA_SODR = PIO_SCL;
        for (volatile int d = 0; d < SLOW_DELAY; d++) {}

        /* Verifica se o Slave já largou a linha (SDA = HIGH) */
        /* PIOA_PDSR é o registo que lê o estado atual do pino */
        if ((PIOA_PDSR & PIO_SDA) != 0) 
        {
            break; /* A linha SDA já está em 3.3V, podemos parar os clocks! */
        }
    }

    /* Passo 2: Gerar Condição de STOP manual */
    /* 1. SCL e SDA a LOW */
    PIOA_OER = PIO_SDA;       /* Passa SDA a saída */
    PIOA_CODR = PIO_SCL;
    PIOA_CODR = PIO_SDA;
    for (volatile int d = 0; d < SLOW_DELAY; d++) {}
    
    /* 2. SCL sobe para HIGH (mantendo SDA a LOW) */
    PIOA_SODR = PIO_SCL;
    for (volatile int d = 0; d < SLOW_DELAY; d++) {}
    
    /* 3. SDA sobe para HIGH ENQUANTO SCL está a HIGH (Isto é a definição de STOP!) */
    PIOA_SODR = PIO_SDA;
    for (volatile int d = 0; d < SLOW_DELAY; d++) {}

    /* Passo 3: Devolver os pinos ao Hardware TWIHS */
    PIOA_PDR = PIO_SDA | PIO_SCL;

    /* Reinicia o hardware TWIHS */
    TWI0_CR = TWI_CR_SWRST;
    TWI0_CR = TWI_CR_MSEN | TWI_CR_SVDIS;
    (void)TWI0_SR;
    TWI0_CWGR = TWI_CWGR_VALUE;
#endif
>>>>>>> origin/OBC_board
}