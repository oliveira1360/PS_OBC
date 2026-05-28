/**
 * @file hal_usart.c
 * @brief Hardware Abstraction Layer para USART.
 *
 * Usa USE_REAL_HW em board.h:
 *   0 → dados simulados (Ground Station fictícia)
 *   1 → hardware real via USART0 do ATSAMV71Q21 (EXT1: PB00=RXD0, PB01=TXD0)
 */

#include "hal/hal_usart.h"
#include "config/board.h"

#if !USE_REAL_HW
#include <stdlib.h>
#include <time.h>
#endif

#if USE_REAL_HW

/* PMC */
#define PMC_BASE 0x400E0600UL
#define PMC_PCER0 (*(volatile uint32_t *)(PMC_BASE + 0x10U))
#define ID_PIOB 11U

/* PIOB */
#define PIOB_BASE 0x400E1000UL
#define PIOB_PDR (*(volatile uint32_t *)(PIOB_BASE + 0x04U))
#define PIOB_ABCDSR0 (*(volatile uint32_t *)(PIOB_BASE + 0x70U))
#define PIOB_ABCDSR1 (*(volatile uint32_t *)(PIOB_BASE + 0x74U))

/* Pin masks — PB00=RXD0, PB01=TXD0 */
#define PIO_RXD0 (1UL << 0) /* PB00 */
#define PIO_TXD0 (1UL << 1) /* PB01 */

/* USART0 Mode Register bits */
#define US_MR_USART_MODE_NORMAL (0x0UL << 0)
#define US_MR_USCLKS_MCK (0x0UL << 4)
#define US_MR_CHRL_8_BIT (0x3UL << 6)
#define US_MR_PAR_NO (0x4UL << 9)
#define US_MR_NBSTOP_1_BIT (0x0UL << 12)
#define US_MR_CHMODE_NORMAL (0x0UL << 14)

/* Baud rate: MCK / (16 × BRGR) = baudrate → BRGR = MCK / (16 × baudrate) */
#define MCK_HZ 12000000UL
#define USART_BRGR_VALUE (MCK_HZ / (16UL * USART_BAUDRATE))


#endif /* USE_REAL_HW */

/* ==========================================================================
 * SECÇÃO SIMULAÇÃO (só compilada quando USE_REAL_HW == 0)
 * ========================================================================== */
#if !USE_REAL_HW

#define SIM_BUF_LEN TTC_BUF_LEN

typedef struct
{
    uint8_t data[SIM_BUF_LEN];
    uint8_t len;
} usart_sim_t;

static usart_sim_t sim = {
    {0x20, 0x01, 0x00, 0x00},
    4U};

static uint8_t rx_index = 0;
static uint8_t tx_ready_flag = 1;
static uint8_t rx_ready_flag = 0;
static uint8_t rx_auto_regen = 1;

static void hal_usart_randomize(void)
{
    static uint16_t tlm_timestamp = 0U;
    int roll = rand() % 100;

    if (roll < 85)
    {
        tlm_timestamp++;
        sim.data[0] = 0x20U;
        sim.data[1] = (uint8_t)(5U + rand() % 31U);
        sim.data[2] = (uint8_t)(tlm_timestamp >> 8U);
        sim.data[3] = (uint8_t)(tlm_timestamp & 0xFFU);
    }
    else if (roll < 93)
    {
        sim.data[0] = 0x01U;
        sim.data[1] = 0;
        sim.data[2] = 0;
        sim.data[3] = 0;
    }
    else if (roll < 97)
    {
        sim.data[0] = 0x10U;
        sim.data[1] = 0x01U;
        sim.data[2] = (uint8_t)(rand() % 10U);
        sim.data[3] = 0;
    }
    else if (roll < 99)
    {
        sim.data[0] = 0x02U;
        sim.data[1] = 0;
        sim.data[2] = 0;
        sim.data[3] = 0;
    }
    else
    {
        sim.data[0] = 0x11U;
        sim.data[1] = 0;
        sim.data[2] = 0;
        sim.data[3] = 0;
    }

    rx_index = 0U;
    rx_ready_flag = 1U;
}

#endif /* !USE_REAL_HW */

/* ==========================================================================
 * IMPLEMENTAÇÕES DAS FUNÇÕES HAL
 * ========================================================================== */

uint8_t hal_usart_init(void)
{
#if USE_REAL_HW
    /* 1. Ativa clock do USART0 no PMC (peripheral ID 13) */
    PMC_PCER0 = (1UL << ID_USART0) | (1UL << ID_PIOB);

    /* 2. Configura PB00(RXD0) e PB01(TXD0) como Peripheral C
     *    SAMV71 Peripheral C: ABCDSR0 (0x70) = 0, ABCDSR1 (0x74) = 1 */
    PIOB_PDR = PIO_RXD0 | PIO_TXD0; // Desativa modo GPIO, entrega ao Periférico

    // Limpa os bits no ABCDSR0 (força a 0)
    PIOB_ABCDSR0 &= ~(PIO_RXD0 | PIO_TXD0);

    // Seta os bits no ABCDSR1 (força a 1)
    PIOB_ABCDSR1 |= (PIO_RXD0 | PIO_TXD0);

    /* 3. Reset e desativa TX/RX */
    USART0_CR = US_CR_RSTRX | US_CR_RSTTX | US_CR_RXDIS | US_CR_TXDIS;

    /* 4. Configura modo: normal, MCK, 8 bits, sem paridade, 1 stop bit */
    USART0_MR = US_MR_USART_MODE_NORMAL | US_MR_USCLKS_MCK | US_MR_CHRL_8_BIT | US_MR_PAR_NO | US_MR_NBSTOP_1_BIT | US_MR_CHMODE_NORMAL;

    /* 5. Baud rate */
    USART0_BRGR = USART_BRGR_VALUE;

    /* 6. Ativa TX e RX */
    USART0_CR = US_CR_RXEN | US_CR_TXEN;

    /* 7. Limpa flags e erros */
    USART0_CR = US_CR_RSTSTA;

    /* 8. Flush — drena todos os bytes residuais do RHR */
    while (USART0_CSR & US_CSR_RXRDY)
    {
        (void)USART0_RHR;
    }

    return 1U;

#else
    srand((unsigned int)time(NULL));
    hal_usart_randomize();
    return 1U;
#endif
}

uint8_t hal_rx_data_availible(void)
{
#if USE_REAL_HW
    uint32_t csr = USART0_CSR;

    /* Limpa erros de overrun/framing se existirem */
    if (csr & (US_CSR_OVRE | US_CSR_FRAME | US_CSR_PARE))
    {
        USART0_CR = US_CR_RSTSTA; /* Reset status bits */
        (void)USART0_RHR;         /* FLUSH the garbage byte! */
    }

    return ((csr & US_CSR_RXRDY) != 0U) ? 1U : 0U;
#else
    return rx_ready_flag;
#endif
}

uint8_t hal_usart_tx_ready(void)
{
#if USE_REAL_HW
    return ((USART0_CSR & US_CSR_TXRDY) != 0U) ? 1U : 0U;
#else
    return tx_ready_flag;
#endif
}

void hal_usart_write_byte(uint8_t byte)
{
#if USE_REAL_HW
    USART0_THR = (uint32_t)byte;
#else
    tx_ready_flag = 1U;
    (void)byte;
#endif
}

uint8_t hal_usart_read_byte(void)
{
#if USE_REAL_HW
    return (uint8_t)(USART0_RHR & 0xFFU);
#else
    uint8_t byte = sim.data[rx_index];
    rx_index++;

    if (rx_index >= sim.len)
    {
        rx_ready_flag = 0U;
        if (rx_auto_regen)
            hal_usart_randomize();
    }

    return byte;
#endif
}