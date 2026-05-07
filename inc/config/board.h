#ifndef BOARD_H
#define BOARD_H

/* I2C */
#define GNSS_ADDR 0x42
#define IMU_ADDR 0x68
#define PRESS_ADDR 0x77
#define TEMP_ADDR 0x48
#define EPS_ADDR 0x60
#define I2C_SPEED_KHZ 25

/* Buffer Sizes */
#define GNSS_BUF_LEN 18
#define IMU_BUF_LEN 18
#define PRES_BUF_LEN 2
#define TEMP_BUF_LEN 2
#define EPS_BUF_LEN 2
#define TTC_BUF_LEN 16U
#define PROPULSOR_BUF_LEN 9 /* STATUS(1) + PRESS(2) + TEMP(2) + THRUST(2) + VALVE(1) + CHK(1) */

/* OTA */
#define OTA_PACKET_SIZE 128
#define OTA_HEADER_SIZE 6
#define OTA_FULL_PACKET (OTA_HEADER_SIZE + OTA_PACKET_SIZE) /* 134 */
#define OTA_SYNC_WORD 0xAA55

/* QSPI / W25Q128 External Memory */
#define QSPI_TIMEOUT_MAX 5000U
#define QSPI_RETRY_MAX 3U

/*SPI */
#define PROPULSOR_CS_PIN 25U

#define USE_REAL_HW 1

#endif