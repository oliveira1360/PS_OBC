#include "test_utils.h"
#include "drivers/i2c_driver.h"
#include "hal/hal_i2c.h"
#include "config/board.h"

/* ---------- callbacks ---------- */
static int cb_gnss = -99;
static int cb_imu  = -99;
static int cb_pres = -99;

static void on_gnss(int r) { cb_gnss = r; }
static void on_imu(int r)  { cb_imu  = r; }
static void on_pres(int r) { cb_pres = r; }

/* ---------- testes existentes ---------- */
void test_i2c_read(void) { /* ... */ }
void test_i2c_nack(void) { /* ... */ }

/* ---------- novo teste ---------- */
void test_i2c_multiplos_perifericos(void) {
    TEST("multiplos perifericos em paralelo sem bloqueio");

    i2c_handle_t h_gnss = {0};
    i2c_handle_t h_imu  = {0};
    i2c_handle_t h_pres = {0};

    uint8_t buf_gnss[GNSS_BUF_LEN] = {0};
    uint8_t buf_imu[IMU_BUF_LEN]   = {0};
    uint8_t buf_pres[PRES_BUF_LEN] = {0};

    cb_gnss = -99;
    cb_imu  = -99;
    cb_pres = -99;

    /* arranca os 3 ao mesmo tempo */
    i2c_start_read(&h_gnss, GNSS_ADDR, buf_gnss, GNSS_BUF_LEN, on_gnss);
    i2c_start_read(&h_imu,  IMU_ADDR,  buf_imu,  IMU_BUF_LEN,  on_imu);
    i2c_start_read(&h_pres, PRES_ADDR, buf_pres, PRES_BUF_LEN, on_pres);

    ASSERT(h_gnss.state == I2C_STARTING, "gnss arranca em STARTING");
    ASSERT(h_imu.state  == I2C_STARTING, "imu arranca em STARTING");
    ASSERT(h_pres.state == I2C_STARTING, "pres arranca em STARTING");

    /* conta quantos ticks foram precisos */
    int ticks = 0;
    int max_ticks = 100;

    while (ticks < max_ticks) {
        /* um tick por periferico por iteracao — nao bloqueia */
        i2c_tick(&h_gnss);
        i2c_tick(&h_imu);
        i2c_tick(&h_pres);
        ticks++;

        /* sai quando todos terminaram */
        if (h_gnss.state == I2C_IDLE &&
            h_imu.state  == I2C_IDLE &&
            h_pres.state == I2C_IDLE)
            break;
    }

    /* todos completaram */
    ASSERT(h_gnss.state == I2C_IDLE, "gnss terminou em IDLE");
    ASSERT(h_imu.state  == I2C_IDLE, "imu terminou em IDLE");
    ASSERT(h_pres.state == I2C_IDLE, "pres terminou em IDLE");

    /* callbacks chamados com sucesso */
    ASSERT(cb_gnss == 0, "gnss callback sucesso");
    ASSERT(cb_imu  == 0, "imu callback sucesso");
    ASSERT(cb_pres == 0, "pres callback sucesso");

    /* buffers preenchidos */
    int gnss_ok = 0, imu_ok = 0, pres_ok = 0;
    for (int i = 0; i < GNSS_BUF_LEN; i++) if (buf_gnss[i] != 0) gnss_ok = 1;
    for (int i = 0; i < IMU_BUF_LEN;  i++) if (buf_imu[i]  != 0) imu_ok  = 1;
    for (int i = 0; i < PRES_BUF_LEN; i++) if (buf_pres[i] != 0) pres_ok = 1;

    ASSERT(gnss_ok, "gnss buffer tem dados");
    ASSERT(imu_ok,  "imu buffer tem dados");
    ASSERT(pres_ok, "pres buffer tem dados");

    /* nao bloqueou — completou em menos de max_ticks */
    /* gnss precisa ~10 ticks, imu ~20, pres ~4       */
    /* o maior (imu=18 bytes) precisa de ~22 ticks    */
    ASSERT(ticks < max_ticks, "completou sem atingir limite de ticks");
    printf("  INFO: completou em %d ticks\n", ticks);

    /* verifica que os perifericos nao interferiram */
    /* cada handle tem o seu proprio index          */
    ASSERT(h_gnss.index == GNSS_BUF_LEN, "gnss leu todos os bytes");
    ASSERT(h_imu.index  == IMU_BUF_LEN,  "imu leu todos os bytes");
    ASSERT(h_pres.index == PRES_BUF_LEN, "pres leu todos os bytes");
}

void test_i2c_nao_bloqueia(void) {
    TEST("leitura nao bloqueia o loop principal");

    i2c_handle_t h_gnss = {0};
    i2c_handle_t h_imu  = {0};
    uint8_t buf_gnss[GNSS_BUF_LEN] = {0};
    uint8_t buf_imu[IMU_BUF_LEN]   = {0};

    cb_gnss = -99;
    cb_imu  = -99;

    i2c_start_read(&h_gnss, GNSS_ADDR, buf_gnss, GNSS_BUF_LEN, on_gnss);
    i2c_start_read(&h_imu,  IMU_ADDR,  buf_imu,  IMU_BUF_LEN,  on_imu);

    int contador_loop = 0;      /* simula trabalho do loop principal */
    int ticks_gnss_idle = -1;   /* tick em que gnss terminou */
    int ticks_imu_idle  = -1;   /* tick em que imu terminou */

    for (int t = 0; t < 100; t++) {
        /* trabalho do loop principal corre em CADA iteracao */
        contador_loop++;

        /* ticks avancam um estado por iteracao */
        i2c_tick(&h_gnss);
        i2c_tick(&h_imu);

        /* regista quando cada um terminou */
        if (h_gnss.state == I2C_IDLE && ticks_gnss_idle == -1)
            ticks_gnss_idle = t;
        if (h_imu.state == I2C_IDLE && ticks_imu_idle == -1)
            ticks_imu_idle = t;

        if (ticks_gnss_idle != -1 && ticks_imu_idle != -1)
            break;
    }

    /* o loop correu em CADA iteracao — nunca ficou bloqueado */
    ASSERT(contador_loop == ticks_imu_idle + 1,
           "loop principal correu em todas as iteracoes");

    /* gnss termina antes do imu (8 bytes vs 18 bytes) */
    ASSERT(ticks_gnss_idle < ticks_imu_idle,
           "gnss termina antes do imu (menos bytes)");

    /* entre o fim do gnss e o fim do imu, o loop continuou */
    ASSERT(ticks_imu_idle - ticks_gnss_idle > 0,
           "loop continuou apos gnss terminar enquanto imu ainda lia");

    printf("  INFO: gnss terminou no tick %d\n", ticks_gnss_idle);
    printf("  INFO: imu  terminou no tick %d\n", ticks_imu_idle);
    printf("  INFO: loop correu %d vezes\n", contador_loop);
}