#ifndef BOARD_H
#define BOARD_H

/* I2C Addresses */
#define GNSS_ADDR 0x42
#define IMU_ADDR 0x68
#define PRESS_ADDR 0x77
#define TEMP_ADDR 0x48
#define EPS_ADDR 0x62

/* Buffer Sizes */
#define GNSS_BUF_LEN 8
#define IMU_BUF_LEN 18
#define PRES_BUF_LEN 2
#define TEMP_BUF_LEN 2
#define EPS_BUF_LEN 2
#define TTC_BUF_LEN 4U
#define PROPULSOR_BUF_LEN 2

#endif