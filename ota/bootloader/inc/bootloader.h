/**
 * @file bootloader.h
 * @brief Bootloader OTA para ATSAMV71Q21B — API principal.
 *
 * Mapa de memória:
 *
 *  Flash interna (2 MB @ 0x00400000):
 *  +------------------+ 0x00400000  <- BOOTLOADER (64 KB)
 *  |   Bootloader     |
 *  +------------------+ 0x00410000  <- APLICAÇÃO (~1984 KB)
 *  |   Aplicação      |
 *  |   (firmware)     |
 *  +------------------+ 0x005FFFFF
 *
 *  Flash externa W25Q128 (16 MB):
 *  +------------------+ 0x000000
 *  |  Reservado/Conf. |  (64 KB)
 *  +------------------+ 0x010000
 *  |  Telemetria      |  (~960 KB)
 *  +------------------+ 0x100000  <- METADADOS OTA (256 B)
 *  |  Imagem OTA      |  (~1 MB)
 *  +------------------+ 0x1FFFFF
 *
 * Fluxo de arranque:
 *  1. Bootloader inicializa QSPI.
 *  2. Lê metadados em 0x100000.
 *  3. Se magic == OTA_MAGIC_PENDING:
 *       a. Verifica CRC32 do firmware na flash externa.
 *       b. Apaga região da aplicação na flash interna.
 *       c. Copia firmware para 0x00410000.
 *       d. Apaga metadados OTA (limpa flag).
 *       e. Arranca a nova aplicação.
 *  4. Caso contrário, arranca directamente a aplicação existente.
 */

#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <stdint.h>
#include <stdbool.h>

/* =========================================================================
 * Endereços — Flash interna
 * ========================================================================= */
#define BOOT_START_ADDR          0x00400000UL   /**< Início do bootloader        */
#define BOOT_SIZE                0x00010000UL   /**< Tamanho: 64 KB              */
#define APP_START_ADDR           0x00410000UL   /**< Início da aplicação         */
#define APP_MAX_SIZE             0x001F0000UL   /**< Máximo ~1984 KB             */

/* =========================================================================
 * Endereços — Flash externa (W25Q128)
 * ========================================================================= */
#define OTA_EXT_METADATA_ADDR    0x100000UL     /**< Sector de metadados OTA     */
#define OTA_EXT_FIRMWARE_ADDR    0x101000UL     /**< Firmware OTA (sector seguinte aos metadados) */
#define OTA_EXT_METADATA_SECTOR  0x100000UL     /**< Sector 4 KB exclusivo dos metadados */

/* =========================================================================
 * Valores mágicos
 * ========================================================================= */
#define OTA_MAGIC_PENDING        0xAB12CD34UL   /**< Imagem válida, por aplicar  */
#define OTA_MAGIC_EMPTY          0xFFFFFFFFUL   /**< Flash apagada (sem OTA)     */

/* =========================================================================
 * Estrutura de metadados OTA (256 bytes, gravada em OTA_EXT_METADATA_ADDR)
 * ========================================================================= */
typedef struct {
    uint32_t magic;           /**< OTA_MAGIC_PENDING se imagem válida pendente  */
    uint32_t firmware_size;   /**< Tamanho do firmware em bytes                 */
    uint32_t firmware_crc32;  /**< CRC32 do firmware (para verificação)         */
    uint32_t fw_version;      /**< Número de versão do firmware                 */
    uint8_t  reserved[240];   /**< Reservado — padding até 256 bytes            */
} __attribute__((packed)) ota_metadata_t;

/* Garante que a estrutura tem exactamente 256 bytes */
typedef char ota_metadata_size_check[
    (sizeof(ota_metadata_t) == 256U) ? 1 : -1];

/* =========================================================================
 * Códigos de resultado
 * ========================================================================= */
typedef enum {
    BOOT_OK            = 0,  /**< Sem erros                                    */
    BOOT_NO_OTA        = 1,  /**< Nenhuma actualização OTA pendente            */
    BOOT_OTA_APPLIED   = 2,  /**< Actualização OTA aplicada com sucesso        */
    BOOT_OTA_CRC_FAIL  = 3,  /**< Falha na verificação CRC32                   */
    BOOT_OTA_FLASH_ERR = 4,  /**< Erro ao escrever/apagar flash interna        */
    BOOT_OTA_SIZE_ERR  = 5,  /**< Firmware demasiado grande                    */
    BOOT_OTA_READ_ERR  = 6,  /**< Erro ao ler flash externa                    */
} boot_result_t;

/* =========================================================================
 * API pública
 * ========================================================================= */

/**
 * @brief Executa a lógica principal do bootloader.
 *
 * Verifica se existe uma actualização OTA pendente, aplica-a se necessário,
 * e arranca a aplicação.
 *
 * @return Código de resultado (normalmente não retorna — salta para a app).
 */
boot_result_t bootloader_run(void);

/**
 * @brief Salta para a aplicação em APP_START_ADDR.
 *
 * Configura VTOR, carrega SP e salta para o Reset_Handler da aplicação.
 * Esta função NÃO retorna.
 */
void bootloader_jump_to_app(void);

#endif /* BOOTLOADER_H */
