/**
 * @file test_gnss.c
 * @brief Testes unitários do periférico GNSS.
 *
 * TODO: Implementar os seguintes grupos de testes:
 *
 * GRUPO 1 — Parsing de frame nominal
 *   [ ] Frame completa válida → latitude, longitude, altitude, speed correctos
 *   [ ] Frame com coordenadas zero → valores a zero sem erro
 *   [ ] Frame com altitude negativa (sub-orbital re-entry) → parsed correctamente
 *   [ ] Checksum correcto → dados aceites
 *
 * GRUPO 2 — Detecção de erros de frame
 *   [ ] Checksum errado → dados rejeitados / não actualizados
 *   [ ] Frame incompleta (menos bytes do que esperado) → não actualiza struct
 *   [ ] Buffer de leitura vazio → gnss_read retorna erro
 *
 * GRUPO 3 — Integração com I2C driver (com mock HAL)
 *   [ ] gnss_read_async inicia pedido I2C correctamente
 *   [ ] gnss_tick chama i2c_tick enquanto transferência em curso
 *   [ ] Após callback I2C com sucesso → dados descodificados
 *   [ ] Após callback I2C com erro → gnss_data não alterado
 *
 * GRUPO 4 — Limites dos valores (ECSS §5.3)
 *   [ ] Latitude fora de [-90°, +90°] → rejeitada ou marcada inválida
 *   [ ] Longitude fora de [-180°, +180°] → rejeitada ou marcada inválida
 *   [ ] Velocidade negativa → rejeitada
 *   [ ] Altitude abaixo de -500m → rejeitada (critério missão)
 *
 * GRUPO 5 — Robustez
 *   [ ] Chamada a gnss_tick sem gnss_read_async prévia → noop seguro
 *   [ ] Chamada múltipla a gnss_read_async sem tick → segunda ignorada
 */

#include <stdio.h>
#include <stdint.h>
#include "../framework/test_runner.h"

/* TODO: incluir mocks e headers necessários */
/* #include "../mocks/mock_hal_i2c.h" */
/* #include "peripherals/gnss.h"     */

int main(void)
{
    TEST_BEGIN("GNSS — Skeleton (não implementado)");
    /* TODO: RUN_TEST(...) */
    printf("  [INFO] Testes GNSS ainda não implementados.\n");
    TEST_END();
}
