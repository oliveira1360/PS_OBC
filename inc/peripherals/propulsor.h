#ifndef PROPULSOR_H
#define PROPULSOR_H

#include <stdint.h>

#define PROPULSOR_CS_PIN 5

/* =========================================================================
   Comandos enviados pelo OBC ao propulsor via SPI
   ========================================================================= */
typedef enum
{
    PROP_CMD_NONE        = 0x00U,  /* sem comando                        */
    PROP_CMD_IGNITE      = 0x01U,  /* ignição do motor                   */
    PROP_CMD_SHUTDOWN    = 0x02U,  /* desligar motor                     */
    PROP_CMD_SET_THRUST  = 0x10U,  /* definir nível de impulso           */
    PROP_CMD_REQUEST_TLM = 0x20U,  /* pedir telemetria do propulsor      */
} propulsor_cmd_t;

/* =========================================================================
   Status devolvido pelo propulsor
   ========================================================================= */
typedef enum
{
    PROP_STATUS_IDLE     = 0x00U,  /* motor desligado                    */
    PROP_STATUS_BURNING  = 0x01U,  /* motor a queimar                    */
    PROP_STATUS_FAULT    = 0x02U,  /* falha detectada                    */
} propulsor_status_t;

/* =========================================================================
   Dados de telemetria do propulsor
   ========================================================================= */
typedef struct
{
    float              chamber_pressure;  /* pressão da câmara em bar    */
    float              temperature;       /* temperatura do motor em °C  */
    float              thrust;            /* impulso actual em N         */
    uint8_t            valve_state;       /* estado da válvula 0=fechada */
    propulsor_status_t status;            /* estado geral do propulsor   */
    propulsor_cmd_t    last_command;      /* último comando enviado      */
} propulsor_data_t;

/* =========================================================================
   Interface pública
   ========================================================================= */
void propulsor_read_async(void);
void propulsor_send_cmd(propulsor_cmd_t cmd, uint8_t payload);
void propulsor_tick(void);
propulsor_data_t *propulsor_get(void);

#endif