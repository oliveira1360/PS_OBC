/**
 * @file flash_efc.h
 * @brief Driver bloqueante para a flash interna via EFC (ATSAMV71Q21B).
 *
 * Operações suportadas:
 *  - Desbloquear regiões de flash (CLB — Clear Lock Bit)
 *  - Apagar grupos de 16 páginas (EPA — Erase Pages, 8 KB por chamada)
 *  - Escrever uma página (WP — Write Page, 512 bytes)
 *  - Escrever uma região completa de firmware
 *
 * NOTA: O bootloader corre sempre da sua própria região (0x00400000–0x0040FFFF).
 * As operações de erase/write incidem apenas sobre a região da aplicação
 * (0x00410000+), pelo que não há risco de apagar o código em execução.
 */

#ifndef FLASH_EFC_H
#define FLASH_EFC_H

#include <stdint.h>
#include <stdbool.h>

/* =========================================================================
 * Constantes da flash interna — ATSAMV71Q21B
 * ========================================================================= */
#define FLASH_BASE_ADDR      0x00400000UL   /**< Início da flash interna        */
#define FLASH_PAGE_SIZE      512U           /**< Bytes por página               */
#define FLASH_EPA_PAGES      16U            /**< Páginas por operação EPA       */
#define FLASH_SECTOR_SIZE    (FLASH_PAGE_SIZE * FLASH_EPA_PAGES)  /* 8 KB */
#define FLASH_TOTAL_SIZE     (2UL * 1024UL * 1024UL)  /* 2 MB */
#define FLASH_TOTAL_PAGES    (FLASH_TOTAL_SIZE / FLASH_PAGE_SIZE) /* 4096 */

/* =========================================================================
 * EFC — Embedded Flash Controller (base 0x400E0C00)
 * ========================================================================= */
#define EFC_BASE             0x400E0C00UL
#define EFC_FMR  (*(volatile uint32_t *)(EFC_BASE + 0x00U)) /**< Mode Register   */
#define EFC_FCR  (*(volatile uint32_t *)(EFC_BASE + 0x04U)) /**< Command Register*/
#define EFC_FSR  (*(volatile uint32_t *)(EFC_BASE + 0x08U)) /**< Status Register */
#define EFC_FRR  (*(volatile uint32_t *)(EFC_BASE + 0x0CU)) /**< Result Register */

/* EFC_FSR bits */
#define EFC_FSR_FRDY         (1UL << 0)  /**< Flash ready (operação concluída) */
#define EFC_FSR_FCMDE        (1UL << 1)  /**< Erro de comando                  */
#define EFC_FSR_FLOCKE       (1UL << 2)  /**< Erro de lock                     */
#define EFC_FSR_FLERR        (1UL << 3)  /**< Erro geral de flash              */
#define EFC_FSR_ERRORS       (EFC_FSR_FCMDE | EFC_FSR_FLOCKE | EFC_FSR_FLERR)

/* EFC_FCR: chave obrigatória + FARG + FCMD */
#define EFC_FCR_KEY          (0x5AUL << 24)
#define EFC_CMD_WP           0x01U  /**< Write Page                            */
#define EFC_CMD_WPL          0x02U  /**< Write Page and Lock                   */
#define EFC_CMD_EWP          0x03U  /**< Erase and Write Page                  */
#define EFC_CMD_EPA          0x07U  /**< Erase Pages (8/16/32 páginas)         */
#define EFC_CMD_SLB          0x08U  /**< Set Lock Bit                          */
#define EFC_CMD_CLB          0x09U  /**< Clear Lock Bit                        */

/* EPA type field (bits [1:0] de FARG) */
#define EFC_EPA_TYPE_8       0x00U  /**<  8 páginas (4 KB)                     */
#define EFC_EPA_TYPE_16      0x01U  /**< 16 páginas (8 KB) — usado aqui        */
#define EFC_EPA_TYPE_32      0x02U  /**< 32 páginas (16 KB)                    */

/* Timeout para wait_ready (iterações de polling) */
#define EFC_TIMEOUT_LOOPS    0x00100000UL

/* =========================================================================
 * Códigos de resultado
 * ========================================================================= */
typedef enum {
    FLASH_EFC_OK        = 0,
    FLASH_EFC_ERROR     = 1,  /**< Erro genérico (FCMDE ou FLERR)             */
    FLASH_EFC_LOCKED    = 2,  /**< Região bloqueada (FLOCKE)                  */
    FLASH_EFC_ALIGN_ERR = 3,  /**< Endereço não alinhado                      */
    FLASH_EFC_RANGE_ERR = 4,  /**< Endereço fora da flash                     */
    FLASH_EFC_TIMEOUT   = 5,  /**< Timeout a aguardar FRDY                    */
} flash_efc_result_t;

/* =========================================================================
 * Protótipos
 * ========================================================================= */

/**
 * @brief Aguarda que o EFC fique pronto (FRDY) e verifica erros.
 */
flash_efc_result_t flash_efc_wait_ready(void);

/**
 * @brief Desbloqueia uma região de flash (limpa lock bits).
 *
 * @param addr  Endereço inicial da região.
 * @param size  Tamanho em bytes.
 */
flash_efc_result_t flash_efc_unlock(uint32_t addr, uint32_t size);

/**
 * @brief Apaga 8 KB de flash a partir de addr (16 páginas — EPA type 16).
 *
 * @param addr  Deve estar alinhado a FLASH_SECTOR_SIZE (8 KB).
 */
flash_efc_result_t flash_efc_erase_sector(uint32_t addr);

/**
 * @brief Apaga uma região de tamanho arbitrário (arredondado ao sector).
 *
 * @param start  Endereço inicial (alinhado a sector).
 * @param size   Número de bytes a apagar.
 */
flash_efc_result_t flash_efc_erase_region(uint32_t start, uint32_t size);

/**
 * @brief Escreve uma página de 512 bytes na flash interna.
 *
 * @param addr  Endereço destino — deve estar alinhado a FLASH_PAGE_SIZE.
 * @param data  Ponteiro para 512 bytes de dados.
 */
flash_efc_result_t flash_efc_write_page(uint32_t addr, const uint8_t *data);

/**
 * @brief Apaga e escreve um firmware completo a partir de dest_addr.
 *
 * Trata do alinhamento, erase sector a sector, e write página a página.
 *
 * @param dest_addr  Destino na flash interna (alinhado à página).
 * @param src        Ponteiro para os dados do firmware.
 * @param size       Número de bytes a escrever.
 */
flash_efc_result_t flash_efc_write_firmware(uint32_t dest_addr,
                                             const uint8_t *src,
                                             uint32_t size);

#endif /* FLASH_EFC_H */
