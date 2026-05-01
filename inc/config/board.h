#ifndef BOARD_H
#define BOARD_H

/* I2C */
#define I2C_MASTER_FREQ 100000U
#define GNSS_ADDR 0x42
#define IMU_ADDR 0x68
#define PRESS_ADDR 0x77
#define TEMP_ADDR 0x48
#define EPS_ADDR 0x62

/* Buffer Sizes */
#define GNSS_BUF_LEN 18
#define IMU_BUF_LEN 18
#define PRES_BUF_LEN 2
#define TEMP_BUF_LEN 2
#define EPS_BUF_LEN 2
#define TTC_BUF_LEN 16U
#define PROPULSOR_BUF_LEN 9 /* STATUS(1) + PRESS(2) + TEMP(2) + THRUST(2) + VALVE(1) + CHK(1) */

#define USE_REAL_HW 1

#endif