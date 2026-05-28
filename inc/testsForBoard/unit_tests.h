#ifndef UNIT_TESTS_H
#define UNIT_TESTS_H

/**
 * @file unit_tests.h
 * @brief Testes unitários a correr diretamente na placa (HIL).
 *
 * Cobre:
 *   - SysTick (timer a contar)
 *   - Sensores EPS, Temperatura, Pressão (range checks)
 *   - Lógica isSystemSafe() com valores injetados
 *   - FSM getMode() — todas as transições
 *   - API OTA do TTC (estado inicial e clear)
 *   - ExtMemory (estado após init)
 *
 * Resultados impressos via printf (UART debug).
 */

void run_unit_tests(void);

#endif /* UNIT_TESTS_H */
