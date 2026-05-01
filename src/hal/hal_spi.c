/**
 * @file hal_spi.c
 * @brief Simulação da Hardware Abstraction Layer (HAL) para o barramento SPI.
 *
 * Este módulo não interage com o hardware físico SPI. Serve apenas para simular
 * o comportamento do barramento para efeitos de teste (gerando dados aleatórios
 * na receção). O código para o hardware real encontra-se comentado dentro
 * das funções como referência.
 */

#include "hal/hal_spi.h"
#include "config/board.h"
#include <stdlib.h>  /* simulacao apenas !!! */

/**
 * @brief Flag que indica se o barramento SPI está pronto para transmitir (Tx).
 */
static uint8_t tx_ready = 1U;

/**
 * @brief Flag que indica se há dados prontos a ser lidos na receção (Rx).
 */
static uint8_t rx_ready = 0U;

/**
 * @brief Buffer interno para guardar o byte simulado recebido via SPI.
 */
static uint8_t rx_data  = 0x00U;

/* -------------------------------------------------------------------------
 * Simulação do frame SPI do Propulsor (cold-gas thruster, CubeSat 3U)
 *
 * Frame de resposta (9 bytes, veja propulsor.c para documentação completa):
 *   [0]   STATUS        : PROP_STATUS_IDLE (0x00)
 *   [1-2] PRESSURE H/L  : pressão câmara /100 = bar  →  1.50–2.50 bar  (tanque frio)
 *   [3-4] TEMP H/L      : temperatura    /10  = °C   →  22–28°C        (ambiente)
 *   [5-6] THRUST H/L    : impulso        /10  = N    →  0.0 N          (válvula fechada)
 *   [7]   VALVE_STATE   : 0 (fechada)
 *   [8]   CHECKSUM      : XOR de bytes 0..7
 *
 * A função prop_generate_frame() é chamada em hal_spi_cs_low() para gerar
 * um frame fresco a cada transação SPI. O checksum é sempre calculado
 * corretamente para que propulsor_parse() aceite o frame.
 * ------------------------------------------------------------------------- */

/** @brief Frame de telemetria do propulsor pré-gerado para a transação atual. */
static uint8_t prop_frame[9];

/** @brief Índice do próximo byte a devolver em hal_spi_read_byte(). */
static uint8_t prop_byte_idx = 0U;

/**
 * @brief Gera um frame de telemetria realista para o propulsor simulado.
 *
 * Simula o estado típico de um cold-gas thruster em standby:
 *   - Status IDLE, válvula fechada, sem impulso
 *   - Pressão residual do tanque (1.50–2.50 bar)
 *   - Temperatura de câmara ambiente (22–28°C)
 *
 * O checksum XOR é calculado automaticamente para garantir que
 * propulsor_parse() aceite o frame sem erro.
 */
static void prop_generate_frame(void)
{
    uint8_t *f = prop_frame;

    /* [0] STATUS: IDLE (0x00 = PROP_STATUS_IDLE) */
    f[0] = 0x00U;

    /* [1-2] Pressão câmara: 1.50–2.50 bar → raw 150–250 */
    uint16_t press_raw = 150U + (uint16_t)(rand() % 101U);
    f[1] = (uint8_t)(press_raw >> 8U);
    f[2] = (uint8_t)(press_raw & 0xFFU);

    /* [3-4] Temperatura: 22–28°C → raw 220–280 (/10) */
    uint16_t temp_raw = 220U + (uint16_t)(rand() % 61U);
    f[3] = (uint8_t)(temp_raw >> 8U);
    f[4] = (uint8_t)(temp_raw & 0xFFU);

    /* [5-6] Impulso: 0.0 N (válvula fechada, standby) */
    f[5] = 0x00U;
    f[6] = 0x00U;

    /* [7] Válvula: fechada */
    f[7] = 0x00U;

    /* [8] Checksum XOR de bytes 0..7 */
    uint8_t chk = 0U;
    for (uint8_t j = 0U; j < 8U; j++)
        chk ^= f[j];
    f[8] = chk;

    prop_byte_idx = 0U;
}

/**
 * @brief Inicializa o periférico SPI (Simulação).
 *
 * Na versão simulada, esta função apenas retorna sucesso. Na versão real,
 * configuraria os registos do microcontrolador (Power Management, Baud Rate, etc.).
 *
 * @return 1U indicando sucesso na inicialização.
 */
uint8_t hal_spi_init(void)
{
    /* hardware real:
       PMC_PCER0 |= (1U << ID_SPI0);
       SPI0_CR   = SPI_CR_SWRST;
       SPI0_MR   = SPI_MR_MSTR | SPI_MR_MODFDIS;
       SPI0_CSR0 = (SPI_BAUD_DIV8 << 8U);
       SPI0_CR   = SPI_CR_SPIEN; */
    return 1U;
}

/**
 * @brief Verifica se o SPI está pronto para transmitir um novo byte.
 *
 * @return 1U se estiver pronto para transmitir, 0U caso contrário.
 */
uint8_t hal_spi_tx_ready(void) { return tx_ready; }

/**
 * @brief Verifica se o SPI tem um byte recebido pronto a ser lido.
 *
 * @return 1U se houver dados disponíveis, 0U caso contrário.
 */
uint8_t hal_spi_rx_ready(void) { return rx_ready; }

/**
 * @brief Coloca o pino de Chip Select (CS) em nível lógico baixo (Ativo).
 *
 * Na simulação, aproveita o CS_LOW para gerar um frame de telemetria fresco
 * para o propulsor, de modo a que os bytes devolvidos em hal_spi_read_byte()
 * formem sempre um frame válido com checksum correto.
 *
 * @param cs_pin O pino/identificador do Chip Select a ser ativado.
 */
void hal_spi_cs_low(uint8_t cs_pin)
{
    (void)cs_pin;
    /* hardware: PIO_CODR = (1U << cs_pin) */
    prop_generate_frame(); /* simulação: prepara frame do propulsor */
}

/**
 * @brief Coloca o pino de Chip Select (CS) em nível lógico alto (Inativo).
 *
 * Simula a desativação de um dispositivo escravo no barramento SPI.
 *
 * @param cs_pin O pino/identificador do Chip Select a ser desativado.
 */
void hal_spi_cs_high(uint8_t cs_pin)
{
    (void)cs_pin;
    /* hardware: PIO_SODR = (1U << cs_pin) */
}

/**
 * @brief Simula o envio de um byte pelo barramento SPI (Full-Duplex).
 *
 * Como o SPI é full-duplex, enviar um byte significa também receber um byte.
 * Na simulação, o byte devolvido corresponde ao byte seguinte do frame de
 * telemetria do propulsor gerado em hal_spi_cs_low(). Isto garante que o
 * frame completo tem um checksum XOR válido, passando a verificação em
 * propulsor_parse().
 *
 * @param data O byte de dados a ser transmitido (ignorado na simulação).
 */
void hal_spi_send_byte(uint8_t data)
{
    (void)data;
    /* hardware: SPI0_TDR = data */
    tx_ready = 1U;

    /* simulação: devolve o próximo byte do frame do propulsor */
    if (prop_byte_idx < sizeof(prop_frame))
        rx_data = prop_frame[prop_byte_idx++];
    else
        rx_data = 0x00U; /* guard: não deve acontecer se PROPULSOR_BUF_LEN == 9 */

    rx_ready = 1U;
}

/**
 * @brief Lê o último byte recebido pelo barramento SPI.
 *
 * Limpa a flag de receção (`rx_ready`) e retorna o valor gerado internamente
 * pela função de envio.
 *
 * @return O byte lido do registo de dados (simulado).
 */
uint8_t hal_spi_read_byte(void)
{
    rx_ready = 0U;
    return rx_data;
    /* hardware: return (uint8_t)(SPI0_RDR & 0xFFU) */
}

/**
 * @brief Prepara o periférico SPI para uma transferência.
 *
 * Esta função atua como um "stub" (função vazia) na simulação, podendo ser
 * útil em hardware real para esvaziar FIFOs ou limpar flags de erro antes 
 * de iniciar uma nova comunicação.
 */
void hal_spi_prepare_transfer(void) { }