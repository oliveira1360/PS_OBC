#include <stdio.h>
#include "hal/hal_i2c.h"
#include "test_utils.h" 

void test_i2c_read(void);
void test_i2c_nack(void);
void test_i2c_multiplos_perifericos(void);
void test_i2c_nao_bloqueia(void);

int main(void) {
    printf("=== A correr testes ===\n");
    hal_i2c_init();

    test_i2c_read();
    test_i2c_nack();
    test_i2c_multiplos_perifericos();
    test_i2c_nao_bloqueia();


    printf("\n=== Testes concluidos ===\n");
    return 0;
}