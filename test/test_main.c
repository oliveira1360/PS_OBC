/**
 * @file test_main.c
 * @brief Ponto de entrada unificado para todos os testes ECSS-E-ST-40C Rev.1
 *
 * Executa todas as suites de testes automatizados:
 *   - Unit Tests: drivers (I2C, SPI, USART)
 *   - Unit Tests: sensors (GNSS, IMU, Pressure, Temperature, EPS)
 *   - Unit Tests: sensors/parse — boundary dos valores de conversão (NOVO)
 *   - Unit Tests: app (init, modes, state_machine completa) (NOVO)
 *   - Unit Tests: i2c_queue — operações de fila (NOVO)
 *   - Integration Tests: sensor pipeline
 *   - Stress/Robustness Tests: ECSS 5.5.3.2c, 5.6.3.1
 *
 * NOTA: Os testes em test/hardware/ (HIL, timing, acceptance) NÃO são
 * incluídos aqui — requerem hardware real e são executados manualmente.
 * Ver: test/hardware/test_hil_sensors.c
 *      test/hardware/test_realtime_timing.c
 *      test/hardware/test_acceptance.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "test_utils.h"
#include "hal/hal_i2c.h"
#include "hal/hal_spi.h"
#include "hal/hal_usart.h"

/* --- Declaracoes das funcoes run_xxx_tests() --- */

/* Drivers */
void run_i2c_driver_tests(void);
void run_spi_driver_tests(void);
void run_usart_driver_tests(void);

/* Sensors — leitura e range */
void run_gnss_tests(void);
void run_imu_tests(void);
void run_pressure_tests(void);
void run_temperature_tests(void);
void run_eps_tests(void);

/* Sensors — boundary dos valores de parse (ECSS §5.5.3.2c items 1,4) */
void run_parse_boundary_tests(void);

/* App */
void run_init_tests(void);
void run_modes_tests(void);

/* App — máquina de estados completa (ECSS §5.5.3.2c — decision coverage) */
void run_state_machine_tests(void);

/* I2C Queue — operações de fila (ECSS §5.5.3.2c items 1,2,3,4,5) */
void run_i2c_queue_tests(void);

/* Integration */
void run_sensor_pipeline_tests(void);

/* Stress / Robustness */
void run_ecss_robustness_tests(void);

int main(void)
{
    /* Seed para HAL simulado (determinístico para reproducibilidade ECSS) */
    srand(42);

    /* Inicializar HALs simulados */
    hal_i2c_init();
    hal_spi_init();
    hal_usart_init();

    printf("\n");
    printf("################################################################\n");
    printf("#  ECSS-E-ST-40C Rev.1 - Test Suite Completa (OBC CubeSat)    #\n");
    printf("################################################################\n");
    printf("# Ref: §5.5.3.2 (Unit Testing) | §5.5.4 (Integration)        #\n");
    printf("#      §5.6.3.1 (Validation)   | §5.8.3.5 (Code Verification) #\n");
    printf("################################################################\n");

    /* ============================================================
       1. Unit Tests - Drivers (ECSS §5.5.3.2)
          Refs: ECSS-E-ST-40_0860087, ECSS-E-ST-40_0860088, ECSS-E-ST-40_0860089
       ============================================================ */
    run_i2c_driver_tests();
    run_spi_driver_tests();
    run_usart_driver_tests();

    /* ============================================================
       2. Unit Tests - I2C Queue (ECSS §5.5.3.2c items 1,2,3,4,5)
          Ref: ECSS-E-ST-40_0860089c — boundary, error cases, global vars
       ============================================================ */
    run_i2c_queue_tests();

    /* ============================================================
       3. Unit Tests - Sensors leitura e range (ECSS §5.5.3.2)
          Refs: ECSS-E-ST-40_0860087, ECSS-E-ST-40_0860088
       ============================================================ */
    run_gnss_tests();
    run_imu_tests();
    run_pressure_tests();
    run_temperature_tests();
    run_eps_tests();

    /* ============================================================
       4. Unit Tests - Parse Boundary (ECSS §5.5.3.2c items 1,3,4)
          Ref: ECSS-E-ST-40_0860089c — boundary n-1,n,n+1; out-of-range
               ECSS-E-ST-40_0860134a — verificação de código (§5.8.3.5)
       ============================================================ */
    run_parse_boundary_tests();

    /* ============================================================
       5. Unit Tests - App: init e modes (ECSS §5.5.3.2)
          Ref: ECSS-E-ST-40_0860087, ECSS-E-ST-40_0860088
       ============================================================ */
    run_init_tests();
    run_modes_tests();

    /* ============================================================
       6. Unit Tests - State Machine COMPLETA (ECSS §5.5.3.2c item 1)
          Ref: ECSS-E-ST-40_0860089c — "all decisions take true and false
               values at least once" (decision coverage 100%)
       ============================================================ */
    run_state_machine_tests();

    /* ============================================================
       7. Integration Tests (ECSS §5.5.4)
          Refs: ECSS-E-ST-40_0860090, ECSS-E-ST-40_0860091
       ============================================================ */
    run_sensor_pipeline_tests();

    /* ============================================================
       8. Stress / Robustness Tests (ECSS §5.5.3.2c, §5.6.3.1)
          Refs: ECSS-E-ST-40_0860089c item 5, ECSS-E-ST-40_0860097
       ============================================================ */
    run_ecss_robustness_tests();

    /* ============================================================
       Sumario final
       ============================================================ */
    printf("\n################################################################\n");
    printf("#  RESULTADOS FINAIS (testes automatizados)                    #\n");
    printf("################################################################\n");
    printf("# NOTA: Testes de hardware (HIL/timing/acceptance) em:        #\n");
    printf("#   test/hardware/test_hil_sensors.c                          #\n");
    printf("#   test/hardware/test_realtime_timing.c                      #\n");
    printf("#   test/hardware/test_acceptance.c                           #\n");
    printf("################################################################\n");
    TEST_SUMMARY();

    return TEST_RETURN();
}
