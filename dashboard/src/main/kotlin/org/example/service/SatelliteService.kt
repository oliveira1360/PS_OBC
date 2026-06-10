package org.example.service

import org.example.domain.SatelliteLog
import org.example.domain.SatelliteStatus
import org.example.domain.TelemetryData
import org.example.repository.LogRepository
import org.example.repository.StatusRepository
import org.example.repository.TelemetryRepository
import org.springframework.stereotype.Service

// ── Interface ──────────────────────────────────────────────────────────────────

interface SatelliteService {
    fun getStatus(): SatelliteStatus
    fun getLatestTelemetry(): TelemetryData
    fun getRecentLogs(limit: Int = 200): List<SatelliteLog>
}

// ── Implementação ──────────────────────────────────────────────────────────────

@Service
class SatelliteServiceImpl(
    private val statusRepo: StatusRepository,
    private val telemetryRepo: TelemetryRepository,
    private val logRepo: LogRepository
) : SatelliteService {

    override fun getStatus(): SatelliteStatus = statusRepo.get()

    override fun getLatestTelemetry(): TelemetryData = telemetryRepo.getLatest()

    override fun getRecentLogs(limit: Int): List<SatelliteLog> = logRepo.findRecent(limit)
}
