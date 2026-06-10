package org.example.domain.sensor

import kotlin.math.sqrt

/**
 * Value Object — Leitura completa da IMU (acelerómetro + giroscópio + magnetómetro).
 * Sensor ICM-42688-P. Unidades: g, °/s, µT.
 */
data class ImuReading(
    val accelX: Double,   // g
    val accelY: Double,
    val accelZ: Double,
    val gyroX: Double,    // °/s
    val gyroY: Double,
    val gyroZ: Double,
    val magX: Double = 0.0,   // µT
    val magY: Double = 0.0,
    val magZ: Double = 0.0
) {
    init {
        require(accelX in ACCEL_RANGE && accelY in ACCEL_RANGE && accelZ in ACCEL_RANGE) {
            "Aceleração fora do intervalo ±${ACCEL_MAX} g"
        }
        require(gyroX in GYRO_RANGE && gyroY in GYRO_RANGE && gyroZ in GYRO_RANGE) {
            "Giroscópio fora do intervalo ±${GYRO_MAX} °/s"
        }
    }

    /** Magnitude do vector de aceleração (norma L2). */
    val accelMagnitude: Double get() = sqrt(accelX * accelX + accelY * accelY + accelZ * accelZ)

    /** Magnitude do vector de velocidade angular. */
    val gyroMagnitude: Double get() = sqrt(gyroX * gyroX + gyroY * gyroY + gyroZ * gyroZ)

    /** True se o satélite está a tumble (rotação excessiva > 50 °/s). */
    fun isTumbling(): Boolean = gyroMagnitude > TUMBLE_THRESHOLD

    /** True se a aceleração total difere de 0g por mais de 0.5 g (perturbação). */
    fun hasExcessiveVibration(): Boolean = accelMagnitude > 0.5

    companion object {
        const val ACCEL_MAX       = 16.0
        const val GYRO_MAX        = 2000.0
        const val TUMBLE_THRESHOLD = 50.0
        private val ACCEL_RANGE   = -ACCEL_MAX..ACCEL_MAX
        private val GYRO_RANGE    = -GYRO_MAX..GYRO_MAX
    }
}
