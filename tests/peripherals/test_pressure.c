/**
 * @file test_pressure.c
 * @brief Testes unitários do periférico de pressão (BMP280 / similar via I2C).
 *
 * TODO: Implementar os seguintes grupos de testes:
 *
 * GRUPO 1 — Conversão raw → Pa / hPa
 *   [ ] Raw de pressão atmosférica standard (~101325 Pa) → valor correcto
 *   [ ] Raw em altitude de órbita baixa (~0 Pa) → 0 ou mínimo detectável
 *   [ ] Compensação de temperatura aplicada correctamente
 *   [ ] Overflow de ADC de pressão → tratado sem crash
 *
 * GRUPO 2 — Integração com I2C driver
 *   [ ] pressure_read_async inicia pedido I2C no PRESS_ADDR (0x77)
 *   [ ] pressure_tick chama i2c_tick correctamente
 *   [ ] Callback sucesso → pressure_data_t actualizada
 *   [ ] Callback erro → pressure_data_t não alterada
 *
 * GRUPO 3 — Limites de missão
 *   [ ] Pressão abaixo de limiar de vácuo → detectada
 *   [ ] Pressão de propulsão (câmara) dentro do range esperado
 *   [ ] Pressão de câmara fora do range → flag de anomalia
 *
 * GRUPO 4 — Robustez
 *   [ ] tick sem read_async → noop
 *   [ ] Segunda chamada sem tick → ignorada
 */

#include <stdio.h>
#include <stdint.h>
#include "../framework/test_runner.h"

int main(void)
{
    TEST_BEGIN("Pressure — Skeleton (não implementado)");
    printf("  [INFO] Testes Pressure ainda não implementados.\n");
    TEST_END();
}
