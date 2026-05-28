#ifndef QSPI_TEST_H
#define QSPI_TEST_H

/**
 * @file qspi_test.h
 * @brief Testes de leitura/escrita para a flash S25FL116K via QSPI.
 *
 * Usa a mesma UART debug que test_i2c_determinism().
 * Requer USE_REAL_HW 1 em board.h para testar hardware real.
 */

void test_qspi_rw(void);

#endif /* QSPI_TEST_H */
