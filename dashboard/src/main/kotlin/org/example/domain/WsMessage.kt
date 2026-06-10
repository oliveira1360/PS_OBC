package org.example.domain

data class WsMessage(
    val type: WsMessageType,
    val data: Any
)

enum class WsMessageType { TELEMETRY, STATUS, LOG, OTA_PROGRESS }
