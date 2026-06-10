package org.example.service

import org.example.repository.TelemetryReadingRepository
import org.example.service.error.ValidationError
import org.springframework.stereotype.Service
import java.time.Instant
import java.time.temporal.ChronoUnit

// ── DTOs ───────────────────────────────────────────────────────────────────────

data class SensorReadingDto(
    val sensorKey: String,
    val recordedAt: Instant,
    val valueNumeric: Double?,
    val valueInteger: Int?
)

data class SensorStatsDto(
    val sensorKey: String,
    val count: Int,
    val min: Double?,
    val max: Double?,
    val avg: Double?,
    val latest: Double?
)

// ── Interface ──────────────────────────────────────────────────────────────────

interface HistoryService {
    fun availableSensors(): List<String>
    fun history(sensors: List<String>, from: Instant?, to: Instant?): List<SensorReadingDto>
    fun stats(sensor: String, from: Instant?, to: Instant?): SensorStatsDto
}

// ── Implementação ──────────────────────────────────────────────────────────────

@Service
class HistoryServiceImpl(
    private val repo: TelemetryReadingRepository
) : HistoryService {

    override fun availableSensors(): List<String> =
        repo.findDistinctSensorKeys()

    override fun history(sensors: List<String>, from: Instant?, to: Instant?): List<SensorReadingDto> {
        if (sensors.isEmpty()) throw ValidationError("Pelo menos um sensor deve ser especificado")

        val end   = to   ?: Instant.now()
        val start = from ?: end.minus(1, ChronoUnit.HOURS)

        if (!start.isBefore(end)) throw ValidationError("O parâmetro 'from' deve ser anterior a 'to'")

        return repo.findByKeysAndTimeRange(sensors, start, end)
            .map { SensorReadingDto(it.sensorKey, it.recordedAt, it.valueNumeric, it.valueInteger) }
    }

    override fun stats(sensor: String, from: Instant?, to: Instant?): SensorStatsDto {
        val end   = to   ?: Instant.now()
        val start = from ?: end.minus(24, ChronoUnit.HOURS)

        val readings = repo.findBySensorKeyAndRecordedAtBetweenOrderByRecordedAtAsc(sensor, start, end)
        val values   = readings.mapNotNull { it.valueNumeric }

        if (values.isEmpty()) return SensorStatsDto(sensor, 0, null, null, null, null)

        return SensorStatsDto(
            sensorKey = sensor,
            count     = values.size,
            min       = values.min(),
            max       = values.max(),
            avg       = values.average(),
            latest    = values.last()
        )
    }
}
