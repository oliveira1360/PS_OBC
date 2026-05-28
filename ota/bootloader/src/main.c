/**
 * @file main.c
 * @brief Ponto de entrada do bootloader OTA — ATSAMV71Q21B.
 *
 * O main() é o ponto de entrada após o Reset_Handler do startup.
 * Inicializa o sistema e delega toda a lógica para bootloader_run(),
 * que não retorna (salta para a aplicação ou fica em loop de segurança).
 */

#include "bootloader.h"
#include "system_samv71.h"

int main(void)
{
    /* Inicialização mínima do sistema:
     *  - Desabilita WDT e RSWDT
     *  - Configura flash wait states (0 WS @ 12 MHz)
     *  - Activa clock do QSPI no PMC */
    system_boot_init();

    /* Executa lógica do bootloader.
     * Esta função NÃO retorna — salta para a aplicação. */
    (void)bootloader_run();

    /* Segurança: se bootloader_run() retornar inesperadamente */
    while (1)
    {
        /* loop de segurança */
    }

    return 0;  /* Nunca alcançado */
}
