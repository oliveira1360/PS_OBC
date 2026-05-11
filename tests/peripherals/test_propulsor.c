/**
 * @file test_propulsor.c
 * @brief Testes unitários do periférico propulsor (via SPI).
 *
 * TODO: Implementar os seguintes grupos de testes:
 *
 * GRUPO 1 — Parsing de frame de telemetria (9 bytes)
 *   Formato: [STATUS][PRESS_H][PRESS_L][TEMP_H][TEMP_L][RSV][RSV][RSV][CHK]
 *   [ ] Frame válida → status, pressure, temp, checksum correctos
 *   [ ] Checksum XOR válido → dados aceites
 *   [ ] Checksum XOR inválido → dados rejeitados / não actualizados
 *   [ ] STATUS=0x00 (IDLE) → propulsor em idle
 *   [ ] STATUS=0x01 (FIRING) → propulsor a disparar
 *   [ ] STATUS=0x02 (ERROR) → flag de erro activa
 *   [ ] Pressão raw → float em bar (escalonamento correcto)
 *   [ ] Temperatura raw → float em °C
 *
 * GRUPO 2 — Comandos enviados ao propulsor
 *   [ ] propulsor_valve_open() → envia PROP_CMD_VALVE_OPEN (0x02)
 *   [ ] propulsor_valve_close() → envia PROP_CMD_VALVE_CLOSE (0x03)
 *   [ ] propulsor_set_thrust(N) → envia PROP_CMD_SET_THRUST (0x04) + valor
 *   [ ] propulsor_read_async() → envia PROP_CMD_READ (0x01)
 *   [ ] Comandos não são enviados enquanto SPI está ocupado
 *
 * GRUPO 3 — Integração com SPI driver
 *   [ ] propulsor_read_async inicia spi_transfer_async com CS PROPULSOR_CS_PIN
 *   [ ] propulsor_tick chama spi_tick enquanto transferência em curso
 *   [ ] Após callback SPI sucesso (0) → frame parsed correctamente
 *   [ ] Após callback SPI erro (-1) → propulsor_data não alterada
 *
 * GRUPO 4 — Limites de segurança de propulsão
 *   [ ] Pressão > P_max → flag de overpress, válvula fecha automaticamente
 *   [ ] Temperatura > T_max → flag de sobreaquecimento, thrust=0
 *   [ ] STATUS=ERROR → sistemas de segurança activados
 *   [ ] Thrust setpoint > limite físico → saturado ao máximo
 *
 * GRUPO 5 — Sequência de operação (valve safety)
 *   [ ] Não é possível abrir válvula se STATUS=ERROR
 *   [ ] set_thrust(0) fecha válvula correctamente
 *   [ ] Abertura de válvula só possível a partir de IDLE
 *
 * GRUPO 6 — Robustez
 *   [ ] tick sem read_async → noop
 *   [ ] Frame incompleta (< 9 bytes) → não actualiza struct
 *   [ ] Todos os bytes 0xFF (bus desligado) → detectado como erro
 */

#include <stdio.h>
#include <stdint.h>
#include "../framework/test_runner.h"

int main(void)
{
    TEST_BEGIN("Propulsor — Skeleton (não implementado)");
    printf("  [INFO] Testes Propulsor ainda não implementados.\n");
    TEST_END();
}
