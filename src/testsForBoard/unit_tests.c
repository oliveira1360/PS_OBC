/**
 * @file unit_tests.c
 * @brief Testes unitários HIL (Hardware-In-the-Loop) para o OBC CubeSat.
 *
 * Cada teste:
 *   1. Configura o estado (ARRANGE)
 *   2. Executa a função alvo (ACT)
 *   3. Verifica o resultado (ASSERT)
 *   4. Repõe o estado original (RESTORE)
 *   5. Imprime PASS / FAIL
 *
 * Não interrompe o super-loop — corre uma única vez no arranque,
 * antes de ttc_read_async() e do ciclo principal.
 */

#include "testsForBoard/unit_tests.h"
#include "hal/hal_systick.h"
#include "app/mission.h"
#include "app/modes.h"
#include "app/sensors.h"
#include "peripherals/eps.h"
#include "peripherals/temperature.h"
#include "peripherals/pressure.h"
#include "peripherals/ttc.h"
#include "peripherals/ext_memory.h"
#include "config/board.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* =========================================================================
 * Infraestrutura de reporte
 * ========================================================================= */

static uint8_t s_passed = 0U;
static uint8_t s_failed = 0U;

#define TEST_PASS(name)                                          \
    do {                                                         \
        printf("  [PASS] %-45s\r\n", (name));                  \
        s_passed++;                                              \
    } while (0)

#define TEST_FAIL(name, reason)                                  \
    do {                                                         \
        printf("  [FAIL] %-45s  (%s)\r\n", (name), (reason));  \
        s_failed++;                                              \
    } while (0)

/* Busy-wait simples para dar tempo ao hardware (usa SysTick) */
static void delay_ms(uint32_t ms)
{
    uint32_t start = hal_systick_get_ms();
    while ((hal_systick_get_ms() - start) < ms)
        ;
}

/* =========================================================================
 * GRUPO 1 — SysTick
 * ========================================================================= */

/**
 * T01: SysTick avança
 * Verifica que o contador de milissegundos não está parado.
 */
static void t01_systick_advances(void)
{
    uint32_t t0 = hal_systick_get_ms();
    delay_ms(10U);
    uint32_t t1 = hal_systick_get_ms();

    if (t1 > t0)
        TEST_PASS("T01 SysTick avanca");
    else
        TEST_FAIL("T01 SysTick avanca", "contador nao avancou em 10 ms");
}

/**
 * T02: SysTick mede ~100 ms com tolerância de 5 ms
 */
static void t02_systick_accuracy(void)
{
    uint32_t t0 = hal_systick_get_ms();
    delay_ms(100U);
    uint32_t elapsed = hal_systick_get_ms() - t0;

    /* Aceita [95..105] ms */
    if (elapsed >= 95U && elapsed <= 105U)
        TEST_PASS("T02 SysTick precisao 100 ms");
    else
    {
        char reason[40];
        /* Usa snprintf para mostrar o valor medido */
        (void)snprintf(reason, sizeof(reason), "medido=%lu ms", (unsigned long)elapsed);
        TEST_FAIL("T02 SysTick precisao 100 ms", reason);
    }
}

/* =========================================================================
 * GRUPO 2 — Sensores (range checks com hardware real)
 * ========================================================================= */

/**
 * T03: EPS — tensão num range plausível [0 V .. 10 V]
 * Executa alguns ticks para deixar o driver I2C completar.
 */
static void t03_eps_voltage_range(void)
{
    eps_read_async();
    /* Dá até 200 ticks ao driver para completar */
    for (uint32_t i = 0U; i < 200U; i++)
        eps_tick();

    if (eps.voltage >= 0.0f && eps.voltage <= 10.0f)
        TEST_PASS("T03 EPS tensao em range [0..10 V]");
    else
        TEST_FAIL("T03 EPS tensao em range [0..10 V]", "valor fora do range");
}

/**
 * T04: EPS — corrente num range plausível [0 A .. 5 A]
 */
static void t04_eps_current_range(void)
{
    /* Reutiliza a leitura já feita em T03 */
    if (eps.current >= 0.0f && eps.current <= 5.0f)
        TEST_PASS("T04 EPS corrente em range [0..5 A]");
    else
        TEST_FAIL("T04 EPS corrente em range [0..5 A]", "valor fora do range");
}

/**
 * T05: Temperatura num range plausível [-50 °C .. 100 °C]
 */
static void t05_temperature_range(void)
{
    temperature_read_async();
    for (uint32_t i = 0U; i < 200U; i++)
        temperature_tick();

    if (temperature.temperature >= -50.0f && temperature.temperature <= 100.0f)
        TEST_PASS("T05 Temperatura em range [-50..100 graus]");
    else
        TEST_FAIL("T05 Temperatura em range [-50..100 graus]", "valor fora do range");
}

/**
 * T06: Pressão num range plausível [0 hPa .. 1100 hPa]
 */
static void t06_pressure_range(void)
{
    pressure_read_async();
    for (uint32_t i = 0U; i < 200U; i++)
        pressure_tick();

    if (pressure.pressure >= 0.0f && pressure.pressure <= 1100.0f)
        TEST_PASS("T06 Pressao em range [0..1100 hPa]");
    else
        TEST_FAIL("T06 Pressao em range [0..1100 hPa]", "valor fora do range");
}

/* =========================================================================
 * GRUPO 3 — isSystemSafe() com injeção de valores
 * ========================================================================= */

/**
 * T07: Sistema saudável → isSystemSafe() == 1
 * Injeta valores dentro de todos os limites de mission.h.
 */
static void t07_system_safe_nominal(void)
{
    /* Guarda estado real */
    float sv = eps.voltage, sc = eps.current;
    float st = temperature.temperature;
    float sb = BATTERY_STATUS;

    /* Injeta valores bons */
    eps.voltage          = 4.0f;   /* dentro de [3.1 .. 6.5] */
    eps.current          = 1.5f;   /* dentro de [1 .. 2]     */
    temperature.temperature = 25.0f; /* dentro de [-10 .. 60]  */
    BATTERY_STATUS       = 80.0f;  /* acima de 30%           */

    int result = isSystemSafe();

    /* Repõe */
    eps.voltage = sv; eps.current = sc;
    temperature.temperature = st;
    BATTERY_STATUS = sb;

    if (result == 1)
        TEST_PASS("T07 isSystemSafe() com valores nominais");
    else
        TEST_FAIL("T07 isSystemSafe() com valores nominais", "retornou 0 (unsafe)");
}

/**
 * T08: Sobretensão → isSystemSafe() == 0
 */
static void t08_system_safe_overvoltage(void)
{
    float sv = eps.voltage;
    eps.voltage = MAX_SAFE_VOLTAGE + 0.5f;  /* acima do limite */

    int result = isSystemSafe();
    eps.voltage = sv;

    if (result == 0)
        TEST_PASS("T08 isSystemSafe() deteta sobretensao");
    else
        TEST_FAIL("T08 isSystemSafe() deteta sobretensao", "retornou 1 (safe) erroneamente");
}

/**
 * T09: Subtensão → isSystemSafe() == 0
 */
static void t09_system_safe_undervoltage(void)
{
    float sv = eps.voltage;
    eps.voltage = LOWEST_SAFE_VOLTAGE - 0.5f;  /* abaixo do limite */

    int result = isSystemSafe();
    eps.voltage = sv;

    if (result == 0)
        TEST_PASS("T09 isSystemSafe() deteta subtensao");
    else
        TEST_FAIL("T09 isSystemSafe() deteta subtensao", "retornou 1 (safe) erroneamente");
}

/**
 * T10: Temperatura elevada → isSystemSafe() == 0
 */
static void t10_system_safe_overtemp(void)
{
    float st = temperature.temperature;
    temperature.temperature = MAX_SAFE_TEMP + 5.0f;

    int result = isSystemSafe();
    temperature.temperature = st;

    if (result == 0)
        TEST_PASS("T10 isSystemSafe() deteta sobretemperatura");
    else
        TEST_FAIL("T10 isSystemSafe() deteta sobretemperatura", "retornou 1 (safe) erroneamente");
}

/**
 * T11: Bateria baixa → isSystemSafe() == 0
 */
static void t11_system_safe_low_battery(void)
{
    float sb = BATTERY_STATUS;
    BATTERY_STATUS = LOWEST_SAFE_BATTERY - 5.0f;

    int result = isSystemSafe();
    BATTERY_STATUS = sb;

    if (result == 0)
        TEST_PASS("T11 isSystemSafe() deteta bateria baixa");
    else
        TEST_FAIL("T11 isSystemSafe() deteta bateria baixa", "retornou 1 (safe) erroneamente");
}

/* =========================================================================
 * GRUPO 4 — FSM getMode() — transições
 *
 * Para cada teste: injeta flags globais, chama getMode(), verifica retorno.
 * Repõe sempre os flags no estado original.
 * ========================================================================= */

/* Helper: configura sistema como "safe" para testes de FSM */
static void make_system_safe(void)
{
    eps.voltage = 4.0f;
    eps.current = 1.5f;
    temperature.temperature = 25.0f;
    BATTERY_STATUS = 80.0f;
}

/* Helper: configura sistema como "unsafe" */
static void make_system_unsafe(void)
{
    eps.voltage = MAX_SAFE_VOLTAGE + 1.0f;
}

/**
 * T12: nominal_mode + sistema saudável + sem comm → permanece nominal
 */
static void t12_fsm_nominal_stays(void)
{
    int scw = COMM_WINDOW_OPEN;
    make_system_safe();
    COMM_WINDOW_OPEN = 0;

    States result = getMode(nominal_mode);
    COMM_WINDOW_OPEN = scw;

    if (result == nominal_mode)
        TEST_PASS("T12 FSM nominal sem comm -> nominal");
    else
        TEST_FAIL("T12 FSM nominal sem comm -> nominal", "transicao inesperada");
}

/**
 * T13: nominal_mode + COMM_WINDOW_OPEN=1 → communication_mode
 */
static void t13_fsm_nominal_to_comm(void)
{
    int scw = COMM_WINDOW_OPEN;
    make_system_safe();
    COMM_WINDOW_OPEN = 1;

    States result = getMode(nominal_mode);
    COMM_WINDOW_OPEN = scw;

    if (result == communication_mode)
        TEST_PASS("T13 FSM nominal + comm_window -> comm");
    else
        TEST_FAIL("T13 FSM nominal + comm_window -> comm", "transicao inesperada");
}

/**
 * T14: nominal_mode + sistema unsafe → safe_mode
 */
static void t14_fsm_nominal_to_safe(void)
{
    float sv = eps.voltage;
    make_system_unsafe();
    COMM_WINDOW_OPEN = 0;

    States result = getMode(nominal_mode);
    eps.voltage = sv;

    if (result == safe_mode)
        TEST_PASS("T14 FSM nominal + unsafe -> safe");
    else
        TEST_FAIL("T14 FSM nominal + unsafe -> safe", "transicao inesperada");
}

/**
 * T15: communication_mode + OTA_REQUESTED=1 → ota_mode
 */
static void t15_fsm_comm_to_ota(void)
{
    int sota = OTA_REQUESTED, scw = COMM_WINDOW_OPEN;
    make_system_safe();
    COMM_WINDOW_OPEN = 1;
    OTA_REQUESTED = 1;

    States result = getMode(communication_mode);
    OTA_REQUESTED = sota;
    COMM_WINDOW_OPEN = scw;

    if (result == ota_mode)
        TEST_PASS("T15 FSM comm + OTA_REQUESTED -> ota");
    else
        TEST_FAIL("T15 FSM comm + OTA_REQUESTED -> ota", "transicao inesperada");
}

/**
 * T16: communication_mode + sem comm window → nominal_mode
 */
static void t16_fsm_comm_to_nominal(void)
{
    int scw = COMM_WINDOW_OPEN, sota = OTA_REQUESTED;
    make_system_safe();
    COMM_WINDOW_OPEN = 0;
    OTA_REQUESTED = 0;

    States result = getMode(communication_mode);
    COMM_WINDOW_OPEN = scw;
    OTA_REQUESTED = sota;

    if (result == nominal_mode)
        TEST_PASS("T16 FSM comm sem window -> nominal");
    else
        TEST_FAIL("T16 FSM comm sem window -> nominal", "transicao inesperada");
}

/**
 * T17: safe_mode + sistema voltou ao normal → nominal_mode
 */
static void t17_fsm_safe_to_nominal(void)
{
    float sb = BATTERY_STATUS;
    make_system_safe();
    BATTERY_STATUS = 80.0f;  /* acima de critical (10%) */

    States result = getMode(safe_mode);
    BATTERY_STATUS = sb;

    if (result == nominal_mode)
        TEST_PASS("T17 FSM safe + sistema OK -> nominal");
    else
        TEST_FAIL("T17 FSM safe + sistema OK -> nominal", "transicao inesperada");
}

/**
 * T18: safe_mode + bateria crítica → ultra_low_power_mode
 */
static void t18_fsm_safe_to_ulp(void)
{
    float sb = BATTERY_STATUS;
    float sv = eps.voltage;
    /* Sistema unsafe (permanece em safe) mas bateria crítica → ULP */
    make_system_unsafe();
    BATTERY_STATUS = BATTERY_IN_CRITICAL_LEVEL - 1.0f;

    States result = getMode(safe_mode);
    BATTERY_STATUS = sb;
    eps.voltage = sv;

    if (result == ultra_low_power_mode)
        TEST_PASS("T18 FSM safe + bateria critica -> ULP");
    else
        TEST_FAIL("T18 FSM safe + bateria critica -> ULP", "transicao inesperada");
}

/**
 * T19: ultra_low_power_mode + bateria não crítica → safe_mode
 */
static void t19_fsm_ulp_to_safe(void)
{
    float sb = BATTERY_STATUS;
    int smt = MISSION_TIMEOUT;
    BATTERY_STATUS = BATTERY_IN_CRITICAL_LEVEL + 5.0f;
    MISSION_TIMEOUT = 0;

    States result = getMode(ultra_low_power_mode);
    BATTERY_STATUS = sb;
    MISSION_TIMEOUT = smt;

    if (result == safe_mode)
        TEST_PASS("T19 FSM ULP + bateria OK -> safe");
    else
        TEST_FAIL("T19 FSM ULP + bateria OK -> safe", "transicao inesperada");
}

/**
 * T20: decommissioning_mode → permanece decommissioning
 */
static void t20_fsm_decommissioning_stays(void)
{
    States result = getMode(decommissioning_mode);

    if (result == decommissioning_mode)
        TEST_PASS("T20 FSM decommissioning fica em decommissioning");
    else
        TEST_FAIL("T20 FSM decommissioning fica em decommissioning", "transicao inesperada");
}

/* =========================================================================
 * GRUPO 5 — API OTA do TTC
 * ========================================================================= */

/**
 * T21: No arranque, não há pacote OTA pronto
 */
static void t21_ota_api_no_packet_on_boot(void)
{
    /* Não enviamos nada — ready deve ser 0 */
    if (ttc_ota_packet_ready() == 0U)
        TEST_PASS("T21 OTA API sem pacote no arranque");
    else
        TEST_FAIL("T21 OTA API sem pacote no arranque", "packet_ready=1 sem dados");
}

/**
 * T22: Após ttc_ota_clear_ready(), flag deve ficar a 0
 */
static void t22_ota_api_clear_ready(void)
{
    ttc_ota_clear_ready();

    if (ttc_ota_packet_ready() == 0U)
        TEST_PASS("T22 OTA API clear_ready apaga flag");
    else
        TEST_FAIL("T22 OTA API clear_ready apaga flag", "flag permanece a 1");
}

/**
 * T23: ttc_ota_get_payload() retorna ponteiro não-nulo
 */
static void t23_ota_api_payload_ptr_valid(void)
{
    const uint8_t *ptr = ttc_ota_get_payload();

    if (ptr != (void *)0)
        TEST_PASS("T23 OTA API payload ptr nao e NULL");
    else
        TEST_FAIL("T23 OTA API payload ptr nao e NULL", "retornou NULL");
}

/* =========================================================================
 * GRUPO 6 — ExtMemory
 * ========================================================================= */

/**
 * T24: Após ExtMem_Init(), status deve ser IDLE
 */
static void t24_extmem_idle_after_init(void)
{
    ExtMem_Init();

    if (ExtMem_GetStatus() == EXT_MEM_IDLE)
        TEST_PASS("T24 ExtMem IDLE apos init");
    else
        TEST_FAIL("T24 ExtMem IDLE apos init", "status != EXT_MEM_IDLE");
}

/**
 * T25: OTA_REQUESTED e COMM_WINDOW_OPEN começam a 0
 * (garante que o sistema não arranca em modo errado)
 */
static void t25_mission_flags_initial_state(void)
{
    /* Estes são definidos em mission.c com valor 0 */
    if (OTA_REQUESTED == 0 && COMM_WINDOW_OPEN == 0 && MISSION_TIMEOUT == 0)
        TEST_PASS("T25 Flags de missao iniciam a 0");
    else
        TEST_FAIL("T25 Flags de missao iniciam a 0", "flag inesperadamente != 0");
}

/* =========================================================================
 * Ponto de entrada público
 * ========================================================================= */

void run_unit_tests(void)
{
    s_passed = 0U;
    s_failed = 0U;

    printf("\r\n");
    printf("  +====================================================+\r\n");
    printf("  |       TESTES UNITARIOS HIL — OBC CubeSat ISEL      |\r\n");
    printf("  +====================================================+\r\n");

    /* --- Grupo 1: SysTick -------------------------------------------- */
    printf("\r\n  -- Grupo 1: SysTick --\r\n");
    t01_systick_advances();
    t02_systick_accuracy();

    /* --- Grupo 2: Sensores (range checks) ---------------------------- */
    printf("\r\n  -- Grupo 2: Sensores --\r\n");
    t03_eps_voltage_range();
    t04_eps_current_range();
    t05_temperature_range();
    t06_pressure_range();

    /* --- Grupo 3: isSystemSafe() ------------------------------------- */
    printf("\r\n  -- Grupo 3: isSystemSafe() --\r\n");
    t07_system_safe_nominal();
    t08_system_safe_overvoltage();
    t09_system_safe_undervoltage();
    t10_system_safe_overtemp();
    t11_system_safe_low_battery();

    /* --- Grupo 4: FSM getMode() -------------------------------------- */
    printf("\r\n  -- Grupo 4: FSM getMode() --\r\n");
    t12_fsm_nominal_stays();
    t13_fsm_nominal_to_comm();
    t14_fsm_nominal_to_safe();
    t15_fsm_comm_to_ota();
    t16_fsm_comm_to_nominal();
    t17_fsm_safe_to_nominal();
    t18_fsm_safe_to_ulp();
    t19_fsm_ulp_to_safe();
    t20_fsm_decommissioning_stays();

    /* --- Grupo 5: API OTA TTC ---------------------------------------- */
    printf("\r\n  -- Grupo 5: API OTA TTC --\r\n");
    t21_ota_api_no_packet_on_boot();
    t22_ota_api_clear_ready();
    t23_ota_api_payload_ptr_valid();

    /* --- Grupo 6: ExtMemory / Flags ---------------------------------- */
    printf("\r\n  -- Grupo 6: ExtMemory e Flags --\r\n");
    t24_extmem_idle_after_init();
    t25_mission_flags_initial_state();

    /* --- Sumário final ----------------------------------------------- */
    uint8_t total = s_passed + s_failed;
    printf("\r\n");
    printf("  +====================================================+\r\n");
    printf("  |  Resultado: %2u/%2u testes passaram                  |\r\n",
           s_passed, total);
    if (s_failed == 0U)
        printf("  |  Todos os testes passaram                  [OK]    |\r\n");
    else
        printf("  |  %2u teste(s) falharam — verificar hardware [!!]   |\r\n",
               s_failed);
    printf("  +====================================================+\r\n");
    printf("\r\n");
}
