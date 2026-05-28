package org.example.model

data class TelemetryData(
    val timestamp: Long = System.currentTimeMillis(),
    val temperature: Double? = null,
    val voltage: Double? = null,
    val current: Double? = null,
    val batteryLevel: Int? = null,
    val altitude: Double? = null,
    val latitude: Double? = null,
    val longitude: Double? = null,
    val signalStrength: Int? = null,
    val rssi: Int? = null
)
