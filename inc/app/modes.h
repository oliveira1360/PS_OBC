#ifndef MODE_H
#define MODE_H
#include <stdint.h>
#include "peripherals/ext_memory.h"
#include "config/board.h"

typedef enum
{
    nominal_mode,
    communication_mode,
    ota_mode,
    safe_mode,
    ultra_low_power_mode,
    decommissioning_mode,
} States;

typedef enum
{
    OTA_SM_IDLE = 0,         /**< Aguarda OTA_REQUESTED                         */
    OTA_SM_ERASE_META,       /**< Apaga sector de metadados (limpeza inicial)   */
    OTA_SM_WAIT_ERASE,       /**< Aguarda fim do erase atual                    */
    OTA_SM_WAIT_PKT,         /**< Aguarda próximo pacote OTA do TTC             */
    OTA_SM_NEED_ERASE,       /**< Precisa de apagar sector antes de escrever    */
    OTA_SM_WRITE_PKT,        /**< Inicia escrita do payload na flash ext        */
    OTA_SM_WAIT_WRITE,       /**< Aguarda fim da escrita do pacote              */
    OTA_SM_ERASE_META_FINAL, /**< Apaga sector de metadados para escrever flag */
    OTA_SM_WAIT_ERASE_META,  /**< Aguarda fim do erase dos metadados          */
    OTA_SM_WRITE_META,       /**< Escreve ota_metadata_t com magic válido       */
    OTA_SM_WAIT_META,        /**< Aguarda fim da escrita dos metadados          */
    OTA_SM_RESET,            /**< Reinicia o sistema                            */
    OTA_SM_ERROR,            /**< Erro — aguarda intervenção                    */
} ota_sm_t;

/* Endereços OTA na flash externa (devem coincidir com o bootloader) */
#define OTA_EXT_META_ADDR MEM_REGION_OTA_START           /* 0x100000 */
#define OTA_EXT_FW_ADDR (MEM_REGION_OTA_START + 0x1000U) /* 0x101000 */
#define OTA_SECTOR_SZ 4096U                              /* 4 KB */
#define OTA_PKT_PAYLOAD_SZ OTA_PACKET_SIZE               /* 128 B */
#define OTA_MAGIC_PENDING 0xAB12CD34UL

/* Metadados OTA (256 bytes — estrutura idêntica ao bootloader) */
typedef struct
{
    uint32_t magic;
    uint32_t firmware_size;
    uint32_t firmware_crc32;
    uint32_t fw_version;
    uint8_t reserved[240];
} __attribute__((packed)) ota_meta_t;

extern States state;

States nominalMode(void);
States communicationMode(void);
States otaMode(void);
States safeMode(void);
States ultraLowPowerMode(void);
States decommissioningMode(void);

States getMode(States currentState);
int isSystemSafe(void);

#endif