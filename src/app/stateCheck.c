#include <stdio.h>
#include "app/states.h"
#include "app/sensors.h"

/**
 * @brief Verifica o estado do sistema e aplica degradação de bateria.
 *
 * Cada ciclo de operação consome ~20% de carga da bateria.
 * Utilizado nos testes de stress para validar o comportamento da
 * máquina de estados sob condições de bateria degradada (ECSS §5.6.3.1).
 */
void stateCheck(void)
{
    BATTERY_STATUS -= 20.0f;
}
