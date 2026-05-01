#ifndef HAL_QSPI_H
#define HAL_QSPI_H
#include <stdint.h>

#define QSPI_CLK_DIV2 0x01U // clock do AHB / 2  → velocidade máxima
#define QSPI_CLK_DIV4 0x03U // clock do AHB / 4  → mais estável

#define QSPI_FLASH_16MB 0x17U // tamanho do flash = 2^(23+1) = 16MB

#define QSPI_CS_HIGH_2 0x01U // NCS fica HIGH mínimo 2 ciclos entre transacções

#define QSPI_MODE_0 0x00U // clock idle LOW  — flash W25Q usa este modo
#define QSPI_MODE_3 0x01U // clock idle HIGH

#define QSPI_FIFO_THR_4 0x03U // interrupção/DMA gerado com 4 bytes no FIFO

uint8_t hal_qspi_init(void);

#endif