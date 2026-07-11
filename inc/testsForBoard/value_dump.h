#ifndef VALUE_DUMP_H
#define VALUE_DUMP_H

/**
 * @file value_dump.h
 * @brief Diagnóstico HIL — imprime TODOS os valores do sistema via UART debug.
 *
 * Pensado para correr uma vez no arranque (como os outros testes desta pasta),
 * ou em modo "live" para observar a telemetria a mudar em tempo real.
 *
 * Imprime:
 *   - Leitura de todos os sensores (GNSS, IMU, EPS, Temperatura, Pressão, Propulsor)
 *   - Estado de missão (modo activo, bateria, flags, isSystemSafe)
 *   - Limiares de mission.h (para comparação directa)
 *   - Pacote de telemetria ttc_data_t completo + dump hexadecimal
 *   - Conteúdo das 3 regiões da flash externa (config / logs / OTA)
 *   - Tempo de iteração e jitter do motor de FSMs (evidência de determinismo)
 *
 * Todos os floats são impressos via inteiro.milésimos (não depende de %f no printf
 * do XC32, que muitas vezes não está activo).
 *
 * Requer USE_REAL_HW 1 em board.h para ler hardware real.
 */

#include <stdint.h>

/** @brief Lê e imprime todos os sensores + estado de missão + limiares. */
void dump_all_values(void);

/** @brief Lê ttc_data_t via ttc_read() e imprime todos os campos + hex. */
void dump_telemetry_packet(void);

/** @brief Lê e imprime as 3 regiões da flash externa (config/logs/OTA). */
void dump_ext_memory(void);

/** @brief Mede tempo/jitter do motor de FSMs (sensors_tick) — determinismo. */
void dump_loop_timing(void);

/**
 * @brief Imprime telemetria em contínuo, uma linha por amostra.
 * @param samples   Número de amostras a imprimir.
 * @param period_ms Intervalo entre amostras (ms).
 */
void dump_live(uint32_t samples, uint32_t period_ms);

/** @brief Corre todos os dumps acima, uma vez. */
void run_value_dump(void);

#endif /* VALUE_DUMP_H */
