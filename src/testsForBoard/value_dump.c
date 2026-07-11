/**
 * @file value_dump.c
 * @brief Diagnóstico HIL — imprime TODOS os valores do sistema via UART debug.
 *
 * Corre uma vez no arranque (como os outros testes em testsForBoard/).
 * Não usa %f: todos os floats são impressos como inteiro.milésimos, para
 * funcionar mesmo quando o printf do XC32 não tem suporte a vírgula flutuante.
 *
 * Para usar: chamar run_value_dump() em main.c, após system_init() e antes
 * do super-loop (tal como test_qspi_rw() / run_unit_tests()).
 */

#include "testsForBoard/value_dump.h"
#include "app/sensors.h"
#include "app/mission.h"
#include "app/modes.h"
#include "hal/hal_systick.h"
#include "hal/hal_qspi.h"
#include "config/board.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* =========================================================================
 * Utilitários
 * ========================================================================= */

/** Imprime um float como inteiro.milésimos + unidade (sem depender de %f). */
static void pf(const char *label, float v, const char *unit)
{
    int  neg   = (v < 0.0f);
    float a    = neg ? -v : v;
    long whole = (long)a;
    long frac  = (long)((a - (float)whole) * 1000.0f + 0.5f);
    if (frac >= 1000) { whole++; frac -= 1000; }
    printf("    %-22s %s%ld.%03ld %s\r\n", label, neg ? "-" : "", whole, frac, unit);
}

/** Nome legível de cada modo da FSM. */
static const char *mode_name(States s)
{
    switch (s)
    {
    case nominal_mode:          return "NOMINAL";
    case communication_mode:    return "COMMUNICATION";
    case ota_mode:              return "OTA";
    case safe_mode:             return "SAFE";
    case ultra_low_power_mode:  return "ULTRA_LOW_POWER";
    case decommissioning_mode:  return "DECOMMISSIONING";
    default:                    return "?";
    }
}

/** Nome do estado do propulsor. */
static const char *prop_status_name(uint8_t st)
{
    switch (st)
    {
    case 0U: return "IDLE";
    case 1U: return "FIRING";
    case 2U: return "ERROR";
    default: return "?";
    }
}

/** Busy-wait em ms via SysTick. */
static void delay_ms(uint32_t ms)
{
    uint32_t start = hal_systick_get_ms();
    while ((hal_systick_get_ms() - start) < ms)
        ;
}

/** Dispara a leitura de todos os sensores e dá tempo às FSMs para completarem. */
static void read_all_sensors(uint32_t settle_ticks)
{
    gnss_read_async();
    imu_read_async();
    eps_read_async();
    temperature_read_async();
    pressure_read_async();
    propulsor_read_async();
    for (uint32_t i = 0U; i < settle_ticks; i++)
        sensors_tick();
}

/** Dump hexadecimal de um buffer, 16 bytes por linha. */
static void hex_dump(const uint8_t *buf, uint32_t len)
{
    for (uint32_t i = 0U; i < len; i++)
    {
        if ((i % 16U) == 0U)
            printf("\r\n      %04lX: ", (unsigned long)i);
        printf("%02X ", (unsigned)buf[i]);
    }
    printf("\r\n");
}

/* =========================================================================
 * 1 — Todos os sensores + estado de missão + limiares
 * ========================================================================= */
void dump_all_values(void)
{
    printf("\r\n  +====================================================+\r\n");
    printf("  |  DUMP 1/4 — SENSORES e ESTADO DE MISSAO            |\r\n");
    printf("  +====================================================+\r\n");

    read_all_sensors(500U);

    printf("\r\n  -- GNSS (0x%02X) --\r\n", (unsigned)GNSS_ADDR);
    pf("latitude",  gnss.latitude,  "deg");
    pf("longitude", gnss.longitude, "deg");
    pf("altitude",  gnss.altitude,  "m");
    pf("speed",     gnss.speed,     "m/s");

    printf("\r\n  -- IMU (0x%02X) --\r\n", (unsigned)IMU_ADDR);
    pf("accel.x", imu.ax, "g");   pf("accel.y", imu.ay, "g");   pf("accel.z", imu.az, "g");
    pf("gyro.x",  imu.gx, "dps"); pf("gyro.y",  imu.gy, "dps"); pf("gyro.z",  imu.gz, "dps");
    pf("mag.x",   imu.mx, "uT");  pf("mag.y",   imu.my, "uT");  pf("mag.z",   imu.mz, "uT");

    printf("\r\n  -- EPS (0x%02X) --\r\n", (unsigned)EPS_ADDR);
    pf("voltage", eps.voltage, "V");
    pf("current", eps.current, "A");

    printf("\r\n  -- Ambiente --\r\n");
    pf("temperature", temperature.temperature, "degC");
    pf("pressure",    pressure.pressure,        "hPa");

    printf("\r\n  -- Propulsor (SPI) --\r\n");
    printf("    %-22s %s\r\n", "status", prop_status_name(propulsor.status));
    pf("pressure", propulsor.pressure, "bar");
    pf("temp",     propulsor.temp,     "degC");
    pf("thrust",   propulsor.thrust,   "N");
    printf("    %-22s %u\r\n", "valve", (unsigned)propulsor.valve);

    printf("\r\n  -- Estado de Missao --\r\n");
    printf("    %-22s %s\r\n", "modo activo", mode_name(mission_state_get()));
    pf("bateria", mission_battery_get(), "%");
    printf("    %-22s %d\r\n", "comm_window",   mission_comm_window_get());
    printf("    %-22s %d\r\n", "ota_requested", mission_ota_requested_get());
    printf("    %-22s %d\r\n", "timeout",       mission_timeout_get());
    printf("    %-22s %d\r\n", "isSystemSafe()", isSystemSafe());

    printf("\r\n  -- Limiares (mission.h) --\r\n");
    pf("MAX_SAFE_VOLTAGE",    MAX_SAFE_VOLTAGE,    "V");
    pf("LOWEST_SAFE_VOLTAGE", LOWEST_SAFE_VOLTAGE, "V");
    pf("MAX_SAFE_CURRENT",    MAX_SAFE_CURRENT,    "A");
    pf("LOWEST_SAFE_CURRENT", LOWEST_SAFE_CURRENT, "A");
    pf("MAX_SAFE_TEMP",       (float)MAX_SAFE_TEMP, "degC");
    pf("LOWEST_SAFE_TEMP",    (float)LOWEST_SAFE_TEMP, "degC");
    printf("    %-22s %d %%\r\n", "LOWEST_SAFE_BATTERY",      LOWEST_SAFE_BATTERY);
    printf("    %-22s %d %%\r\n", "BATTERY_IN_CRITICAL",      BATTERY_IN_CRITICAL_LEVEL);
}

/* =========================================================================
 * 2 — Pacote de telemetria ttc_data_t completo
 * ========================================================================= */
void dump_telemetry_packet(void)
{
    ttc_data_t pkt;

    printf("\r\n  +====================================================+\r\n");
    printf("  |  DUMP 2/4 — PACOTE TELEMETRIA ttc_data_t (%2u B)    |\r\n",
           (unsigned)sizeof(ttc_data_t));
    printf("  +====================================================+\r\n");

    (void)ttc_read(&pkt);

    pf("eps.voltage",  pkt.eps.voltage,   "V");
    pf("eps.current",  pkt.eps.current,   "A");
    pf("temperature",  pkt.temp.temperature, "degC");
    pf("pressure",     pkt.press.pressure,   "hPa");
    pf("gnss.lat",     pkt.gnss.latitude,  "deg");
    pf("gnss.lon",     pkt.gnss.longitude, "deg");
    pf("gnss.alt",     pkt.gnss.altitude,  "m");
    pf("doppler",      pkt.doppler,        "kHz");
    printf("    %-22s %u\r\n",       "ranging",       (unsigned)pkt.raging);
    printf("    %-22s %s\r\n",       "current_state", mode_name((States)pkt.current_state));
    printf("    %-22s %u\r\n",       "ota_active",    (unsigned)pkt.ota_active);
    printf("    %-22s 0x%02X\r\n",   "last_command",  (unsigned)pkt.last_command);
    printf("    %-22s %u\r\n",       "cmd_status",    (unsigned)pkt.cmd_status);

    printf("\r\n  -- Dump hexadecimal cru (%u bytes) --", (unsigned)sizeof(ttc_data_t));
    hex_dump((const uint8_t *)&pkt, (uint32_t)sizeof(ttc_data_t));
}

/* =========================================================================
 * 3 — Conteúdo das 3 regiões da flash externa
 * ========================================================================= */
void dump_ext_memory(void)
{
    uint8_t buf[32];

    printf("\r\n  +====================================================+\r\n");
    printf("  |  DUMP 3/4 — FLASH EXTERNA S25FL116K (2 MB)         |\r\n");
    printf("  +====================================================+\r\n");

    printf("\r\n  -- CONFIG @ 0x%06lX (primeiros 32 B) --",
           (unsigned long)MEM_REGION_CONFIG_START);
    hal_qspi_read_memory(MEM_REGION_CONFIG_START, buf, sizeof(buf));
    hex_dump(buf, sizeof(buf));

    printf("\r\n  -- LOGS   @ 0x%06lX (primeiros 32 B) --",
           (unsigned long)MEM_REGION_LOGS_START);
    hal_qspi_read_memory(MEM_REGION_LOGS_START, buf, sizeof(buf));
    hex_dump(buf, sizeof(buf));

    printf("\r\n  -- OTA    @ 0x%06lX (primeiros 32 B) --",
           (unsigned long)MEM_REGION_OTA_START);
    hal_qspi_read_memory(MEM_REGION_OTA_START, buf, sizeof(buf));
    hex_dump(buf, sizeof(buf));
}

/* =========================================================================
 * 4 — Tempo de iteração e jitter do motor de FSMs (determinismo)
 * ========================================================================= */
void dump_loop_timing(void)
{
    const uint32_t K = 2000U;   /* iterações de sensors_tick() por ronda */
    const uint32_t R = 10U;     /* número de rondas                     */
    uint32_t tmin = 0xFFFFFFFFUL, tmax = 0U, tsum = 0U;

    printf("\r\n  +====================================================+\r\n");
    printf("  |  DUMP 4/4 — TEMPO / JITTER do motor de FSMs        |\r\n");
    printf("  +====================================================+\r\n");
    printf("    %lu iteracoes de sensors_tick() por ronda, %lu rondas\r\n",
           (unsigned long)K, (unsigned long)R);

    /* Aquece os caminhos de leitura antes de medir. */
    read_all_sensors(50U);

    for (uint32_t r = 0U; r < R; r++)
    {
        uint32_t t0 = hal_systick_get_ms();
        for (uint32_t i = 0U; i < K; i++)
            sensors_tick();
        uint32_t dt = hal_systick_get_ms() - t0;

        if (dt < tmin) tmin = dt;
        if (dt > tmax) tmax = dt;
        tsum += dt;
        printf("    ronda %2lu: %lu ms\r\n", (unsigned long)(r + 1U), (unsigned long)dt);
    }

    uint32_t tavg = tsum / R;
    uint32_t per_iter_us = (tavg * 1000UL) / K;   /* microssegundos por iteração */

    printf("\r\n    min=%lu ms  max=%lu ms  avg=%lu ms  jitter(max-min)=%lu ms\r\n",
           (unsigned long)tmin, (unsigned long)tmax,
           (unsigned long)tavg, (unsigned long)(tmax - tmin));
    printf("    custo medio por iteracao ~= %lu us\r\n", (unsigned long)per_iter_us);
}

/* =========================================================================
 * Modo "live" — telemetria contínua
 * ========================================================================= */
void dump_live(uint32_t samples, uint32_t period_ms)
{
    printf("\r\n  -- LIVE: %lu amostras a cada %lu ms --\r\n",
           (unsigned long)samples, (unsigned long)period_ms);
    printf("    %-6s %-9s %-9s %-9s %-9s %-9s\r\n",
           "n", "V", "A", "degC", "hPa", "modo");

    for (uint32_t n = 0U; n < samples; n++)
    {
        read_all_sensors(300U);

        /* Tudo numa linha, floats em mili-unidades para legibilidade compacta. */
        printf("    %-6lu %ld.%03ld   %ld.%03ld   %ld.%03ld   %ld.%03ld   %s\r\n",
               (unsigned long)n,
               (long)eps.voltage,        (long)((eps.voltage - (long)eps.voltage) * 1000.0f),
               (long)eps.current,        (long)((eps.current - (long)eps.current) * 1000.0f),
               (long)temperature.temperature, (long)((temperature.temperature - (long)temperature.temperature) * 1000.0f),
               (long)pressure.pressure,  (long)((pressure.pressure - (long)pressure.pressure) * 1000.0f),
               mode_name(mission_state_get()));

        delay_ms(period_ms);
    }
}

/* =========================================================================
 * Ponto de entrada
 * ========================================================================= */
void run_value_dump(void)
{
    printf("\r\n");
    printf("  ######################################################\r\n");
    printf("  #   VALUE DUMP — DIAGNOSTICO COMPLETO DO OBC (HIL)   #\r\n");
    printf("  ######################################################\r\n");

    dump_all_values();
    dump_telemetry_packet();
    dump_ext_memory();
    dump_loop_timing();

    /* Para telemetria contínua, descomentar (ex.: 20 amostras a 1 s): */
    /* dump_live(20U, 1000U); */

    printf("\r\n  ######  FIM DO VALUE DUMP  ######\r\n\r\n");
}
