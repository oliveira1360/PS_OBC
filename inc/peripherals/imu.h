#ifndef IMU_H
#define IMU_H

#include <stdint.h>

typedef struct
{
    float ax, ay, az; /* Acelerometro */
    float gx, gy, gz; /* Giroscopio   */
    float mx, my, mz; /* Magnetometro */
} imu_data_t;         /* 288 bits / 36 bytes */

int  imu_read(imu_data_t *out);
void imu_read_async(void);
void imu_tick(void);

#endif /* IMU_H */
