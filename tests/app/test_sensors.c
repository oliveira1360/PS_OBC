/**
 * @file test_sensors.c
 * @brief Testes unitários da camada de sensores (sensors.c — sensors_read_all, sensors_tick).
 *
 * TODO: Implementar os seguintes grupos de testes:
 *
 * GRUPO 1 — sensors_read_all()
 *   [ ] Chama read_async de todos os periféricos (GNSS, IMU, Pressure, Temp, EPS, TTC, Prop)
 *   [ ] Não chama read_async se já estiver em curso para o mesmo periférico
 *   [ ] Após todos os callbacks de sucesso → dados globais actualizados
 *   [ ] Após callback de erro em 1 periférico → restantes não afectados
 *
 * GRUPO 2 — sensors_tick()
 *   [ ] Chama tick de todos os periféricos em cada chamada
 *   [ ] Sem crash se algum periférico não foi inicializado
 *
 * GRUPO 3 — sensors_print()
 *   [ ] Imprime dados de todos os sensores sem crash
 *   [ ] Com valores nominais → output formatado correctamente
 *   [ ] Com valores extremos (NaN, ±inf) → não causa crash
 *
 * GRUPO 4 — Timing (integração com hal_systick)
 *   [ ] sensors_read_all não é chamado mais de 1 vez por TIME_TO_UPDATE_VALUES ms
 *   [ ] Após TIME_TO_UPDATE_VALUES ms → nova leitura é iniciada
 */

#include <stdio.h>
#include <stdint.h>
#include "../framework/test_runner.h"

int main(void)
{
    TEST_BEGIN("Sensors App — Skeleton (não implementado)");
    printf("  [INFO] Testes Sensors ainda não implementados.\n");
    TEST_END();
}
