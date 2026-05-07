#ifndef PROPULSOR_H
#define PROPULSOR_H

#include <stdint.h>

#define PROP_CMD_READ         0x01U
#define PROP_CMD_VALVE_OPEN   0x02U
#define PROP_CMD_VALVE_CLOSE  0x03U
#define PROP_CMD_SET_THRUST   0x04U


typedef struct {
    uint8_t  status;    /* 0=IDLE, 1=FIRING, 2=ERROR */
    float    pressure;  /* bar */
    float    temp;      /* °C */
    float    thrust;    /* N */
    uint8_t  valve;     /* 0=fechada, 1=aberta */
} propulsor_data_t;

/**
 * @brief Inicia leitura assíncrona de telemetria do propulsor.
 */
void propulsor_read_async(void);
 
/**
 * @brief Envia comando para abrir a válvula.
 */
void propulsor_valve_open(void);
 
/**
 * @brief Envia comando para fechar a válvula.
 */
void propulsor_valve_close(void);
 
/**
 * @brief Define o thrust setpoint.
 * @param thrust_raw Valor raw (raw/10 = Newtons).
 */
void propulsor_set_thrust(uint16_t thrust_raw);
 
/**
 * @brief Tick da FSM SPI — chamar no super-loop.
 */
void propulsor_tick(void);
 

#endif