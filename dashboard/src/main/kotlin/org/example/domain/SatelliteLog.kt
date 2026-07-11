package org.example.domain

import java.util.concurrent.atomic.AtomicLong

data class SatelliteLog(
    /**
     * Id único e monotónico. Antes era System.currentTimeMillis(), o que gerava
     * ids duplicados para logs no mesmo milissegundo — e o frontend deduplica
     * por id, pelo que entradas legítimas eram silenciosamente descartadas.
     */
    val id: Long = ID_GENERATOR.incrementAndGet(),
    val timestamp: Long = System.currentTimeMillis(),
    val type: LogType = LogType.INFO,
    val message: String,
    val raw: String? = null
) {
    companion object {
        private val ID_GENERATOR = AtomicLong(System.currentTimeMillis())
    }
}

enum class LogType { INFO, WARNING, ERROR, COMMAND, RESPONSE, SYSTEM }
