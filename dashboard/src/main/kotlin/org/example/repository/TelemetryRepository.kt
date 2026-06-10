package org.example.repository

import org.example.domain.TelemetryData
import org.springframework.stereotype.Repository

interface TelemetryRepository {
    fun getLatest(): TelemetryData
    fun save(data: TelemetryData)
}

@Repository
class InMemoryTelemetryRepository : TelemetryRepository {
    @Volatile private var latest = TelemetryData()
    override fun getLatest() = latest
    override fun save(data: TelemetryData) { latest = data }
}
