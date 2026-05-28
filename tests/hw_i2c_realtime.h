/**
 * @file hw_i2c_realtime.h
 * @brief Teste de determinismo I2C para correr directamente na placa.
 *
 * Como usar na main():
 *
 *   #include "../../tests/hw_i2c_realtime.h"   // ajusta o caminho
 *
 *   int main(void) {
 *       // ... inicializações normais do sistema ...
 *       hal_i2c_init();
 *
 *       hw_i2c_realtime_test();   // <- adiciona esta linha
 *
 *       // ... resto do loop principal ...
 *   }
 *
 * Output via printf (mapeado para UART/SWO):
 *   Imprime tick counts para IMU, EPS, Pressure, Temperature e GNSS
 *   em 10 execuções cada, provando que o valor é sempre idêntico.
 *
 * Não depende do framework de testes (sem setjmp, sem TEST_BEGIN/END).
 * Funciona com qualquer printf redirigido para UART.
 */

#ifndef HW_I2C_REALTIME_H
#define HW_I2C_REALTIME_H

/**
 * @brief Corre o teste de determinismo I2C na placa real.
 *
 * Para cada sensor I2C (IMU, EPS, Pressure, Temperature, GNSS):
 *   - Inicia 10 leituras independentes via i2c_tick()
 *   - Conta os ticks necessários para completar cada leitura
 *   - Imprime uma tabela com os resultados
 *   - Imprime PASS se todos os ticks forem iguais, FAIL caso contrário
 *
 * Chama directamente da main() antes do loop principal.
 */
void hw_i2c_realtime_test(void);

#endif /* HW_I2C_REALTIME_H */
