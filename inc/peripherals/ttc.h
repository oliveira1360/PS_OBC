#ifndef TTC_H
#define TTC_H
#include "peripherals/eps.h"
#include "peripherals/temperature.h"
#include "peripherals/pressure.h"
#include "peripherals/gnss.h"
#include "peripherals/imu.h"
#include "app/modes.h"
#include <stdbool.h>


typedef enum
{
    CMD_NONE = 0x00,
    CMD_ENTER_SAFE = 0x01,
    CMD_REMOTE_CTRL = 0x02,
    CMD_START_OTA = 0x10,
    CMD_END_OTA = 0x11,
    CMD_RECEIVING_OTA = 0x12,
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


#endif