#ifndef MISSION_H
#define MISSION_H
#include <stdint.h>

#define MAX_SAFE_VOLTAGE    6.5f
#define LOWEST_SAFE_VOLTAGE 3.1f
#define MAX_SAFE_CURRENT    2.0f
#define LOWEST_SAFE_CURRENT 1.0f

#define MAX_SAFE_TEMP 60
#define LOWEST_SAFE_TEMP -10

#define LOWEST_SAFE_BATTERY 30
#define BATTERY_IN_CRITICAL_LEVEL 15

#define SAFE_DATA_NOMINAL_MODE 10000 // 10 segundos 
#define zSAFE_DATA_SAFE_MODE 2500 // 2,5 segundos 

extern int COMM_WINDOW_OPEN;
extern int OTA_REQUESTED;
extern int MISSION_TIMEOUT;
extern uint16_t TIME_TO_UPDATE_VALUES;

#endif