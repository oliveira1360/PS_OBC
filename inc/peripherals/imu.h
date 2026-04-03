#ifndef IMU_H
#define IMU_H

#include <stdint.h>

typedef struct {
    float ax, ay, az; // Acelerómetro
    float gx, gy, gz; // Giroscópio
    float mx, my, mz; // Magnetómetro
} imu_data_t;

int imu_read(imu_data_t *out);
void imu_read_async() ;
void imu_tick(void) ;

#endif