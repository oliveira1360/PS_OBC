package org.example.http.dto

data class ConnectRequest(
    val port: String,
    val baudRate: Int = 9600,
    /** Formato da telemetria: "RAW", "ASCII" ou "AUTO" (default). */
    val format: String = "AUTO"
)
