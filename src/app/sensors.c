#include <stdio.h>
#include "app/sensors.h"
#include "app/mission.h"
#include "peripherals/propulsor.h"


gnss_data_t gnss = {0};
imu_data_t imu = {0};
pressure_data_t pressure = {0};
temperature_data_t temperature = {0};
eps_data_t eps = {0};
ttc_data_t ttc = {0};
propulsor_data_t propulsor = {0};

float BATTERY_STATUS = 100.0f;

static uint8_t spi_turn = 0U;

void sensors_tick(void)
{
    eps_tick();
    gnss_tick();
    imu_tick();
    pressure_tick();
    temperature_tick();
    ttc_tick();
    propulsor_tick();
    ExtMem_Tick();
}

void sensors_read_all(void)
{
    if (spi_turn)
    {
        propulsor_read_async();
        spi_turn = 0U;
    }
    else
    {
        eps_read_async();
        gnss_read_async();
        imu_read_async();
        pressure_read_async();
        temperature_read_async();
        spi_turn = 1U;
    }
}

void sensors_save_to_flash(void)
{

    uint32_t frame_size = sizeof(ttc_data_t);

    ttc.eps = eps;
    ttc.temp = temperature;
    ttc.press = pressure;
    ttc.imu = imu;
    ttc.current_state = state;


    // 2. Segurança: Garantir que não excede o limite da página da Flash (256 bytes)
    if (frame_size <= 256U)
    {
        // 3. Verifica se a memória está livre para aceitar novos dados
        if (ExtMem_GetStatus() == EXT_MEM_IDLE) 
        {
            // 4. O "Casting" mágico: passa o ponteiro da struct global 'ttc' como array de uint8_t
            ExtMem_SaveTelemetryAsync((uint8_t *)&ttc, frame_size);
        }
        else
        {
            // A memória ainda está ocupada a gravar os dados anteriores ou num processo de Erase.
            // Neste caso, podes ignorar esta gravação ou criar um buffer circular na RAM se for crítico.
            // Para este nível de projeto, ignorar e esperar pelo próximo ciclo é normalmente suficiente.
        }
    }
    else
    {
        // Erro: Struct ultrapassou os 256 bytes!
        printf("ERRO: ttc_data_t (%lu bytes) excede a pagina QSPI (256 bytes)!\n", frame_size);
    }
}
void sensors_print(void)
{
    printf(
        "\r\nTELEM:{\"lat\":%.4f,\"lon\":%.4f,\"alt\":%.2f,\"spd\":%.2f,"
        "\"ax\":%.3f,\"ay\":%.3f,\"az\":%.3f,"
        "\"gx\":%.3f,\"gy\":%.3f,\"gz\":%.3f,"
        "\"mx\":%.3f,\"my\":%.3f,\"mz\":%.3f,"
        "\"pres\":%.2f,\"temp\":%.2f,"
        "\"volt\":%.3f,\"curr\":%.3f}\r\n",
        (double)gnss.latitude,  (double)gnss.longitude,
        (double)gnss.altitude,  (double)gnss.speed,
        (double)imu.ax, (double)imu.ay, (double)imu.az,
        (double)imu.gx, (double)imu.gy, (double)imu.gz,
        (double)imu.mx, (double)imu.my, (double)imu.mz,
        (double)pressure.pressure,
        (double)temperature.temperature,
        (double)eps.voltage, (double)eps.current
    );
}

/*
void sensors_print(void)
{
    printf("+----------------------------------------------------------------------+\n");
    printf("|                            TELEMETRY DATA                            |\n");
    printf("+----------------------------------------------------------------------+\n");
    printf("| --- GNSS ----------------------------------------------------------- |\n");
    printf("|  Latitude:    %-10.2f deg                                         |\n", gnss.latitude);
    printf("|  Longitude:   %-10.2f deg                                         |\n", gnss.longitude);
    printf("|  Altitude:    %-10.2f km                                          |\n", gnss.altitude);
    printf("|  Speed:       %-10.2f km/s                                       |\n", gnss.speed);
    printf("|                                                                      |\n");
    printf("| --- IMU ------------------------------------------------------------ |\n");
    printf("|  Accel:  X=%-7.2f Y=%-7.2f Z=%-7.2f m/s2                          |\n", imu.ax, imu.ay, imu.az);
    printf("|  Gyro:   X=%-7.2f Y=%-7.2f Z=%-7.2f deg/s                         |\n", imu.gx, imu.gy, imu.gz);
    printf("|  Mag:    X=%-7.2f Y=%-7.2f Z=%-7.2f uT                            |\n", imu.mx, imu.my, imu.mz);
    printf("|                                                                      |\n");
    printf("| --- Pressure & Temperature ----------------------------------------- |\n");
    printf("|  Pressure:    %-10.2f hPa                                         |\n", pressure.pressure);
    printf("|  Temperature: %-10.2f C                                           |\n", temperature.temperature);
    printf("|                                                                      |\n");
    printf("| --- EPS ------------------------------------------------------------ |\n");
    printf("|  Voltage:     %-10.2f V                                           |\n", eps.voltage);
    printf("|  Current:     %-10.2f A                                           |\n", eps.current);
    printf("| --- USART ------------------------------------------------------     |\n");
    printf("|  Doppler:     %-10.2f kHz                                         |\n", ttc.doppler);
    printf("+----------------------------------------------------------------------+\n\n");
    printf("\n\n\n\n\n");
}
*/