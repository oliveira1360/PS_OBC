/**
 * @file test_ttc.c
 * @brief Testes unitários do periférico TTC (Telemetry, Tracking & Command via USART).
 *
 * TODO: Implementar os seguintes grupos de testes:
 *
 * GRUPO 1 — Parsing de comandos recebidos (ground_command_t)
 *   [ ] CMD_NONE (0x00) → ignorado, sem mudança de estado
 *   [ ] CMD_ENTER_SAFE (0x01) → forçar transição para safe_mode
 *   [ ] CMD_REMOTE_CTRL (0x02) → modo de controlo remoto activado
 *   [ ] CMD_START_OTA (0x10) → OTA_REQUESTED = 1
 *   [ ] CMD_END_OTA (0x11) → OTA_REQUESTED = 0
 *   [ ] CMD_REQUEST_DATA (0x20) → envia telemetria
 *   [ ] Byte de comando inválido / desconhecido → ignorado
 *
 * GRUPO 2 — Envio de telemetria (ttc_send_telemetry)
 *   [ ] Frame de telemetria tem tamanho correcto (TTC_BUF_LEN = 16 bytes)
 *   [ ] Estado corrente incluído na frame
 *   [ ] Dados EPS, temperatura, pressão, GNSS, IMU presentes
 *   [ ] Doppler e ranging presentes
 *   [ ] ota_active correctamente reflectido
 *
 * GRUPO 3 — Protocolo OTA (Over-The-Air update)
 *   [ ] Pacote OTA com header válido (OTA_SYNC_WORD = 0xAA55) → aceite
 *   [ ] Pacote OTA com sync word errada → rejeitado
 *   [ ] Pacote OTA com tamanho > OTA_PACKET_SIZE → rejeitado
 *   [ ] Sequência de pacotes OTA completa → firmware aceite
 *   [ ] Timeout entre pacotes OTA → session abortada
 *
 * GRUPO 4 — Integração com USART driver
 *   [ ] ttc_read_async inicia recepção USART de TTC_CMD_LEN bytes
 *   [ ] ttc_tick chama usart_rx_tick enquanto recepção em curso
 *   [ ] Após recepção completa → comando parsed e executado
 *   [ ] Após erro de recepção → cmd_status = ACK_FAILED
 *
 * GRUPO 5 — Acknowledgment
 *   [ ] Após comando válido → cmd_status = ACK_SUCCESS
 *   [ ] Após comando inválido → cmd_status = ACK_FAILED
 *   [ ] ACK enviado via USART após processamento
 *
 * GRUPO 6 — Robustez
 *   [ ] ttc_tick sem ttc_read_async → noop
 *   [ ] Recepção com barulho (bytes extra) → sincronização correcta
 */

#include <stdio.h>
#include <stdint.h>
#include "../framework/test_runner.h"

int main(void)
{
    TEST_BEGIN("TTC — Skeleton (não implementado)");
    printf("  [INFO] Testes TTC ainda não implementados.\n");
    TEST_END();
}
