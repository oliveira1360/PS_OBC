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
 * Circular buffer de RX — preenchido pelo interrupt RXRDY.
 *
 * Usado para todas as leituras pequenas (comandos TTC, frame scanner).
 * O interrupt garante que nenhum byte é perdido mesmo que o main loop
 * esteja ocupado com printf/sensores.
 * ========================================================================== */
#if USE_REAL_HW

#define RX_CIRC_MASK  (RX_CIRC_BUF_SIZE - 1U)

static volatile uint8_t  rx_circ_buf[RX_CIRC_BUF_SIZE];
static volatile uint16_t rx_circ_head = 0U;
static volatile uint16_t rx_circ_tail = 0U;

void USART0_Handler(void)
{
    uint32_t csr = USART0_CSR;
    if (csr & US_CSR_RXRDY)
    {
        uint8_t byte = (uint8_t)(USART0_RHR & 0xFFU);
        uint16_t next = (rx_circ_head + 1U) & RX_CIRC_MASK;
        if (next != rx_circ_tail)
        {
            rx_circ_buf[rx_circ_head] = byte;
            rx_circ_head = next;
        }
    }
    if (csr & (US_CSR_OVRE | US_CSR_FRAME | US_CSR_PARE))
        USART0_CR = US_CR_RSTSTA;
}

/* ==========================================================================
 * DMA RX — XDMAC canal 0 para o payload OTA (132 bytes em modo bulk).
 *
 * Antes de arrancar, desabilita o interrupt RXRDY para evitar que os bytes
 * sejam consumidos simultaneamente pelo interrupt e pelo DMA.
 * XDMAC_Handler reabilita o RXRDY interrupt quando a transferência termina.
 * ========================================================================== */
static volatile uint8_t s_dma_rx_done = 0U;

void XDMAC_Handler(void)
{
    uint32_t gis = XDMAC_GIS;
    if (gis & (1U << DMA_CH_USART0_RX))
    {
        (void)XDMAC_CH_CIS(DMA_CH_USART0_RX);
        XDMAC_GD = (1U << DMA_CH_USART0_RX);
        /* Reabilita interrupt RXRDY — circular buffer volta a receber */
        USART0_IER = US_IER_RXRDY;
        s_dma_rx_done = 1U;
    }
}

/**
 * @brief Inicia transferência DMA bulk USART0 RX → buffer.
 *
 * Desabilita o interrupt RXRDY antes de arrancar para evitar conflito.
 * XDMAC_Handler reabilita-o quando os @p len bytes forem todos recebidos.
 *
 * @param buf  Buffer de destino em SRAM.
 * @param len  Número de bytes a receber (tipicamente OTA_FULL_PACKET-2 = 132).
 */
void hal_usart_dma_recv(uint8_t *buf, uint16_t len)
{
    /* Desabilita RXRDY enquanto DMA está activo */
    USART0_IDR = US_IER_RXRDY;

    XDMAC_GD = (1U << DMA_CH_USART0_RX);

    XDMAC_CH_CSA(DMA_CH_USART0_RX)  = USART0_BASE + US_RHR_OFFSET;
    XDMAC_CH_CDA(DMA_CH_USART0_RX)  = (uint32_t)buf;
    XDMAC_CH_CUBC(DMA_CH_USART0_RX) = (uint32_t)len;
    XDMAC_CH_CBC(DMA_CH_USART0_RX)  = 0U;
    XDMAC_CH_CC(DMA_CH_USART0_RX)   = XDMAC_CC_USART0_RX;

    XDMAC_CH_CIE(DMA_CH_USART0_RX)  = XDMAC_CI_BIS;
    XDMAC_GIE = (1U << DMA_CH_USART0_RX);
    XDMAC_GE  = (1U << DMA_CH_USART0_RX);
}

uint8_t hal_usart_dma_done(void)  { return s_dma_rx_done; }
void    hal_usart_dma_clear(void) { s_dma_rx_done = 0U;   }

#endif /* USE_REAL_HW */

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

    /* 9. Habilita interrupt RXRDY (circular buffer) + IRQ13 no NVIC */
    rx_circ_head = 0U;
    rx_circ_tail = 0U;
    USART0_IER  = US_IER_RXRDY;
    NVIC_ISER0  = (1UL << ID_USART0);

    /* 10. Habilita clock do XDMAC no PMC (ID=58 → bit 26 do PCER1)
     *     e IRQ58 no NVIC (ISER1 bit 26) — usado para bulk OTA (132 B) */
    PMC_PCER1  |= (1UL << (ID_XDMAC - 32U));
    NVIC_ISER1  = (1UL << (ID_XDMAC - 32U));

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
    return (rx_circ_head != rx_circ_tail) ? 1U : 0U;
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
    if (rx_circ_head == rx_circ_tail) return 0U;
    uint8_t byte = rx_circ_buf[rx_circ_tail];
    rx_circ_tail = (rx_circ_tail + 1U) & RX_CIRC_MASK;
    return byte;
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