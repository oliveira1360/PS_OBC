package org.example.repository

import jakarta.persistence.*
import org.springframework.data.jpa.repository.JpaRepository
import org.springframework.data.jpa.repository.Query
import org.springframework.data.repository.query.Param
import org.springframework.stereotype.Repository
import java.time.Instant

// ── Entity ─────────────────────────────────────────────────────────────────────

@Entity
@Table(
    name = "telemetry_readings",
    indexes = [
        Index(name = "idx_telemetry_recorded_at", columnList = "recorded_at DESC"),
        Index(name = "idx_telemetry_sensor_type", columnList = "sensor_type_id, recorded_at DESC")
    ]
)
class TelemetryReadingEntity(

    @Id @GeneratedValue(strategy = GenerationType.IDENTITY)
    val id: Long = 0,

    @Column(name = "recorded_at", nullable = false)
    val recordedAt: Instant = Instant.now(),

    @Column(name = "sensor_key", nullable = false, length = 50)
    val sensorKey: String,

    @Column(name = "value_numeric")
    val valueNumeric: Double? = null,

    @Column(name = "value_integer")
    val valueInteger: Int? = null,

    @Column(name = "source", length = 50)
    val source: String? = null,

    @Column(name = "quality")
    val quality: Short = 100
)

// ── Spring Data interface (implementação gerada pelo Spring) ───────────────────

@Repository
interface TelemetryReadingRepository : JpaRepository<TelemetryReadingEntity, Long> {

    fun findBySensorKeyAndRecordedAtBetweenOrderByRecordedAtAsc(
        sensorKey: String,
        from: Instant,
        to: Instant
    ): List<TelemetryReadingEntity>

    @Query("""
        SELECT r FROM TelemetryReadingEntity r
        WHERE r.sensorKey IN :keys
          AND r.recordedAt BETWEEN :from AND :to
        ORDER BY r.recordedAt ASC
    """)
    fun findByKeysAndTimeRange(
        @Param("keys") keys: Collection<String>,
        @Param("from") from: Instant,
        @Param("to")   to: Instant
    ): List<TelemetryReadingEntity>

    @Query("""
        SELECT r FROM TelemetryReadingEntity r
        WHERE r.recordedAt = (
            SELECT MAX(r2.recordedAt) FROM TelemetryReadingEntity r2
            WHERE r2.sensorKey = r.sensorKey
        )
    """)
    fun findLatestPerSensor(): List<TelemetryReadingEntity>

    @Query("SELECT DISTINCT r.sensorKey FROM TelemetryReadingEntity r ORDER BY r.sensorKey")
    fun findDistinctSensorKeys(): List<String>
}
