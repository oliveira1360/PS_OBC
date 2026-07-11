#ifndef TTC_H
#define TTC_H
#include "peripherals/eps.h"
#include "peripherals/temperature.h"
#include "peripherals/pressure.h"
#include "peripherals/gnss.h"
#include "peripherals/imu.h"
#include "app/modes.h"
#include <stdbool.h>


#define TTC_CMD_LEN  4U


typedef enum
{
    CMD_NONE = 0x00,
    CMD_START_OTA = 0x10,
    CMD_END_OTA = 0x11,
    CMD_REQUEST_DATA = 0x20
} ground_command_t;

typedef enum
{
    ACK_WAITING = 0x00,
    ACK_SUCCESS = 0x01,
    ACK_FAILED = 0x02
} cmd_status_t;

typedef struct
{
    eps_data_t eps; // 8 bytes
    temperature_data_t temp; // 4 bytes
    pressure_data_t press; // 4 bytes
    gnss_data_t gnss; // 16 bytes
    imu_data_t imu;   // 36 bytes
    float doppler;   // Efeito Doppler // 4 bytes
    uint16_t raging; // Medição de Distância // 2 bytes
    uint8_t current_state; // 1 byte
    bool ota_active; // 1 bytes
    ground_command_t last_command; // 4 bytes
    cmd_status_t cmd_status; // 4 bytes
} ttc_data_t; // 84 bytes


int ttc_read(ttc_data_t *out);
void ttc_read_async();
void ttc_tick(void);
void ttc_send_telemetry(void);

/* =========================================================================
 * API de acesso aos pacotes OTA recebidos — usada pelo otaMode()
 *
 * Quando um pacote OTA é recebido e validado pelo TTC (sync word correcto),
 * o payload fica disponível até ttc_ota_clear_ready() ser chamado.
 * ========================================================================= */

/** @return 1 se há um pacote OTA pronto para processar, 0 caso contrário. */
uint8_t ttc_ota_packet_ready(void);

/** @return Número de sequência do pacote pronto (0xFFFF = marcador de fim). */
uint16_t ttc_ota_get_seq(void);

/** @return Ponteiro para o payload do pacote (OTA_PACKET_SIZE bytes max). */
const uint8_t *ttc_ota_get_payload(void);

/** @return Número de bytes válidos no payload do pacote actual. */
uint16_t ttc_ota_get_payload_len(void);

/** @brief Limpa o flag de pacote pronto — deve ser chamado após processar. */
void ttc_ota_clear_ready(void);

/** @brief Repõe estado OTA após abort — permite que próximo CMD_START_OTA seja tratado correctamente. */
void ttc_ota_abort(void);

void ttc_send_ota_ready(void);


#endif