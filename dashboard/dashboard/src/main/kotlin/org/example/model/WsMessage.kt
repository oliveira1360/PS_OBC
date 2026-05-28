package org.example.model

data class WsMessage(
    val type: String,  // "TELEMETRY", "STATUS", "LOG", "OTA_PROGRESS"
    val data: Any
)
