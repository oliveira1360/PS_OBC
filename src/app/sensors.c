#include <stdio.h>
#include "app/sensors.h"
#include "drivers/i2c_driver.h"

gnss_data_t gnss = {0};
imu_data_t imu = {0};
pressure_data_t pressure = {0};
temperature_data_t temperature = {0};
eps_data_t eps = {0};

float BATTERY_STATUS = 100.0f;
float EPS_BUS_VOLTAGE = 8.2f;
float TEMP_INTERNAL = 25.0f;

void sensors_tick(void)
{
    i2c_tick(&i2c_master);
}

void sensors_read_all(void)
{
    gnss_read_async();
    imu_read_async();
    pressure_read_async();
    temperature_read_async();
    eps_read_async();
}

void sensors_print(void)
{
    printf("+----------------------------------------------------------------------+\n");
    printf("|                            TELEMETRY DATA                            |\n");
    printf("+----------------------------------------------------------------------+\n");
    printf("| --- GNSS ----------------------------------------------------------- |\n");
    printf("|  Latitude:    %-10.2f deg                                         |\n", gnss.latitude);
    printf("|  Longitude:   %-10.2f deg                                         |\n", gnss.longitude);
    printf("|  Altitude:    %-10.2f m                                           |\n", gnss.altitude);
    printf("|  Speed:       %-10.2f m/s                                         |\n", gnss.speed);
    printf("|                                                                      |\n");
    printf("| --- IMU ------------------------------------------------------------ |\n");
    printf("|  Accel:  X=%-7.2f Y=%-7.2f Z=%-7.2f m/s2                          |\n", imu.ax, imu.ay, imu.az);
    printf("|  Gyro:   X=%-7.2f Y=%-7.2f Z=%-7.2f deg/s                         |\n", imu.gx, imu.gy, imu.gz);
    printf("|  Mag:    X=%-7.2f Y=%-7.2f Z=%-7.2f uT                            |\n", imu.mx, imu.my, imu.mz);
    printf("|                                                                      |\n");
    printf("| --- Pressure & Temperature ----------------------------------------- |\n");
    printf("|  Pressure:    %-10.2f hPa                                         |\n", pressure.pressure);
    printf("|  Temperature: %-10.2f C                                           |\n", temperature.temperature);
    printf("|                                                                      |\n");
    printf("| --- EPS ------------------------------------------------------------ |\n");
    printf("|  Voltage:     %-10.2f V                                           |\n", eps.voltage);
    printf("|  Current:     %-10.2f A                                           |\n", eps.current);
    printf("+----------------------------------------------------------------------+\n\n");
    printf("\n\n\n\n\n");
}