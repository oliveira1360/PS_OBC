package org.example.domain.sensor

import java.time.Instant

/**
 * Agregado de domínio — representa um instante completo de telemetria do satélite.
 * Usa Value Objects em vez de primitivos soltos para garantir validação em toda a cadeia.
 */
data class TelemetrySnapshot(
    val recordedAt: Instant = Instant.now(),
    val source: String = "UNKNOWN",

    // EPS
    val voltage:      Voltage?     = null,
    val current:      Current?     = null,

    // Ambiente
    val temperature:  Temperature? = null,
    val pressure:     Pressure?    = null,

    // Navegação
    val coordinates:  Coordinates? = null,

    // Atitude
    val imu:          ImuReading?  = null,

    // Comunicações
    val rssi:         Int?         = null,   // dBm — tipo primitivo por ser inerentemente inteiro
    val doppler:      Double?      = null    // Hz
) {
    /** True se algum subsistema crítico reporta uma condição de alerta. */
    fun hasCriticalCondition(): Boolean =
        temperature?.isCritical() == true ||
        voltage?.isCritical()     == true ||
        current?.isCritical()     == true ||
        imu?.isTumbling()         == true

    /** Converte para o modelo TelemetryData legacy (compatibilidade com o frontend actual). */
    fun toLegacyModel(): org.example.domain.TelemetryData =
        org.example.domain.TelemetryData(
            timestamp    = recordedAt.toEpochMilli(),
            voltage      = voltage?.volts,
            current      = current?.amperes,
            batteryLevel = voltage?.batteryPercent(),
            temperature  = temperature?.celsius,
            pressure     = pressure?.hPa,
            latitude     = coordinates?.latitude,
            longitude    = coordinates?.longitude,
            altitude     = coordinates?.altitudeKm,
            speed        = coordinates?.speedKmS,
            doppler      = doppler,
            accelX       = imu?.accelX,
            accelY       = imu?.accelY,
            accelZ       = imu?.accelZ,
            gyroX        = imu?.gyroX,
            gyroY        = imu?.gyroY,
            gyroZ        = imu?.gyroZ,
            magX         = imu?.magX,
            magY         = imu?.magY,
            magZ         = imu?.magZ,
            rssi         = rssi
        )
}
