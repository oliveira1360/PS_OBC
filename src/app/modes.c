#include <stdio.h>
#include "mission.h"
#include "app/modes.h"
#include "sensors.h"
#include "modes.h"
#include "hal/hal_qspi.h"

static uint32_t s_print_last_ms = 0U;

static ota_sm_t s_state = OTA_SM_IDLE;
static uint32_t s_erase_addr = 0U;              /**< Sector a apagar actualmente   */
static uint32_t s_next_fw_sector = 0U;          /**< Próximo sector de fw a apagar */
static uint32_t s_bytes_written = 0U;           /**< Total de bytes de fw gravados  */
static ota_meta_t s_meta;                       /**< Metadados a gravar no fim      */
static uint8_t s_write_buf[OTA_PKT_PAYLOAD_SZ]; /**< Cópia local do payload */
static uint16_t s_pending_seq = 0U;             /**< seq do pacote em escrita       */
static uint16_t s_pending_len = 0U;             /**< len do pacote em escrita       */

static void ota_handle_idle(void);
static void ota_handle_wait_erase(void);
static void ota_handle_wait_pkt(void);
static void ota_handle_need_erase(void);
static void ota_handle_write_pkt(void);
static void ota_handle_wait_write(void);
static void ota_handle_wait_erase_meta(void);
static void ota_handle_write_meta(void);
static void ota_handle_wait_meta(void);
static void ota_handle_reset(void);
static void ota_handle_error(void);
static uint32_t ota_crc32(uint32_t addr, uint32_t size);

States otaMode(void)
{
    sensors_tick();

    switch (s_state)
    {
    case OTA_SM_IDLE:
        ota_handle_idle();
        break;
    case OTA_SM_WAIT_ERASE:
        ota_handle_wait_erase();
        break;
    case OTA_SM_WAIT_PKT:
        ota_handle_wait_pkt();
        break;
    case OTA_SM_NEED_ERASE:
        ota_handle_need_erase();
        break;
    case OTA_SM_WRITE_PKT:
        ota_handle_write_pkt();
        break;
    case OTA_SM_WAIT_WRITE:
        ota_handle_wait_write();
        break;
    case OTA_SM_WAIT_ERASE_META:
        ota_handle_wait_erase_meta();
        break;
    case OTA_SM_WRITE_META:
        ota_handle_write_meta();
        break;
    case OTA_SM_WAIT_META:
        ota_handle_wait_meta();
        break;
    case OTA_SM_RESET:
        ota_handle_reset();
        break;
    case OTA_SM_ERROR:
        ota_handle_error();
        break;
    default:
        s_state = OTA_SM_IDLE;
        break;
    }

    return getMode(ota_mode);
}

States nominalMode(void)
{
    sensors_tick();
    uint32_t now = hal_systick_get_ms();

    if ((now - s_print_last_ms) >= TIME_TO_UPDATE_VALUES)
    {
        ttc_send_telemetry();
        sensors_print();
        sensors_read_all();
        s_print_last_ms = now;
    }

    return getMode(nominal_mode);
}

States communicationMode(void)
{
    sensors_tick(); 
    return getMode(communication_mode);
}

States safeMode(void)
{
    sensors_tick();
    uint32_t now = hal_systick_get_ms();

    if ((now - s_print_last_ms) >= TIME_TO_UPDATE_VALUES)
    {
        ttc_send_telemetry();
        sensors_print();
        sensors_read_all();
        s_print_last_ms = now;
    }

    return getMode(safe_mode);
}

States ultraLowPowerMode(void)
{
    sensors_tick(); /* sensors_tick() já chama ExtMem_Tick() internamente */
    return getMode(ultra_low_power_mode);
}

States decommissioningMode(void)
{
    return getMode(decommissioning_mode);
}

/* ================================================================== *
 * Funções de cada Estado da Máquina OTA
 * ================================================================== */

static void ota_handle_idle(void)
{
    if (OTA_REQUESTED)
    {
        /* GARANTIA: Espera que a ExtMem termine qualquer operação dos sensores */
        if (ExtMem_GetStatus() != EXT_MEM_IDLE)
        {
            return; /* Fica aqui a aguardar silenciosamente no próximo tick */
        }

        /* Limpa write-protection antes de qualquer erase.
         * O W25Q pode ter BP bits setados que fazem sector erases serem
         * ignorados silenciosamente (flash retorna IDLE mas dado não muda). */
        uint8_t sr1_before = hal_qspi_read_status();
        printf("[OTA] SR1=0x%02X — a limpar write-protection...\n", (unsigned)sr1_before);
        hal_qspi_clear_write_protection();
        uint8_t sr1_after = hal_qspi_read_status();
        printf("[OTA] SR1 apos WRSR=0x%02X — Inicio OTA\n", (unsigned)sr1_after);

        printf("[OTA] Inicio — a apagar sector de metadata\n");
        s_bytes_written = 0U;
        s_next_fw_sector = OTA_EXT_FW_ADDR;
        memset(&s_meta, 0xFF, sizeof(s_meta));

        s_erase_addr = OTA_EXT_META_ADDR;
        ExtMem_EraseSectorAsync(s_erase_addr);
        s_state = OTA_SM_WAIT_ERASE;
    }
}

static void ota_handle_wait_erase(void)
{
    static uint32_t s_erase_dbg = 0U;
    ext_mem_status_t st = ExtMem_GetStatus();

    /* Imprime na 1ª chamada e depois a cada ~5000 */
    if (s_erase_dbg == 0U || s_erase_dbg % 5000U == 0U)
    {
        printf("[OTA] WAIT_ERASE #%lu addr=0x%lX status=%d\n",
               (unsigned long)s_erase_dbg,
               (unsigned long)s_erase_addr, (int)st);
    }
    s_erase_dbg++;

    if (st == EXT_MEM_IDLE)
    {
        s_erase_dbg = 0U;
        if (s_erase_addr == OTA_EXT_META_ADDR && s_bytes_written == 0U)
        {
            printf("[OTA] Metadata apagada — a apagar sector fw 0x%lX\n",
                   (unsigned long)OTA_EXT_FW_ADDR);

            /* Dá tempo ao hardware da Flash para repor o Write Enable Latch. */
            for (volatile int delay = 0; delay < 100000; delay++)
            {
            }

            s_erase_addr = OTA_EXT_FW_ADDR;
            ExtMem_EraseSectorAsync(s_erase_addr);
            s_next_fw_sector = OTA_EXT_FW_ADDR + OTA_SECTOR_SZ;
        }
        else if (s_erase_addr >= OTA_EXT_FW_ADDR)
        {
            uint8_t erase_check;
            hal_qspi_read_memory(s_erase_addr, &erase_check, 1U);
            if (erase_check != 0xFFU)
            {
                /* Flash ainda não apagou — mantém WAIT_ERASE mais um tick */
                printf("[OTA] Erase nao confirmado addr=0x%lX val=0x%02X — a aguardar\n",
                       (unsigned long)s_erase_addr, erase_check);
            }
            else
            {
                printf("[OTA] Erase OK confirmado — a aguardar pacotes\n");
                ttc_send_ota_ready();
                s_state = OTA_SM_WAIT_PKT;
            }
        }
    }
    else if (st == EXT_MEM_ERROR)
    {
        printf("[OTA] ERRO no erase (addr=0x%lX)\n", (unsigned long)s_erase_addr);
        s_state = OTA_SM_ERROR;
    }
}

static void ota_handle_wait_pkt(void)
{
    static uint32_t s_pkt_dbg = 0U;

    if (++s_pkt_dbg >= 100000U)
    {
        s_pkt_dbg = 0U;
        printf("[OTA] WAIT_PKT — ready=%d bytes_written=%lu\n",
               (int)ttc_ota_packet_ready(),
               (unsigned long)s_bytes_written);
    }

    if (!ttc_ota_packet_ready())
    {
        return; /* Ainda não chegou pacote */
    }

    s_pkt_dbg = 0U;
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
        return;
    }

    if (s_pending_len == 0U)
    {
        printf("[OTA] Aviso: Pacote fantasma (len=0) ignorado.\n");
        ttc_ota_clear_ready();
        return; /* Ignora e volta a ficar à espera de um pacote real */
    }

    uint32_t pkt_addr = OTA_EXT_FW_ADDR + (uint32_t)s_pending_seq * OTA_PKT_PAYLOAD_SZ;

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

static void ota_handle_need_erase(void)
{
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
}

static void ota_handle_write_pkt(void)
{
    uint32_t pkt_addr = OTA_EXT_FW_ADDR + (uint32_t)s_pending_seq * OTA_PKT_PAYLOAD_SZ;
    ExtMem_OtaWriteAsync(pkt_addr, s_write_buf, s_pending_len);
    s_state = OTA_SM_WAIT_WRITE;
}

static void ota_handle_wait_write(void)
{
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
}

static void ota_handle_wait_erase_meta(void)
{
    if (ExtMem_GetStatus() == EXT_MEM_IDLE)
    {
        /* Valida firmware_size antes do loop CRC. */
        if (s_meta.firmware_size == 0U || s_meta.firmware_size > (0xF00000UL - OTA_EXT_FW_ADDR))
        {
            printf("[OTA] firmware_size invalido (%lu) — a abortar\n",
                   (unsigned long)s_meta.firmware_size);
            s_state = OTA_SM_ERROR;
            return;
        }

        uint8_t v[8];
        hal_qspi_read_memory(OTA_EXT_FW_ADDR, v, 8U);
        printf("[OTA] FW[0]: %02X %02X %02X %02X %02X %02X %02X %02X\n",
               v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7]);

        uint8_t v2[8];
        hal_qspi_read_memory(OTA_EXT_FW_ADDR + 128U, v2, 8U);
        printf("[OTA] FW[128]: %02X %02X %02X %02X %02X %02X %02X %02X\n",
               v2[0], v2[1], v2[2], v2[3], v2[4], v2[5], v2[6], v2[7]);

        /* Verifica CRC da firmware na flash externa antes de escrever metadados */
        uint32_t computed_crc = ota_crc32(OTA_EXT_FW_ADDR, s_meta.firmware_size);
        if (computed_crc != s_meta.firmware_crc32)
        {
            printf("[DEBUG] A comparar os primeiros 4 bytes do computed CRC: %02X %02X %02X %02X\n",
                   (unsigned int)((computed_crc >> 24) & 0xFF),
                   (unsigned int)((computed_crc >> 16) & 0xFF),
                   (unsigned int)((computed_crc >> 8) & 0xFF),
                   (unsigned int)(computed_crc & 0xFF));

            /* Lê primeiros 4 bytes de cada sector para diagnóstico */
            uint8_t dbg[4];
            uint32_t sector_addrs[] = {
                OTA_EXT_FW_ADDR,
                OTA_EXT_FW_ADDR + 0x1000U,
                OTA_EXT_FW_ADDR + 0x2000U,
                OTA_EXT_FW_ADDR + 0x3000U,
                OTA_EXT_FW_ADDR + 0x4000U};

            for (uint8_t s = 0U; s < 5U; s++)
            {
                hal_qspi_read_memory(sector_addrs[s], dbg, sizeof(dbg));
                printf("[OTA] Flash[0x%05lX]: %02X %02X %02X %02X\n",
                       (unsigned long)sector_addrs[s], dbg[0], dbg[1], dbg[2], dbg[3]);
            }
            s_state = OTA_SM_ERROR;
        }
        else
        {
            printf("[OTA] CRC OK (0x%08lX) — a escrever metadados\n",
                   (unsigned long)computed_crc);
            s_meta.magic = OTA_MAGIC_PENDING;
            memset(s_meta.reserved, 0xFF, sizeof(s_meta.reserved));
            s_state = OTA_SM_WRITE_META;
        }
    }
    else if (ExtMem_GetStatus() == EXT_MEM_ERROR)
    {
        printf("[OTA] ERRO no erase dos metadados\n");
        s_state = OTA_SM_ERROR;
    }
}

static void ota_handle_write_meta(void)
{
    ExtMem_OtaWriteAsync(OTA_EXT_META_ADDR, (uint8_t *)&s_meta, sizeof(s_meta));
    s_state = OTA_SM_WAIT_META;
}

static void ota_handle_wait_meta(void)
{
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
}

static void ota_handle_reset(void)
{
    OTA_REQUESTED = 0;
    hal_system_reset();
}

static void ota_handle_error(void)
{
    OTA_REQUESTED = 0;
    s_state = OTA_SM_IDLE;
    ttc_ota_abort(); /* Repõe TTC para aceitar novo CMD_START_OTA */
    printf("[OTA] Abortado por erro. Voltando ao modo comunicacao.\n");
}

static uint32_t ota_crc32(uint32_t addr, uint32_t size)
{
    uint32_t crc = 0xFFFFFFFFUL;
    uint8_t buf[256];

    uint32_t done = 0U;
    while (done < size)
    {
        uint32_t chunk = (size - done) > 256U ? 256U : (size - done);
        hal_qspi_read_memory(addr + done, buf, chunk);

        for (uint32_t i = 0U; i < chunk; i++)
        {
            crc ^= (uint32_t)buf[i];
            for (uint8_t bit = 0U; bit < 8U; bit++)
            {
                if (crc & 1UL)
                    crc = (crc >> 1U) ^ 0xEDB88320UL;
                else
                    crc = (crc >> 1U);
            }
        }
        done += chunk;
    }
    return crc ^ 0xFFFFFFFFUL;
}