package org.example.domain

data class TelemetryData(
    val timestamp: Long = System.currentTimeMillis(),
    // EPS
    val voltage: Double? = null,
    val current: Double? = null,
    val batteryLevel: Int? = null,
    // Pressure & Temperature
    val temperature: Double? = null,
    val pressure: Double? = null,
    // GNSS
    val latitude: Double? = null,
    val longitude: Double? = null,
    val altitude: Double? = null,
    val speed: Double? = null,
    // USART / Doppler
    val doppler: Double? = null,
    // IMU
    val accelX: Double? = null,
    val accelY: Double? = null,
    val accelZ: Double? = null,
    val gyroX: Double? = null,
    val gyroY: Double? = null,
    val gyroZ: Double? = null,
    val magX: Double? = null,
    val magY: Double? = null,
    val magZ: Double? = null,
    // Comms
    val rssi: Int? = null,
    val signalStrength: Int? = null
)
