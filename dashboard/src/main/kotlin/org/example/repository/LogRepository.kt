package org.example.repository

import org.example.domain.SatelliteLog
import org.springframework.stereotype.Repository
import java.util.concurrent.CopyOnWriteArrayList

interface LogRepository {
    fun findRecent(limit: Int = 200): List<SatelliteLog>
    fun save(log: SatelliteLog)
    fun count(): Int
}

@Repository
class InMemoryLogRepository : LogRepository {
    private val logs = CopyOnWriteArrayList<SatelliteLog>()
    private val maxSize = 500

    override fun findRecent(limit: Int): List<SatelliteLog> = logs.takeLast(limit)
    override fun count(): Int = logs.size

    override fun save(log: SatelliteLog) {
        logs.add(log)
        if (logs.size > maxSize) logs.removeAt(0)
    }
}
