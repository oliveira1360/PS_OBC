#ifndef EXT_MEMORY_H
#define EXT_MEMORY_H

#include <stdint.h>
#include "drivers/qspi_driver.h" // O teu driver FSM

/* ==========================================================================
 * Mapa de Memória (Memory Map) para 2MB
 * ========================================================================== */
#define MEM_REGION_CONFIG_START  0x000000UL  /* 64KB para configurações boot */
#define MEM_REGION_LOGS_START    0x010000UL  /* ~1MB para logs de telemetria */
#define MEM_REGION_OTA_START     0x100000UL  /* ~1MB para imagens OTA */

/* ==========================================================================
 * Tipos e Estados da Camada de Aplicação
 * ========================================================================== */
typedef enum {
    EXT_MEM_IDLE,
    EXT_MEM_BUSY,
    EXT_MEM_ERROR
} ext_mem_status_t;

/* ==========================================================================
 * Protótipos da API de Aplicação
 * ========================================================================== */

/**
 * @brief Inicializa a memória, o HAL e o Driver QSPI.
 */
void ExtMem_Init(void);

/**
 * @brief Função de Tick que deve ser chamada no ciclo principal (super-loop).
 */
void ExtMem_Tick(void);

/**
 * @brief Devolve o estado atual da memória externa.
 */
ext_mem_status_t ExtMem_GetStatus(void);

/**
 * @brief Guarda um bloco de telemetria (máx 256 bytes) de forma assíncrona.
 */
void ExtMem_SaveTelemetryAsync(uint8_t *data, uint32_t len);

/**
 * @brief Apaga um setor de 4KB (necessário antes de voltar a escrever).
 */
void ExtMem_EraseSectorAsync(uint32_t sector_address);

/**
 * @brief Escreve dados na região OTA da flash externa (endereço explícito).
 *
 * Igual a SaveTelemetryAsync mas para qualquer endereço (tipicamente
 * MEM_REGION_OTA_START + offset). Máximo 256 bytes por chamada.
 *
 * @param addr  Endereço destino na flash externa.
 * @param data  Buffer de dados.
 * @param len   Número de bytes (max 256).
 */
void ExtMem_OtaWriteAsync(uint32_t addr, uint8_t *data, uint32_t len);

#endif // EXT_MEMORY_H