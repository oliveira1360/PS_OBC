#ifndef SEU_INJECTION_TEST_H
#define SEU_INJECTION_TEST_H

/**
 * Teste de injeção de SEU sobre a proteção TMR das estruturas globais
 * (módulo seu_data). Corrompe deliberadamente cópias e verifica que a
 * votação 2-de-3 deteta, repara e mantém o sistema num estado coerente.
 *
 * Chamar directamente da main(), depois de system_init().
 */
void test_seu_injection(void);

#endif
