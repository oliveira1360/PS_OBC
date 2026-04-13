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
    eps_data_t eps;
    temperature_data_t temp;
    pressure_data_t press;
    gnss_data_t gnss;
    imu_data_t imu;
    float doppler;   // Efeito Doppler
    uint16_t raging; // Medição de Distância
    uint8_t current_state;
    bool ota_active;
    ground_command_t last_command;
    cmd_status_t cmd_status;
} ttc_data_t;


int ttc_read(ttc_data_t *out);
void ttc_read_async();
void ttc_tick(void);
void ttc_send_telemetry(void);


#endif