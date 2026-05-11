/**
 * @file hal_usart.c
<<<<<<< HEAD
 * @brief Simulação da Hardware Abstraction Layer (HAL) para o periférico USART.
 *
 * Este módulo não interage com hardware real. Em vez disso, simula o 
 * comportamento de uma porta série bidirecional, gerando pacotes de dados 
 * fictícios (comandos da "Ground Station") para efeitos de teste do 
 * subsistema de Telemetria, Rastreio e Comando (TT&C).
=======
 * @brief Hardware Abstraction Layer para USART.
 *
 * Usa USE_REAL_HW em board.h:
 *   0 → dados simulados (Ground Station fictícia)
 *   1 → hardware real via USART0 do ATSAMV71Q21 (EXT1: PB00=RXD0, PB01=TXD0)
>>>>>>> origin/OBC_board
 */

#include "hal/hal_usart.h"
#include "config/board.h"
<<<<<<< HEAD
#include <stdlib.h>  /* para simulacao apenas !!! */
#include <time.h>    /* para simulacao apenas !!! */

/**
 * @brief Estrutura do buffer de receção simulado.
 *
 * Contém os dados fictícios recebidos via USART e o respetivo comprimento.
 */
typedef struct
{
    uint8_t data[TTC_BUF_LEN]; /**< Array de dados recebidos */
    uint8_t len;               /**< Comprimento total da mensagem */
} usart_sim_t;

/**
 * @brief Variável estática que armazena o estado do pacote simulado atual.
 * Inicializada com um pacote padrão que simula um comando `CMD_REQUEST_DATA` (0x20).
 */
static usart_sim_t sim = {
    {0x20, 0x01, 0x00, 0x00},  /* 0x20 = CMD_REQUEST_DATA */
    4U
};

/**
 * @brief Índice de leitura atual do buffer de receção simulado.
 */
static uint8_t rx_index  = 0;

/**
 * @brief Flag que indica se o USART está pronto para transmitir dados.
 */
static uint8_t tx_ready  = 1;

/**
 * @brief Flag que indica se existem dados disponíveis para leitura.
 */
static uint8_t rx_ready  = 0;

/**
 * @brief Controla se o RX auto-regenera dados ao esgotar o buffer.
 * 1 = comportamento normal (dados contínuos da Ground Station).
 * 0 = desativado — útil para testar timeout de receção.
 */
static uint8_t rx_auto_regen = 1;

/**
 * @brief Gera pacotes de comandos simulados da Ground Station (GS) com
 *        distribuição de probabilidade realista para uma missão CubeSat.
 *
 * Distribuição simulada (por 100 uplinks):
 *   85% CMD_REQUEST_DATA (0x20) — telemetria periódica normal
 *    8% CMD_ENTER_SAFE   (0x01) — operador entra em safe mode
 *    4% CMD_START_OTA    (0x10) — início de atualização OTA
 *    2% CMD_REMOTE_CTRL  (0x02) — controlo remoto manual
 *    1% CMD_END_OTA      (0x11) — fim/confirmação de OTA
 *
 * Formato do frame de 4 bytes por comando:
 *
 *  CMD_REQUEST_DATA : [0x20][doppler_x10 kHz][timestamp_hi][timestamp_lo]
 *    doppler_x10: efeito Doppler em décimas de kHz (5–35 → 0.5–3.5 kHz)
 *    timestamp  : contador de ciclos de telemetria (big-endian 16-bit)
 *
 *  CMD_ENTER_SAFE   : [0x01][0x00][0x00][0x00]
 *
 *  CMD_START_OTA    : [0x10][ver_major][ver_minor][0x00]
 *    ver_major/minor: versão do firmware proposto (ex: 1.3)
 *
 *  CMD_REMOTE_CTRL  : [0x02][0x00][0x00][0x00]
 *
 *  CMD_END_OTA      : [0x11][0x00][0x00][0x00]
 */
static void hal_usart_randomize(void)
{
    static uint16_t tlm_timestamp = 0U; /* contador de ciclos de telemetria */
=======

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

/* Macros de acesso — USART0 */
#define USART0_CR (*(volatile uint32_t *)(USART0_BASE + US_CR_OFFSET))
#define USART0_MR (*(volatile uint32_t *)(USART0_BASE + US_MR_OFFSET))
#define USART0_CSR (*(volatile uint32_t *)(USART0_BASE + US_CSR_OFFSET))
#define USART0_RHR (*(volatile uint32_t *)(USART0_BASE + US_RHR_OFFSET))
#define USART0_THR (*(volatile uint32_t *)(USART0_BASE + US_THR_OFFSET))
#define USART0_BRGR (*(volatile uint32_t *)(USART0_BASE + US_BRGR_OFFSET))

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
>>>>>>> origin/OBC_board
    int roll = rand() % 100;

    if (roll < 85)
    {
<<<<<<< HEAD
        /* ---- CMD_REQUEST_DATA (85%) ---- */
        tlm_timestamp++;
        sim.data[0] = 0x20U;                             /* CMD_REQUEST_DATA       */
        sim.data[1] = (uint8_t)(5U + rand() % 31U);     /* Doppler: 0.5–3.5 kHz  */
        sim.data[2] = (uint8_t)(tlm_timestamp >> 8U);   /* timestamp high byte    */
        sim.data[3] = (uint8_t)(tlm_timestamp & 0xFFU); /* timestamp low byte     */
    }
    else if (roll < 93)
    {
        /* ---- CMD_ENTER_SAFE (8%) ---- */
        sim.data[0] = 0x01U; /* CMD_ENTER_SAFE */
        sim.data[1] = 0x00U;
        sim.data[2] = 0x00U;
        sim.data[3] = 0x00U;
    }
    else if (roll < 97)
    {
        /* ---- CMD_START_OTA (4%) ---- */
        sim.data[0] = 0x10U;                         /* CMD_START_OTA     */
        sim.data[1] = 0x01U;                         /* ver_major = 1     */
        sim.data[2] = (uint8_t)(rand() % 10U);       /* ver_minor = 0–9   */
        sim.data[3] = 0x00U;
    }
    else if (roll < 99)
    {
        /* ---- CMD_REMOTE_CTRL (2%) ---- */
        sim.data[0] = 0x02U; /* CMD_REMOTE_CTRL */
        sim.data[1] = 0x00U;
        sim.data[2] = 0x00U;
        sim.data[3] = 0x00U;
    }
    else
    {
        /* ---- CMD_END_OTA (1%) ---- */
        sim.data[0] = 0x11U; /* CMD_END_OTA */
        sim.data[1] = 0x00U;
        sim.data[2] = 0x00U;
        sim.data[3] = 0x00U;
    }

    rx_index = 0U;
    rx_ready = 1U;
}

/**
 * @brief Inicializa o periférico USART (Simulação).
 *
 * Em hardware real, configuraria os pinos, baud rate e interrupções.
 * Na simulação, gera o primeiro pacote aleatório para testes.
 *
 * @return 1U indicando sucesso na inicialização.
 */
uint8_t hal_usart_init(void)
{
    hal_usart_randomize();
    return 1U;
}

/**
 * @brief Verifica se o hardware USART está pronto para transmitir.
 *
 * @return Sempre 1U na simulação (o envio é imediato e inesgotável).
 */
uint8_t hal_usart_is_tx_ready(void)
{
    return tx_ready;  /* simulacao: sempre pronto para enviar */
}

/**
 * @brief Simula a transmissão de um caractere pelo USART.
 *
 * Como é uma simulação de Ground Station, os bytes enviados pelo 
 * satélite são simplesmente descartados.
 *
 * @param data O byte a ser transmitido.
 */
void hal_usart_write_char(uint8_t data)
{
    (void)data;  /* simulacao: descarta o byte enviado */
    tx_ready = 1U;
}

/**
 * @brief Verifica se há dados recebidos no buffer USART.
 *
 * @return 1U se houverem dados não lidos, 0U caso contrário.
 */
uint8_t hal_usart_data_available(void)
{
    return rx_ready;
}

/**
 * @brief Lê o próximo caractere do buffer de receção simulado.
 *
 * Retorna o próximo byte da estrutura `sim`. Quando todos os bytes
 * do pacote tiverem sido lidos, baixa a flag `rx_ready`. Se
 * `rx_auto_regen` estiver ativo, gera automaticamente um novo pacote.
 *
 * @return O byte recebido, ou 0x00U se não houverem dados ou limite atingido.
 */
uint8_t hal_usart_read_char(void)
{
    uint8_t byte = 0x00U;

    if (rx_index < sim.len)
        byte = sim.data[rx_index++];

    if (rx_index >= sim.len)
    {
        rx_ready = 0U;
        if (rx_auto_regen)
            hal_usart_randomize(); /* gera novo pacote da GS automaticamente */
    }

    return byte;
}

/**
 * @brief Controla manualmente o estado de prontidão do transmissor.
 *        Útil para simular condições de timeout em testes.
 *
 * @param v 1U = TX pronto; 0U = TX ocupado/bloqueado.
 */
void hal_usart_set_tx_ready(uint8_t v)
{
    tx_ready = v;
}

/**
 * @brief Ativa ou desativa a regeneração automática de pacotes RX.
 *        Quando desativada, o buffer esgota-se e `rx_ready` fica a 0,
 *        simulando ausência de dados (útil para testar timeouts de receção).
 *
 * @param v 1U = auto-regen ativo (padrão); 0U = desativado.
 */
void hal_usart_set_rx_auto_regen(uint8_t v)
{
    rx_auto_regen = v;
}

/**
 * @brief Prepara o USART para uma nova receção.
 *
 * Na simulação, aciona a geração de um novo pacote de comandos 
 * aleatórios provenientes da "Ground Station". Em hardware real, 
 * serviria para limpar FIFOs ou rearmar interrupções de receção.
 */
void hal_usart_prepare_rx(void)
{
    hal_usart_randomize();
=======
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
>>>>>>> origin/OBC_board
}