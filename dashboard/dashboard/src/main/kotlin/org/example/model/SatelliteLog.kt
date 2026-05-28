package org.example.model

data class SatelliteLog(
    val id: Long = System.currentTimeMillis(),
    val timestamp: Long = System.currentTimeMillis(),
    val type: LogType = LogType.INFO,
    val message: String,
    val raw: String? = null
)

enum class LogType { INFO, WARNING, ERROR, COMMAND, RESPONSE, SYSTEM }
