/**
 * @file qspi_boot.h
 * @brief Driver QSPI bloqueante para o bootloader (ATSAMV71Q21B + W25Q128).
 *
 * Versão simplificada e bloqueante do driver QSPI do projecto principal.
 * Reutiliza os mesmos registos e definições do W25Q128, mas sem FSM
 * nem super-loop — as operações esperam no próprio ciclo até terminar.
 *
 * Operações disponíveis:
 *  - qspi_boot_init()         : Inicializa QSPI em Serial Memory Mode
 *  - qspi_boot_read()         : Lê N bytes da flash externa (bloqueante)
 *  - qspi_boot_wren()         : Envia Write Enable (WREN)
 *  - qspi_boot_erase_sector() : Apaga sector de 4 KB (bloqueante)
 *  - qspi_boot_write_page()   : Escreve até 256 bytes numa página (bloqueante)
 *  - qspi_boot_wait_busy()    : Aguarda que a flash externa fique livre
 */

#ifndef QSPI_BOOT_H
#define QSPI_BOOT_H

#include <stdint.h>
#include <stdbool.h>

/* =========================================================================
 * Endereços base — reutilizados do hal_qspi.h do projecto principal
 * ========================================================================= */
#define QSPI_BASE_ADDR       0x4007C000UL
#define QSPI_MEM_BASE_ADDR   0x80000000UL  /**< Região memory-mapped do QSPI  */

/* Periférico ID (PMC_PCER1 definido em system_samv71.h) */
#define ID_QSPI_PERIPH       43U

/* PIO A — pinos QSPI (Peripheral A) */
#define PIOA_BASE_ADDR       0x400E0E00UL
#define PIOA_PDR      (*(volatile uint32_t *)(PIOA_BASE_ADDR + 0x04U))
#define PIOA_ABCDSR0  (*(volatile uint32_t *)(PIOA_BASE_ADDR + 0x70U))
#define PIOA_ABCDSR1  (*(volatile uint32_t *)(PIOA_BASE_ADDR + 0x74U))

/* PA11=QCS, PA12=QIO1, PA13=QIO0, PA14=SCK, PA17=QIO2 */
#define QSPI_PIN_MASK  ((1UL<<11)|(1UL<<12)|(1UL<<13)|(1UL<<14)|(1UL<<17))

/* =========================================================================
 * Registos QSPI (offsets iguais ao hal_qspi.h do projecto principal)
 * ========================================================================= */
#define QSPI_REG(off)  (*(volatile uint32_t *)(QSPI_BASE_ADDR + (off)))
#define QSPI_CR        QSPI_REG(0x00U)  /**< Control Register                 */
#define QSPI_MR        QSPI_REG(0x04U)  /**< Mode Register                    */
#define QSPI_SR        QSPI_REG(0x10U)  /**< Status Register                  */
#define QSPI_SCR       QSPI_REG(0x20U)  /**< Serial Clock Register            */
#define QSPI_IAR       QSPI_REG(0x30U)  /**< Instruction Address Register     */
#define QSPI_ICR       QSPI_REG(0x34U)  /**< Instruction Code Register        */
#define QSPI_IFR       QSPI_REG(0x38U)  /**< Instruction Frame Register       */

/* QSPI_CR bits */
#define QSPI_CR_QSPIEN   (1UL << 0)
#define QSPI_CR_SWRST    (1UL << 7)
#define QSPI_CR_LASTXFER (1UL << 24)

/* QSPI_MR bits */
#define QSPI_MR_SMM              (1UL << 0)           /**< Serial Memory Mode  */
#define QSPI_MR_CSMODE_LASTXFER  (0x1UL << 4)
#define QSPI_MR_DLYCS(v)         (((v) & 0xFFU) << 24)

/* QSPI_SR bits */
#define QSPI_SR_INSTRE   (1UL << 10)  /**< Instruction End                    */
#define QSPI_SR_QSPIENS  (1UL << 24)  /**< QSPI Enable Status                 */

/* QSPI_SCR */
#define QSPI_SCR_SCBR(v) (((v) & 0xFFU) << 8)
#define QSPI_CLK_DIV     2U           /**< SCK = MCK / 3                      */

/* QSPI_IFR bits — coincidentes com hal_qspi.h */
#define QSPI_IFR_WIDTH_SINGLE   (0x0UL << 0)
#define QSPI_IFR_INSTEN         (1UL << 4)
#define QSPI_IFR_ADDREN         (1UL << 5)
#define QSPI_IFR_DATAEN         (1UL << 7)
#define QSPI_IFR_ADDRL_24       (0x0UL << 10)
#define QSPI_IFR_TFRTYP_READ    (0x0UL << 12)   /**< Leitura de registo       */
#define QSPI_IFR_TFRTYP_READMEM (0x1UL << 12)   /**< Leitura memory-mapped    */
#define QSPI_IFR_TFRTYP_WRITE   (0x2UL << 12)   /**< Escrita de comando       */
#define QSPI_IFR_TFRTYP_WRITEMEM (0x3UL << 12)  /**< Escrita memory-mapped    */
#define QSPI_IFR_NBDUM(v)       (((v) & 0x1FU) << 16)

/* =========================================================================
 * W25Q128 — comandos e constantes (igual ao hal_qspi.h)
 * ========================================================================= */
#define W25Q_CMD_WREN            0x06U
#define W25Q_CMD_READ_STATUS_1   0x05U
#define W25Q_CMD_FAST_READ       0x0BU
#define W25Q_CMD_PAGE_PROGRAM    0x02U
#define W25Q_CMD_SECTOR_ERASE    0x20U
#define W25Q_FAST_READ_DUMMY     8U
#define W25Q_PAGE_SIZE           256U
#define W25Q_SECTOR_SIZE         4096U
#define W25Q_SR1_BUSY            0x01U
#define W25Q_SR1_WEL             0x02U

/* Timeout de polling (iterações) */
#define QSPI_BOOT_TIMEOUT        0x00200000UL

/* =========================================================================
 * Códigos de resultado
 * ========================================================================= */
typedef enum {
    QSPI_BOOT_OK      = 0,
    QSPI_BOOT_TIMEOUT_ERR = 1,
    QSPI_BOOT_WEL_ERR = 2,
} qspi_boot_result_t;

/* =========================================================================
 * Protótipos
 * ========================================================================= */

/**
 * @brief Inicializa o QSPI em Serial Memory Mode para W25Q128.
 */
void qspi_boot_init(void);

/**
 * @brief Lê len bytes da flash externa a partir de addr.
 *
 * Usa Fast Read (0x0B) via memory-mapped access.
 *
 * @param addr  Endereço na flash externa (24-bit).
 * @param buf   Buffer destino.
 * @param len   Número de bytes a ler.
 */
qspi_boot_result_t qspi_boot_read(uint32_t addr, uint8_t *buf, uint32_t len);

/**
 * @brief Envia Write Enable (WREN) e verifica WEL bit.
 */
qspi_boot_result_t qspi_boot_wren(void);

/**
 * @brief Aguarda que a flash externa saia do estado busy (bloqueante).
 */
qspi_boot_result_t qspi_boot_wait_busy(void);

/**
 * @brief Apaga o sector de 4 KB que contém addr (bloqueante).
 *
 * @param addr  Qualquer endereço dentro do sector.
 */
qspi_boot_result_t qspi_boot_erase_sector(uint32_t addr);

/**
 * @brief Escreve até 256 bytes numa página da flash externa (bloqueante).
 *
 * @param addr  Endereço na flash (deve estar dentro de uma única página).
 * @param buf   Dados a escrever.
 * @param len   Número de bytes (máximo W25Q_PAGE_SIZE).
 */
qspi_boot_result_t qspi_boot_write_page(uint32_t addr,
                                         const uint8_t *buf,
                                         uint32_t len);

#endif /* QSPI_BOOT_H */
