#include <stdint.h>
#include <stdio.h>
#include "app/init.h"
#include "config/board.h"
#include "app/sensors.h"
#include "app/mission.h"
#include "hal/hal_gpio.h"
#include "hal/hal_i2c.h"
#include "hal/hal_peripherals_init.h"
#include "hal/hal_qspi.h"
#include "hal/hal_spi.h"
#include "hal/hal_usart.h"
#include "hal/hal_system.h"
#include "hal/hal_systick.h"
#include "peripherals/ttc.h"
#include "testsForBoard/qspi_test.h"
#include "testsForBoard/wcet_test.h"
#include "testsForBoard/seu_injection_test.h"

static uint8_t self_test(void);
void recovery();
init_status_t init_status = {0};

static int init_all(void)
{
    hal_systick_init();

    if (!init_status.gpio)
        init_status.gpio = hal_gpio_init();

    if (!init_status.i2c)
        init_status.i2c = hal_i2c_init();

    if (!init_status.spi)
        init_status.spi = hal_spi_init();

    if (!init_status.qspi)
        init_status.qspi = hal_qspi_init();

    if (!init_status.usart)
        init_status.usart = hal_usart_init();

    if (!init_status.gnss)
        init_status.gnss = hal_gnss_init();

    if (!init_status.imu)
        init_status.imu = hal_imu_init();

    if (!init_status.ttc)
        init_status.ttc = hal_ttc_init();

    if (!init_status.pressure)
        init_status.pressure = hal_pressure_init();

    if (!init_status.temperature)
        init_status.temperature = hal_temperature_init();

    if (!init_status.ext_memory)
        init_status.ext_memory = hal_ext_memory_init();

    return init_status.gpio && init_status.i2c && init_status.spi &&
           init_status.qspi && init_status.usart && init_status.gnss &&
           init_status.imu && init_status.ttc && init_status.pressure &&
           init_status.temperature && init_status.ext_memory;
}

void recovery()
{
    hal_i2c_bus_recovery();
}

int system_init(void)
{
    uint8_t attempts = 0;

    while (!init_all())
    {
        recovery();
        attempts++;
        if (attempts >= MAX_INIT_RETRIES)
            return 0;
    }

    while (!self_test())
    {
        attempts++;
        if (attempts >= MAX_INIT_RETRIES)
            return 0;
    }

    return 1;
}

static uint8_t self_test(void)
{
    return hal_self_test_gpio() &&
           hal_self_test_i2c() &&
           hal_self_test_spi() &&
           hal_self_test_qspi() &&
           hal_self_test_usart() &&
           hal_self_test_memory();
}

/* ==========================================================================
 * TMR das variaveis de controlo / transicao
 *
 * Mesmo principio do seu_data (votacao 2-de-3 byte a byte, com 2 bancos
 * shadow em RAM separada), mas com o commit a fechar cada iteracao do
 * super-loop de topo — assim capta as transicoes de estado (state,
 * s_sys_state) e as flags de missao (COMM_WINDOW_OPEN, OTA_REQUESTED, ...),
 * que sao escritas ao longo da iteracao.
 *
 * Mantido dentro de init.c (ja compilado) para nao exigir adicionar um novo
 * ficheiro ao projeto MPLAB X.
 * ========================================================================== */

#define CTRL_TMR_MAX_REGIONS 12U
#define CTRL_TMR_BANK_BYTES  64U

static volatile uint8_t s_ctrl_bank_b[CTRL_TMR_BANK_BYTES];
static volatile uint8_t s_ctrl_bank_c[CTRL_TMR_BANK_BYTES];

typedef struct
{
    volatile uint8_t *live; /* copia A (a propria variavel)        */
    uint16_t          size; /* bytes                               */
    uint16_t          off;  /* offset da regiao dentro dos bancos  */
    const char       *name; /* para logging                        */
} ctrl_region_t;

static ctrl_region_t s_ctrl_regions[CTRL_TMR_MAX_REGIONS];
static uint8_t  s_ctrl_n    = 0U;
static uint16_t s_ctrl_used = 0U;

/* Regista uma variavel de controlo sob TMR (copia inicial para B e C). */
static void ctrl_protect(volatile void *live, uint16_t size, const char *name)
{
    ctrl_region_t *r;
    uint16_t i;

    if (s_ctrl_n >= CTRL_TMR_MAX_REGIONS) { return; }
    if ((uint16_t)(s_ctrl_used + size) > CTRL_TMR_BANK_BYTES) { return; }

    r = &s_ctrl_regions[s_ctrl_n];
    r->live = (volatile uint8_t *)live;
    r->size = size;
    r->off  = s_ctrl_used;
    r->name = name;

    for (i = 0U; i < size; i++)
    {
        s_ctrl_bank_b[r->off + i] = r->live[i];
        s_ctrl_bank_c[r->off + i] = r->live[i];
    }
    s_ctrl_used = (uint16_t)(s_ctrl_used + size);
    s_ctrl_n++;
}

/* Votacao 2-de-3 byte a byte, com reparacao (identica a seu_data_scrub). */
static void ctrl_scrub(void)
{
    uint8_t  n;
    uint16_t i;

    for (n = 0U; n < s_ctrl_n; n++)
    {
        const ctrl_region_t *r = &s_ctrl_regions[n];
        for (i = 0U; i < r->size; i++)
        {
            const uint8_t a = r->live[i];
            const uint8_t b = s_ctrl_bank_b[r->off + i];
            const uint8_t c = s_ctrl_bank_c[r->off + i];

            if (a == b && b == c) { continue; }      /* tudo coerente */

            if (a == b)      { s_ctrl_bank_c[r->off + i] = a; }  /* C corrompida */
            else if (a == c) { s_ctrl_bank_b[r->off + i] = a; }  /* B corrompida */
            else if (b == c) { r->live[i] = b; }                 /* live corrompida */
            else             { r->live[i] = b;                   /* falha tripla:  */
                               s_ctrl_bank_c[r->off + i] = b;    /* adota ult. commit */
                               printf("[CTRL-TMR] falha tripla em '%s'[%u]\r\n",
                                      r->name, (unsigned)i); }
        }
    }
}

/* Consolida as escritas legitimas desta iteracao nas 3 copias. */
static void ctrl_commit(void)
{
    uint8_t  n;
    uint16_t i;

    for (n = 0U; n < s_ctrl_n; n++)
    {
        const ctrl_region_t *r = &s_ctrl_regions[n];
        for (i = 0U; i < r->size; i++)
        {
            const uint8_t a = r->live[i];
            s_ctrl_bank_b[r->off + i] = a;
            s_ctrl_bank_c[r->off + i] = a;
        }
    }
}

/* ==========================================================================
 * FSM de topo — ciclo de vida do sistema
 *
 *   Boot ----boot_done----> Stabilize ----stabilize_timeout----> Mission
 *    |  \___boot_fail && reset_count>MAX___> Recovery ___________/
 *    |                                          ^
 *   Mission ----ground_cmd_reset----> Boot      |  (init falhado em Stabilize)
 *
 * FAST_BOOT (board.h) a 1 coloca a espera de estabilizacao a 0 ms, pelo que o
 * estado STABILIZE atravessa de imediato para MISSION — reproduz o arranque
 * antigo (rapido), ideal para a apresentacao. A 0 usa STABILIZE_TIMEOUT_MS.
 * ========================================================================== */

typedef enum
{
    SYS_BOOT = 0,
    SYS_STABILIZE,
    SYS_RECOVERY,
    SYS_MISSION
} sys_state_t;

static sys_state_t s_sys_state = SYS_BOOT;
static uint32_t    s_reset_count = 0U;          /* reset_count do diagrama          */
static uint32_t    s_stabilize_deadline = 0U;   /* instante-limite da estabilizacao */
/* /set stabilize_timeout : 0 se FAST_BOOT, senao STABILIZE_TIMEOUT_MS */
static uint32_t    s_stabilize_timeout_ms = (FAST_BOOT ? 0UL : STABILIZE_TIMEOUT_MS);

volatile uint8_t GROUND_CMD_RESET = 0U;         /* posto a 1 por comando de solo    */

void lifecycle_request_reset(void)
{
    GROUND_CMD_RESET = 1U;
}

/* Verifica se todos os subsistemas estao inicializados (sem re-inicializar). */
static int init_ok(void)
{
    return init_status.gpio && init_status.i2c && init_status.spi &&
           init_status.qspi && init_status.usart && init_status.gnss &&
           init_status.imu && init_status.ttc && init_status.pressure &&
           init_status.temperature && init_status.ext_memory;
}

/* Corre os testes de placa uma unica vez (apenas no 1o boot). */
static void run_board_tests_once(void)
{
#if RUN_BOARD_TESTS
    static uint8_t done = 0U;
    if (done) { return; }
    done = 1U;
    test_qspi_rw();
    test_wcet();
    test_seu_injection();
#endif
}

/* ---- Estado BOOT: on entry / Init ---- */
static void sys_do_boot(void)
{
    printf("[SYS] BOOT - Init\n");

    if (init_all() && self_test())
    {
        s_reset_count = 0U;
        run_board_tests_once();
        s_stabilize_deadline = hal_systick_get_ms() + s_stabilize_timeout_ms;
        printf("[SYS] boot_done -> STABILIZE (timeout=%lu ms)\n",
               (unsigned long)s_stabilize_timeout_ms);
        s_sys_state = SYS_STABILIZE;
        return;
    }

    /* boot_fail */
    recovery();          /* bus recovery I2C */
    s_reset_count++;
    printf("[SYS] boot_fail (reset_count=%lu)\n", (unsigned long)s_reset_count);

    if (s_reset_count > MAX_INIT_RETRIES)
    {
        /* Garante o minimo para falar com o solo: init uart / init TT&C */
        init_status.usart = hal_usart_init();
        init_status.ttc   = hal_ttc_init();
        ttc_read_async();
        printf("[SYS] reset_count>MAX -> RECOVERY\n");
        s_sys_state = SYS_RECOVERY;
    }
    /* senao, permanece em BOOT e retenta no proximo ciclo */
}

/* ---- Estado STABILIZE: /get stabilize_timeout ---- */
static void sys_do_stabilize(void)
{
    sensors_tick();   /* mantem sensores/TMR/ExtMem vivos durante a estabilizacao */

    if (!init_ok())
    {
        /* init_done && !stabilize_timeout -> Recovery (guarda defensiva) */
        printf("[SYS] init perdido em STABILIZE -> RECOVERY\n");
        s_sys_state = SYS_RECOVERY;
        return;
    }

    if ((int32_t)(hal_systick_get_ms() - s_stabilize_deadline) >= 0)
    {
        printf("[SYS] stabilize_timeout -> MISSION\n");
        ttc_read_async();     /* arma o RX do radio ao entrar em missao */
        s_sys_state = SYS_MISSION;
    }
}

/* ---- Estado RECOVERY: ping ground / read cmd / Init / get stabilize_timeout ---- */
static void sys_do_recovery(void)
{
    static uint32_t s_last_ping = 0U;
    static uint8_t  s_reinited  = 0U;
    uint32_t now = hal_systick_get_ms();

    sensors_tick();

    /* Ping a estacao terrestre a ~1 Hz (heartbeat/telemetria).
     * O RX de comandos ja esta armado (ttc_read_async), o handler TTC trata
     * os comandos recebidos ("Read cmd ground station"). */
    if ((now - s_last_ping) >= 1000U)
    {
        ttc_send_telemetry();
        s_last_ping = now;
    }

    if (!s_reinited)
    {
        /* Tenta reinicializar tudo. */
        if (init_all() && self_test())
        {
            s_reinited = 1U;
            s_stabilize_deadline = now + s_stabilize_timeout_ms;
            printf("[SYS] RECOVERY init_done - a estabilizar\n");
        }
        return;
    }

    /* Reinicializado — espera a estabilizacao e entra em missao. */
    if ((int32_t)(now - s_stabilize_deadline) >= 0)
    {
        s_reinited = 0U;               /* repoe para uma futura recovery */
        printf("[SYS] RECOVERY -> MISSION\n");
        ttc_read_async();
        s_sys_state = SYS_MISSION;
    }
}

/* ---- Estado MISSION: Execute mission modes ---- */
static void sys_do_mission(void)
{
    mission_lifecycle();

    if (GROUND_CMD_RESET)
    {
        GROUND_CMD_RESET = 0U;
        s_reset_count = 0U;
        printf("[SYS] ground_cmd_reset -> BOOT\n");
        s_sys_state = SYS_BOOT;
    }
}

/* ---- Super-loop da FSM de topo (nunca retorna) ---- */
void system_lifecycle(void)
{

    ctrl_protect(&s_sys_state,           sizeof s_sys_state,           "sys_state");
    ctrl_protect(&s_reset_count,         sizeof s_reset_count,         "reset_count");
    ctrl_protect((volatile void *)&GROUND_CMD_RESET,
                                         sizeof GROUND_CMD_RESET,      "gnd_reset");
    ctrl_protect(&state,                 sizeof state,                 "mission_state");
    ctrl_protect(&COMM_WINDOW_OPEN,      sizeof COMM_WINDOW_OPEN,      "comm_window");
    ctrl_protect(&OTA_REQUESTED,         sizeof OTA_REQUESTED,         "ota_req");
    ctrl_protect(&MISSION_TIMEOUT,       sizeof MISSION_TIMEOUT,       "mission_to");
    ctrl_protect(&TIME_TO_UPDATE_VALUES, sizeof TIME_TO_UPDATE_VALUES, "upd_interval");

    for (;;)
    {
        ctrl_scrub();    

        switch (s_sys_state)
        {
        case SYS_BOOT:      sys_do_boot();      break;
        case SYS_STABILIZE: sys_do_stabilize(); break;
        case SYS_RECOVERY:  sys_do_recovery();  break;
        case SYS_MISSION:   sys_do_mission();   break;
        default:            s_sys_state = SYS_BOOT; break;
        }

        ctrl_commit();   
    }
}