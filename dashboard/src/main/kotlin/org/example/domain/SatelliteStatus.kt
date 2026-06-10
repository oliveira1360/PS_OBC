package org.example.domain

data class SatelliteStatus(
    val timestamp: Long = System.currentTimeMillis(),
    val connected: Boolean = false,
    val comPort: String? = null,
    val baudRate: Int = 9600,
    /** Formato de telemetria ativo: "RAW", "ASCII" ou "AUTO". */
    val telemetryFormat: String = "AUTO",
    val obc:     SubsystemStatus = SubsystemStatus.UNKNOWN,
    val power:   SubsystemStatus = SubsystemStatus.UNKNOWN,
    val comms:   SubsystemStatus = SubsystemStatus.UNKNOWN,
    val adcs:    SubsystemStatus = SubsystemStatus.UNKNOWN,
    val payload: SubsystemStatus = SubsystemStatus.UNKNOWN
)

enum class SubsystemStatus { OK, WARNING, ERROR, UNKNOWN }
