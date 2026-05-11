/**
 * @file test_imu.c
 * @brief Testes unitários do periférico IMU (MPU-6050 / ICM-42688 via I2C).
 *
 * TODO: Implementar os seguintes grupos de testes:
 *
 * GRUPO 1 — Parsing de dados brutos (raw → float)
 *   [ ] Acelerómetro: raw 0x0000 → 0.0 g
 *   [ ] Acelerómetro: raw positivo máximo → +g_max correctamente escalado
 *   [ ] Acelerómetro: raw negativo máximo (complemento de dois) → -g_max
 *   [ ] Giroscópio: raw 0 → 0.0 °/s
 *   [ ] Magnetómetro: raw positivo e negativo → µT correctos
 *
 * GRUPO 2 — Integração com I2C driver
 *   [ ] imu_read_async inicia pedido I2C no endereço IMU_ADDR (0x68)
 *   [ ] imu_tick chama i2c_tick enquanto transferência em curso
 *   [ ] Callback I2C sucesso → imu_data_t actualizada
 *   [ ] Callback I2C erro → imu_data_t não alterada
 *
 * GRUPO 3 — Detecção de dados inválidos (FDIR)
 *   [ ] Todos os eixos a zero durante mais de N leituras → flag de erro
 *   [ ] Aceleração total |a| = 0 (sensor avariado) → detectado
 *   [ ] Overflow / saturação de eixo → detectado e reportado
 *
 * GRUPO 4 — Limites de missão
 *   [ ] Aceleração máxima no lançamento (launch lock ~5g) → dentro do range
 *   [ ] Taxa angular máxima de tumbling → dentro do range do giroscópio
 *
 * GRUPO 5 — Robustez
 *   [ ] imu_tick sem imu_read_async → noop seguro
 *   [ ] Segunda chamada a imu_read_async sem tick → ignorada
 */

#include <stdio.h>
#include <stdint.h>
#include "../framework/test_runner.h"

int main(void)
{
    TEST_BEGIN("IMU — Skeleton (não implementado)");
    printf("  [INFO] Testes IMU ainda não implementados.\n");
    TEST_END();
}
