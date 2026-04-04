#ifndef HAL_PERIPHERALS_INIT_H
#define HAL_PERIPHERALS_INIT_H

#include <stdint.h>


uint8_t hal_gnss_init(void);
uint8_t hal_imu_init(void);
uint8_t hal_pressure_init(void);
uint8_t hal_temperature_init(void);
uint8_t hal_eps_init(void);
uint8_t hal_ttc_init(void);
uint8_t hal_ext_memory_init(void);

#endif 