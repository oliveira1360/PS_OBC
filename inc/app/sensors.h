#ifndef SENSORS_H
#define SENSORS_H

#include "peripherals/gnss.h"
#include "peripherals/imu.h"
#include "peripherals/pressure.h"
#include "peripherals/temperature.h"
#include "peripherals/eps.h"
#include "peripherals/ttc.h"
#include "peripherals/propulsor.h"

extern gnss_data_t gnss;
extern imu_data_t imu;
extern pressure_data_t pressure;
extern temperature_data_t temperature;
extern eps_data_t eps;
extern ttc_data_t ttc;
extern propulsor_data_t propulsor;

extern float BATTERY_STATUS;
void sensors_tick(void);
void sensors_read_all(void);
void sensors_print(void);

#endif