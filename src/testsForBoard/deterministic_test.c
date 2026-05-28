#include "testsForBoard/deterministic_test.h"
#include <stdint.h>
#include "drivers/i2c_driver.h"
#include "config/board.h"
#include <string.h>
#include <stdio.h>

void test_i2c_determinism(void)
{
    static const struct
    {
        const char *name;
        uint8_t addr;
        uint8_t use_reg;
        uint8_t reg;
        uint8_t n_bytes;
    } sensors[] = {
        {"IMU   (0x68)  6B  reg=0x3B", IMU_ADDR, 1, 0x3B, 6},
        {"EPS   (0x60)  2B          ", EPS_ADDR, 1, 0x00, EPS_BUF_LEN},
        {"Press (0x77)  2B          ", PRESS_ADDR, 1, 0x00, PRES_BUF_LEN},
        {"Temp  (0x48)  2B          ", TEMP_ADDR, 1, 0x00, TEMP_BUF_LEN},
        {"GNSS  (0x42) 18B          ", GNSS_ADDR, 0, 0x00, GNSS_BUF_LEN},
    };
    static const int N_SENSORS = 5;

    printf("\r\n");
    printf("  +--------------------------------------------------+\r\n");
    printf("  |  TESTE I2C: NAO-BLOQUEIO & DETERMINISMO          |\r\n");
    printf("  |  Bus: %d kHz  |  Execucoes por sensor: %d        |\r\n",
           I2C_SPEED_KHZ, I2C_TEST_N_RUNS);
    printf("  +--------------------------------------------------+\r\n");

    for (int s = 0; s < N_SENSORS; s++)
    {

        {
            i2c_handle_t dummy;
            uint8_t dummy_buf[GNSS_BUF_LEN];
            memset(&dummy, 0, sizeof(dummy));
            dummy.addr = sensors[s].addr;
            dummy.buf = dummy_buf;
            dummy.len = sensors[s].n_bytes;
            dummy.rw = 1;
            dummy.reg = sensors[s].reg;
            dummy.use_reg = sensors[s].use_reg;
            dummy.state = I2C_STARTING;
            while (dummy.state != I2C_IDLE)
            {
                i2c_tick(&dummy);
            }
        }
        uint32_t counts[I2C_TEST_N_RUNS];
        uint32_t min_t = 0xFFFFFFFFU, max_t = 0U;

        printf("\r\n  Sensor: %s\r\n", sensors[s].name);
        printf("  +---------------------------+\r\n");

        for (int run = 0; run < I2C_TEST_N_RUNS; run++)
        {

            i2c_handle_t h;
            uint8_t buf[GNSS_BUF_LEN];
            memset(&h, 0, sizeof(h));
            memset(buf, 0, sizeof(buf));

            /* Setup equivalente a *_read_async().
             * Nao bloqueante: devolve o controlo aqui sem esperar pelo bus. */
            h.addr = sensors[s].addr;
            h.buf = buf;
            h.len = sensors[s].n_bytes;
            h.rw = 1;
            h.reg = sensors[s].reg;
            h.use_reg = sensors[s].use_reg;
            h.state = I2C_STARTING;

            /* Conta ticks ate a transacao completar */
            uint32_t ticks = 0;
            while (h.state != I2C_IDLE && ticks < I2C_TEST_MAX_TICKS)
            {
                i2c_tick(&h);
                ticks++;
            }

            counts[run] = ticks;
            if (ticks < min_t)
                min_t = ticks;
            if (ticks > max_t)
                max_t = ticks;

            /* Imprime apenas as primeiras e ultimas 5 execucoes */
            if (run < 5 || run >= I2C_TEST_N_RUNS - 5)
                printf("  |  Run %3d : %6lu ticks    |\r\n",
                       run + 1, (unsigned long)ticks);
            if (run == 5)
                printf("  |  ...                      |\r\n");
        }

        /* Jitter maximo aceitavel em bare metal:
         * +-1 tick por byte e o limite fisico do polling assincrono.
         * O hardware I2C (25 kHz) e a CPU sao assincronos — quando um
         * byte termina exactamente na fronteira de um poll, o resultado
         * varia 1 tick. Isso e inevitavel e nao afecta o WCET. */
        uint32_t jitter = max_t - min_t;
        uint32_t max_jitter = (uint32_t)sensors[s].n_bytes + 2U;

        printf("  +---------------------------+\r\n");
        printf("  |  Min=%-6lu  Max=%-6lu    |\r\n",
               (unsigned long)min_t, (unsigned long)max_t);
        printf("  |  Jitter: %lu tick(s)       |\r\n", (unsigned long)jitter);
        if (jitter <= max_jitter)
            printf("  |  DETERMINISTICO     [OK]  |\r\n");
        else
            printf("  |  JITTER EXCESSIVO   [!!]  |\r\n");
        printf("  +---------------------------+\r\n");
    }

    printf("\r\n");
    printf("  +--------------------------------------------------+\r\n");
    printf("  |  Teste concluido. A iniciar OBC loop principal.  |\r\n");
    printf("  +--------------------------------------------------+\r\n");
    printf("\r\n");
}