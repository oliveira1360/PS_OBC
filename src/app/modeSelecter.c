#include <stdio.h>
#include <string.h>
#include "app/states.h"
#include "app/modes.h"
#include "app/mission.h"
#include "app/sensors.h"
#include "hal/hal_systick.h"
#include "hal/hal_system.h"
#include "peripherals/ttc.h"
#include "peripherals/ext_memory.h"
#include "config/board.h"

/*
#include <time.h> // remover no futuro, uso para o clock(tempo)

States nominalMode(void)
{
    static clock_t last_time = 0;

    if (last_time == 0) {
        last_time = clock();
    }

    clock_t now = clock();

    if (((now - last_time) * 1000) / CLOCKS_PER_SEC > TIME_TO_UPDATE_VALUES)
    {
        sensors_print();
        sensors_read_all();
        last_time = clock();
    }

    return getMode(nominal_mode);
}*/

States nominalMode(void)
{
    static uint32_t last_ms = 0U;

    uint32_t now = hal_systick_get_ms();

    if ((now - last_ms) >= TIME_TO_UPDATE_VALUES)
    {
        sensors_print();
        sensors_read_all();
        last_ms = now;
    }

    return getMode(nominal_mode);
}

States communicationMode(void)
{
    return getMode(communication_mode);
}

/* Endereços OTA na flash externa (devem coincidir com o bootloader) */
#define OTA_EXT_META_ADDR MEM_REGION_OTA_START           /* 0x100000 */
#define OTA_EXT_FW_ADDR (MEM_REGION_OTA_START + 0x1000U) /* 0x101000 */
#define OTA_SECTOR_SZ 4096U                              /* 4 KB */
#define OTA_PKT_PAYLOAD_SZ OTA_PACKET_SIZE               /* 128 B */
#define OTA_MAGIC_PENDING 0xAB12CD34UL

/* Estados da FSM interna do otaMode() */
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

/* Metadados OTA (256 bytes — estrutura idêntica ao bootloader) */
typedef struct
{
    uint32_t magic;
    uint32_t firmware_size;
    uint32_t firmware_crc32;
    uint32_t fw_version;
    uint8_t reserved[240];
} __attribute__((packed)) ota_meta_t;

States otaMode(void)
{
    static ota_sm_t s_state = OTA_SM_IDLE;
    static uint32_t s_erase_addr = 0U;              /**< Sector a apagar actualmente   */
    static uint32_t s_next_fw_sector = 0U;          /**< Próximo sector de fw a apagar */
    static uint32_t s_bytes_written = 0U;           /**< Total de bytes de fw gravados  */
    static ota_meta_t s_meta;                       /**< Metadados a gravar no fim      */
    static uint8_t s_write_buf[OTA_PKT_PAYLOAD_SZ]; /**< Cópia local do payload */
    static uint16_t s_pending_seq = 0U;             /**< seq do pacote em escrita       */
    static uint16_t s_pending_len = 0U;             /**< len do pacote em escrita       */

    switch (s_state)
    {
    /* ------------------------------------------------------------------
     * IDLE — aguarda que o TTC active o flag OTA_REQUESTED
     * ------------------------------------------------------------------ */
    case OTA_SM_IDLE:
        if (OTA_REQUESTED)
        {
            printf("[OTA] Inicio do modo OTA\n");
            s_bytes_written = 0U;
            s_next_fw_sector = OTA_EXT_FW_ADDR; /* Primeiro sector de firmware */
            memset(&s_meta, 0xFF, sizeof(s_meta));

            /* Primeiro: apagar sector de metadados (limpeza preventiva) */
            s_erase_addr = OTA_EXT_META_ADDR;
            ExtMem_EraseSectorAsync(s_erase_addr);
            s_state = OTA_SM_WAIT_ERASE;
            /* Estado de retorno após este erase */
            /* (usamos s_next_fw_sector como marcador de fase) */
        }
        break;

    /* ------------------------------------------------------------------
     * WAIT_ERASE — aguarda que o ExtMem termine o erase atual
     * ------------------------------------------------------------------ */
    case OTA_SM_WAIT_ERASE:
        if (ExtMem_GetStatus() == EXT_MEM_IDLE)
        {
            /* Verifica de onde veio este erase */
            if (s_erase_addr == OTA_EXT_META_ADDR &&
                s_bytes_written == 0U)
            {
                /* Erase inicial do sector de metadados concluído
                 * → apagar primeiro sector de firmware */
                s_erase_addr = OTA_EXT_FW_ADDR;
                ExtMem_EraseSectorAsync(s_erase_addr);
                s_next_fw_sector = OTA_EXT_FW_ADDR + OTA_SECTOR_SZ;
                /* Permanece em WAIT_ERASE mas agora para o fw sector */
            }
            else if (s_erase_addr >= OTA_EXT_FW_ADDR)
            {
                /* Erase de sector de firmware concluído → pode receber pacotes */
                s_state = OTA_SM_WAIT_PKT;
            }
        }
        else if (ExtMem_GetStatus() == EXT_MEM_ERROR)
        {
            printf("[OTA] ERRO no erase (addr=0x%lX)\n",
                   (unsigned long)s_erase_addr);
            s_state = OTA_SM_ERROR;
        }
        break;

    /* ------------------------------------------------------------------
     * WAIT_PKT — aguarda pacote OTA do TTC
     * ------------------------------------------------------------------ */
    case OTA_SM_WAIT_PKT:
        if (!ttc_ota_packet_ready())
        {
            break; /* Ainda não chegou pacote */
        }

        s_pending_seq = ttc_ota_get_seq();
        s_pending_len = ttc_ota_get_payload_len();

        if (s_pending_seq == 0xFFFFU)
        {
            /* Marcador de fim — payload: [4B size][4B crc][4B version] */
            const uint8_t *ep = ttc_ota_get_payload();

            s_meta.firmware_size = ((uint32_t)ep[0] << 24) |
                                   ((uint32_t)ep[1] << 16) |
                                   ((uint32_t)ep[2] << 8) |
                                   (uint32_t)ep[3];
            s_meta.firmware_crc32 = ((uint32_t)ep[4] << 24) |
                                    ((uint32_t)ep[5] << 16) |
                                    ((uint32_t)ep[6] << 8) |
                                    (uint32_t)ep[7];
            s_meta.fw_version = ((uint32_t)ep[8] << 24) |
                                ((uint32_t)ep[9] << 16) |
                                ((uint32_t)ep[10] << 8) |
                                (uint32_t)ep[11];
            ttc_ota_clear_ready();

            printf("[OTA] Fim da transferencia: %lu bytes, CRC=0x%08lX, v%lu\n",
                   (unsigned long)s_meta.firmware_size,
                   (unsigned long)s_meta.firmware_crc32,
                   (unsigned long)s_meta.fw_version);

            /* Apaga sector de metadados antes de escrever a flag */
            s_erase_addr = OTA_EXT_META_ADDR;
            ExtMem_EraseSectorAsync(s_erase_addr);
            s_state = OTA_SM_WAIT_ERASE_META;
            break;
        }

        /* Pacote de dados: verifica se precisa de apagar o próximo sector */
        {
            uint32_t pkt_addr = OTA_EXT_FW_ADDR +
                                (uint32_t)s_pending_seq * OTA_PKT_PAYLOAD_SZ;

            if (pkt_addr >= s_next_fw_sector)
            {
                /* Entrou num sector ainda não apagado — apaga antes */
                s_erase_addr = s_next_fw_sector;
                s_next_fw_sector += OTA_SECTOR_SZ;
                ExtMem_EraseSectorAsync(s_erase_addr);
                s_state = OTA_SM_NEED_ERASE;
                /* Não limpa o ready — vai escrever depois do erase */
            }
            else
            {
                /* Sector já apagado — pode escrever directamente */
                memcpy(s_write_buf, ttc_ota_get_payload(), s_pending_len);
                ttc_ota_clear_ready();
                s_state = OTA_SM_WRITE_PKT;
            }
        }
        break;

    /* ------------------------------------------------------------------
     * NEED_ERASE — aguarda erase do sector antes de escrever o pacote
     * ------------------------------------------------------------------ */
    case OTA_SM_NEED_ERASE:
        if (ExtMem_GetStatus() == EXT_MEM_IDLE)
        {
            memcpy(s_write_buf, ttc_ota_get_payload(), s_pending_len);
            ttc_ota_clear_ready();
            s_state = OTA_SM_WRITE_PKT;
        }
        else if (ExtMem_GetStatus() == EXT_MEM_ERROR)
        {
            printf("[OTA] ERRO no erase do sector de firmware\n");
            s_state = OTA_SM_ERROR;
        }
        break;

    /* ------------------------------------------------------------------
     * WRITE_PKT — inicia escrita do payload na flash externa
     * ------------------------------------------------------------------ */
    case OTA_SM_WRITE_PKT:
    {
        uint32_t pkt_addr = OTA_EXT_FW_ADDR +
                            (uint32_t)s_pending_seq * OTA_PKT_PAYLOAD_SZ;
        ExtMem_OtaWriteAsync(pkt_addr, s_write_buf, s_pending_len);
        s_state = OTA_SM_WAIT_WRITE;
        break;
    }

    /* ------------------------------------------------------------------
     * WAIT_WRITE — aguarda fim da escrita do pacote
     * ------------------------------------------------------------------ */
    case OTA_SM_WAIT_WRITE:
        if (ExtMem_GetStatus() == EXT_MEM_IDLE)
        {
            s_bytes_written += s_pending_len;
            s_state = OTA_SM_WAIT_PKT;
        }
        else if (ExtMem_GetStatus() == EXT_MEM_ERROR)
        {
            printf("[OTA] ERRO na escrita do pacote seq=%u\n", s_pending_seq);
            s_state = OTA_SM_ERROR;
        }
        break;

    /* ------------------------------------------------------------------
     * WAIT_ERASE_META — aguarda erase do sector de metadados
     * ------------------------------------------------------------------ */
    case OTA_SM_WAIT_ERASE_META:
        if (ExtMem_GetStatus() == EXT_MEM_IDLE)
        {
            /* Prepara metadados com magic válido */
            s_meta.magic = OTA_MAGIC_PENDING;
            memset(s_meta.reserved, 0xFF, sizeof(s_meta.reserved));
            s_state = OTA_SM_WRITE_META;
        }
        else if (ExtMem_GetStatus() == EXT_MEM_ERROR)
        {
            printf("[OTA] ERRO no erase dos metadados\n");
            s_state = OTA_SM_ERROR;
        }
        break;

    /* ------------------------------------------------------------------
     * WRITE_META — escreve ota_metadata_t na flash externa
     * ------------------------------------------------------------------ */
    case OTA_SM_WRITE_META:
        /* Escreve os primeiros 256 bytes (sizeof(ota_meta_t) == 256) */
        ExtMem_OtaWriteAsync(OTA_EXT_META_ADDR,
                             (uint8_t *)&s_meta,
                             sizeof(s_meta));
        s_state = OTA_SM_WAIT_META;
        break;

    /* ------------------------------------------------------------------
     * WAIT_META — aguarda fim da escrita dos metadados
     * ------------------------------------------------------------------ */
    case OTA_SM_WAIT_META:
        if (ExtMem_GetStatus() == EXT_MEM_IDLE)
        {
            printf("[OTA] Metadados gravados. A reiniciar...\n");
            s_state = OTA_SM_RESET;
        }
        else if (ExtMem_GetStatus() == EXT_MEM_ERROR)
        {
            printf("[OTA] ERRO na escrita dos metadados\n");
            s_state = OTA_SM_ERROR;
        }
        break;

    /* ------------------------------------------------------------------
     * RESET — reinicia para que o bootloader aplique o firmware
     * ------------------------------------------------------------------ */
    case OTA_SM_RESET:
        OTA_REQUESTED = 0;
        hal_system_reset();
        break; /* Nunca chega aqui */

    /* ------------------------------------------------------------------
     * ERROR — fica em loop; aguarda intervenção externa (ex: safe_mode)
     * ------------------------------------------------------------------ */
    case OTA_SM_ERROR:
        OTA_REQUESTED = 0;
        s_state = OTA_SM_IDLE;
        printf("[OTA] Abortado por erro. Voltando ao modo comunicacao.\n");
        break;

    default:
        s_state = OTA_SM_IDLE;
        break;
    }

    return getMode(ota_mode);
}

States safeMode(void)
{
    static uint32_t last_ms = 0U;

    uint32_t now = hal_systick_get_ms();

    if ((now - last_ms) >= TIME_TO_UPDATE_VALUES)
    {
        sensors_print();
        sensors_read_all();
        last_ms = now;
    }

    return getMode(safe_mode);
}

States ultraLowPowerMode(void)
{
    return getMode(ultra_low_power_mode);
}

States decommissioningMode(void)
{
    return getMode(decommissioning_mode);
}

/**
 * @brief verify the system health
 *
 * @return 0 if the systema is not in safe mode, returns 1 if everything is right in the system
 */

int isSystemSafe(void)
{
    if (eps.voltage > MAX_SAFE_VOLTAGE || eps.voltage < LOWEST_SAFE_VOLTAGE)
        return 0;
    if (eps.current > MAX_SAFE_CURRENT || eps.current < LOWEST_SAFE_CURRENT)
        return 0;
    if (temperature.temperature > MAX_SAFE_TEMP || temperature.temperature < LOWEST_SAFE_TEMP)
        return 0;
    if (BATTERY_STATUS < LOWEST_SAFE_BATTERY)
        return 0;

    return 1;
}

States getMode(States currentState)
{
    int safe = isSystemSafe();
    int battery_critical = (BATTERY_STATUS < BATTERY_IN_CRITICAL_LEVEL);

    switch (currentState)
    {
    case nominal_mode:
        if (!safe)
            return safe_mode;
        if (COMM_WINDOW_OPEN)
            return communication_mode;
        return nominal_mode;

    case communication_mode:
        if (!safe)
            return safe_mode;
        if (OTA_REQUESTED)
            return ota_mode;
        if (!COMM_WINDOW_OPEN)
            return nominal_mode;
        return communication_mode;

    case ota_mode:
        if (!safe)
            return safe_mode;
        if (!OTA_REQUESTED)
            return communication_mode;
        return ota_mode;

    case safe_mode:
        if (battery_critical)
            return ultra_low_power_mode;
        if (safe)
            return nominal_mode;
        return safe_mode;

    case ultra_low_power_mode:
        if (!battery_critical)
            return safe_mode;
        if (MISSION_TIMEOUT)
            return decommissioning_mode;
        return ultra_low_power_mode;

    case decommissioning_mode:
        return decommissioning_mode;

    default:
        return nominal_mode;
    }
}

void mission_lifecycle()
{
    static States state = nominal_mode;

    switch (state)
    {
    case nominal_mode:
        state = nominalMode();
        break;
    case communication_mode:
        state = communicationMode();
        break;
    case ota_mode:
        state = otaMode();
        break;
    case safe_mode:
        state = safeMode();
        break;
    case ultra_low_power_mode:
        state = ultraLowPowerMode();
        break;
    case decommissioning_mode:
        state = decommissioningMode();
        break;
    }
}