#ifndef STATES_H
#define STATES_H

void stateCheck(void);

extern float BATTERY_STATUS;
extern float EPS_BUS_VOLTAGE;
extern float TEMP_INTERNAL;
extern float PRESSURE_SENSOR;
extern float IMU_ACCEL_Z;
extern float GNSS_SATS;
extern float TT_C_PWR;

extern int COMM_WINDOW_OPEN;
extern int OTA_REQUESTED;
extern int MISSION_TIMEOUT;
#endif 