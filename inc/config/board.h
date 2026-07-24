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
#define TTC_BUF_LEN 80U   /* frame de telemetria alargado: 0x20 + 18 floats + estado + checksum = 75 B */
#define PROPULSOR_BUF_LEN 8

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

#define SYS_HCLK_HZ 300000000UL   /* clock do core (CPU / SysTick)        */
#define SYS_MCK_HZ  150000000UL   /* clock dos periféricos (MCK = HCLK/2) */
#define USART_BRGR_FOR(baud) ((SYS_MCK_HZ) / (16UL * (unsigned long)(baud)))
#define FAST_BOOT 1 // 0 diagrma
#define STABILIZE_TIMEOUT_MS 30000UL   /* espera pos-boot quando FAST_BOOT=0 */
#define RUN_BOARD_TESTS 1              /* 1 = corre test_qspi/wcet/seu no 1o boot */

#define USE_REAL_HW 1

#endif