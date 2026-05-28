#include "ext_memory.h"
#include "hal/hal_qspi.h"

/* ==========================================================================
 * Variáveis Privadas
 * ========================================================================== */
static qspi_handle_t qspi_handle;
static ext_mem_status_t current_status = EXT_MEM_IDLE;

/* Ponteiro para saber onde escrever a próxima telemetria */
static uint32_t telemetry_write_ptr = MEM_REGION_LOGS_START;

/* ==========================================================================
 * Funções Privadas (Callbacks)
 * ========================================================================== */

/**
 * @brief Callback que o qspi_driver chama quando termina uma operação.
 * @param result 1 para sucesso, 0 para erro.
 */
static void qspi_operation_callback(int result) {
    if (result == 1) {
        current_status = EXT_MEM_IDLE; // Operação concluída com sucesso
    } else {
        current_status = EXT_MEM_ERROR; // Ocorreu um erro (timeout, etc)
    }
}

/* ==========================================================================
 * Implementação da API
 * ========================================================================== */

void ExtMem_Init(void) {
    // 1. Inicializa o Hardware
    hal_qspi_init();
    
    // 2. Inicializa a estrutura do driver
    qspi_handle.state = QSPI_IDLE;
    qspi_handle.operation = QSPI_OP_NONE;
    
    current_status = EXT_MEM_IDLE;
}

void ExtMem_Tick(void) {
    // Esta função "bombeia" a máquina de estados do teu driver.
    // Deve ser chamada constantemente no teu "Nominal_mode" ou "Safe_mode"
    qspi_tick(&qspi_handle);
}

ext_mem_status_t ExtMem_GetStatus(void) {
    return current_status;
}

void ExtMem_SaveTelemetryAsync(uint8_t *data, uint32_t len) {
    // Proteção: não inicia nova operação se já estiver ocupado
    if (current_status != EXT_MEM_IDLE) {
        return; 
    }
    
    // Proteção de tamanho (Flash Pages têm máximo 256 bytes)
    if (len > 256U) {
        current_status = EXT_MEM_ERROR;
        return;
    }

    current_status = EXT_MEM_BUSY;
    
    // Inicia a escrita na FSM do driver
    qspi_write_async(&qspi_handle, 
                     telemetry_write_ptr, 
                     data, 
                     len, 
                     qspi_operation_callback);
    
    // Atualiza o ponteiro para a próxima vez 
    // (Nota: Numa versão final, terás de verificar se não passaste o limite da região de logs!)
    telemetry_write_ptr += len; 
}

void ExtMem_EraseSectorAsync(uint32_t sector_address) {
    if (current_status != EXT_MEM_IDLE) {
        return;
    }

    current_status = EXT_MEM_BUSY;

    qspi_erase_sector_async(&qspi_handle,
                            sector_address,
                            qspi_operation_callback);
}

void ExtMem_OtaWriteAsync(uint32_t addr, uint8_t *data, uint32_t len) {
    if (current_status != EXT_MEM_IDLE) {
        return;
    }

    if (len == 0U || len > 256U) {
        current_status = EXT_MEM_ERROR;
        return;
    }

    current_status = EXT_MEM_BUSY;

    qspi_write_async(&qspi_handle,
                     addr,
                     data,
                     len,
                     qspi_operation_callback);
}