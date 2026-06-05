/**
 * @file qspi_test.c
 * @brief Testes de leitura/escrita para a flash S25FL116K via QSPI.
 *
 * Sequência de cada teste:
 *   1. Erase do sector de teste (4 KB)
 *   2. Verifica que o sector está apagado (todos 0xFF)
 *   3. Escreve um padrão conhecido (256 bytes)
 *   4. Lê de volta e compara byte a byte
 *   5. Reporta PASS / FAIL via printf (UART debug)
 *
 * Endereço de teste: 0x010000 (início da região de logs —
 * longe do OTA em 0x100000 e da config em 0x000000).
 */

#include "testsForBoard/qspi_test.h"
#include "hal/hal_qspi.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* -------------------------------------------------------------------------
 * Configuração do teste
 * ------------------------------------------------------------------------- */
#define TEST_SECTOR_ADDR   0x010000UL  /* Sector 16 — região de logs        */
#define TEST_PAGE_SIZE     256U        /* Uma página da S25FL116K            */
#define BUSY_TIMEOUT       10000000UL  /* 10 Milhões - dá tempo à flash!     */

/* -------------------------------------------------------------------------
 * Utilitário: aguarda fim de erase/write com timeout
 * Devolve 1 se OK, 0 se timeout (COM DEBUG PRINTS)
 * ------------------------------------------------------------------------- */
static uint8_t wait_not_busy(void)
{
    uint32_t count = 0U;
    uint8_t status;

    while (1)
    {
        /* Lemos o valor cru do Status Register em vez de apenas verificar o bit */
        status = hal_qspi_read_status();
        
        /* Bit 0 (0x01) é o BUSY bit na S25FL116K */
        if ((status & 0x01U) == 0U) 
        {
            return 1U; /* OK, já não está ocupada */
        }

        count++;
        if (count >= BUSY_TIMEOUT)
        {
            /* Se esgotou o tempo, imprime qual foi o valor lido */
            printf("\r\n      -> [DEBUG] TIMEOUT! Status Reg bloqueado em: 0x%02X ", status);
            return 0U; /* falhou */
        }
    }
}

/* =========================================================================
 * Teste 1 — Erase + verifica 0xFF
 * ========================================================================= */
static uint8_t test_erase(void)
{
    uint8_t buf[TEST_PAGE_SIZE];

    printf("  [T1] Erase sector 0x%06lX ... ", (unsigned long)TEST_SECTOR_ADDR);

    /* WREN obrigatorio antes de qualquer escrita/erase */
    hal_qspi_send_command(W25Q_CMD_WRITE_ENABLE);
    hal_qspi_send_command_addr(W25Q_CMD_SECTOR_ERASE, TEST_SECTOR_ADDR);

    if (!wait_not_busy())
    {
        printf("TIMEOUT [FAIL]\r\n");
        return 0U;
    }

    /* Verifica que os primeiros 256 bytes estao a 0xFF (apagados) */
    hal_qspi_read_memory(TEST_SECTOR_ADDR, buf, TEST_PAGE_SIZE);

    for (uint32_t i = 0U; i < TEST_PAGE_SIZE; i++)
    {
        if (buf[i] != 0xFFU)
        {
            printf("byte[%lu]=0x%02X != 0xFF [FAIL]\r\n",
                   (unsigned long)i, buf[i]);
            return 0U;
        }
    }

    printf("OK [PASS]\r\n");
    return 1U;
}

/* =========================================================================
 * Teste 2 — Escreve padrao 0xA5/0x5A alternado e le de volta
 * ========================================================================= */
static uint8_t test_write_read_pattern(void)
{
    uint8_t write_buf[TEST_PAGE_SIZE];
    uint8_t read_buf[TEST_PAGE_SIZE];
    uint32_t errors = 0U;

    /* Preenche com padrao alternado: 0xA5, 0x5A, 0xA5, 0x5A ... */
    for (uint32_t i = 0U; i < TEST_PAGE_SIZE; i++)
    {
        write_buf[i] = (i % 2U == 0U) ? 0xA5U : 0x5AU;
    }

    printf("  [T2] Escrita padrao 0xA5/0x5A ... ");

    hal_qspi_send_command(W25Q_CMD_WRITE_ENABLE);
    hal_qspi_write_memory(TEST_SECTOR_ADDR, write_buf, TEST_PAGE_SIZE);

    if (!wait_not_busy())
    {
        printf("TIMEOUT [FAIL]\r\n");
        return 0U;
    }

    /* Le de volta */
    memset(read_buf, 0x00U, TEST_PAGE_SIZE);
    hal_qspi_read_memory(TEST_SECTOR_ADDR, read_buf, TEST_PAGE_SIZE);

    for (uint32_t i = 0U; i < TEST_PAGE_SIZE; i++)
    {
        if (read_buf[i] != write_buf[i])
        {
            if (errors < 5U)  /* Mostra ate 5 erros para nao encher o terminal */
            {
                printf("\r\n  [T2]   byte[%lu]: escrito=0x%02X lido=0x%02X",
                       (unsigned long)i, write_buf[i], read_buf[i]);
            }
            errors++;
        }
    }

    if (errors == 0U)
    {
        printf("OK [PASS]\r\n");
        return 1U;
    }
    else
    {
        printf("\r\n  [T2] %lu erros [FAIL]\r\n", (unsigned long)errors);
        return 0U;
    }
}

/* =========================================================================
 * Teste 3 — Escreve contador (0x00..0xFF) e le de volta
 * ========================================================================= */
static uint8_t test_write_read_counter(void)
{
    uint8_t write_buf[TEST_PAGE_SIZE];
    uint8_t read_buf[TEST_PAGE_SIZE];
    uint32_t errors = 0U;

    /* Precisa de novo erase (flash nao pode ir de 0→1 sem erase) */
    hal_qspi_send_command(W25Q_CMD_WRITE_ENABLE);
    hal_qspi_send_command_addr(W25Q_CMD_SECTOR_ERASE, TEST_SECTOR_ADDR);
    if (!wait_not_busy())
    {
        printf("  [T3] Erase TIMEOUT [FAIL]\r\n");
        return 0U;
    }

    /* Preenche com 0x00, 0x01, 0x02 ... 0xFF */
    for (uint32_t i = 0U; i < TEST_PAGE_SIZE; i++)
    {
        write_buf[i] = (uint8_t)i;
    }

    printf("  [T3] Escrita contador 0x00..0xFF ... ");

    hal_qspi_send_command(W25Q_CMD_WRITE_ENABLE);
    hal_qspi_write_memory(TEST_SECTOR_ADDR, write_buf, TEST_PAGE_SIZE);

    if (!wait_not_busy())
    {
        printf("TIMEOUT [FAIL]\r\n");
        return 0U;
    }

    memset(read_buf, 0xFFU, TEST_PAGE_SIZE);
    hal_qspi_read_memory(TEST_SECTOR_ADDR, read_buf, TEST_PAGE_SIZE);

    for (uint32_t i = 0U; i < TEST_PAGE_SIZE; i++)
    {
        if (read_buf[i] != write_buf[i])
        {
            if (errors < 5U)
            {
                printf("\r\n  [T3]   byte[%lu]: escrito=0x%02X lido=0x%02X",
                       (unsigned long)i, write_buf[i], read_buf[i]);
            }
            errors++;
        }
    }

    if (errors == 0U)
    {
        printf("OK [PASS]\r\n");
        return 1U;
    }
    else
    {
        printf("\r\n  [T3] %lu erros [FAIL]\r\n", (unsigned long)errors);
        return 0U;
    }
}

/* =========================================================================
 * Ponto de entrada público
 * ========================================================================= */
void test_qspi_rw(void)
{
    uint8_t passed = 0U;
    uint8_t total  = 3U;

    printf("\r\n");
    printf("  +--------------------------------------------------+\r\n");
    printf("  |  TESTE QSPI: S25FL116K — LEITURA / ESCRITA       |\r\n");
    printf("  |  Sector de teste: 0x%06lX  (regiao logs)        |\r\n",
           (unsigned long)TEST_SECTOR_ADDR);
    printf("  +--------------------------------------------------+\r\n");

    /* Inicializa o HAL QSPI (ja chamado em system_init, mas seguro chamar de novo) */
    hal_qspi_init();

    if (test_erase())         passed++;
    if (test_write_read_pattern()) passed++;
    if (test_write_read_counter()) passed++;

    printf("  +--------------------------------------------------+\r\n");
    printf("  |  Resultado: %u/%u testes passaram               |\r\n",
           passed, total);
    if (passed == total)
        printf("  |  QSPI OK — flash operacional            [PASS]  |\r\n");
    else
        printf("  |  FALHA em %u teste(s) — verificar hw   [FAIL]  |\r\n",
               total - passed);
    printf("  +--------------------------------------------------+\r\n");
    printf("\r\n");
}
