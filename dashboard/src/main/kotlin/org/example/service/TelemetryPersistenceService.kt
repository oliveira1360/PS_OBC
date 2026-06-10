package org.example.service

import org.example.domain.TelemetryData
import org.example.domain.sensor.TelemetrySnapshot
import org.example.repository.TelemetryReadingEntity
import org.example.repository.TelemetryReadingRepository
import org.springframework.stereotype.Service
import java.time.Instant

/**
 * Persiste um TelemetrySnapshot na base de dados, uma linha por sensor.
 * A granularidade por campo permite queries eficientes sobre sensores individuais.
 */
@Service
class TelemetryPersistenceService(
    private val repo: TelemetryReadingRepository
) {
    fun persist(snapshot: TelemetrySnapshot, source: String = "LIVE") {
        val readings = buildReadings(snapshot, source)
        if (readings.isNotEmpty()) repo.saveAll(readings)
    }

    /**
     * Persiste a telemetria ao vivo (TelemetryData) na BD, uma linha por sensor
     * presente. É isto que alimenta o separador "Histórico". As chaves de sensor
     * coincidem com as usadas em [buildReadings].
     */
    fun persistData(t: TelemetryData, source: String = "LIVE") {
        val at = Instant.ofEpochMilli(t.timestamp)
        val readings = buildList {
            t.temperature?.let { add(numeric("temperature", it, at, source)) }
            t.pressure?.let    { add(numeric("pressure",    it, at, source)) }
            t.voltage?.let     { add(numeric("voltage",     it, at, source)) }
            t.current?.let     { add(numeric("current",     it, at, source)) }
            t.latitude?.let    { add(numeric("latitude",    it, at, source)) }
            t.longitude?.let   { add(numeric("longitude",   it, at, source)) }
            t.altitude?.let    { add(numeric("altitude",    it, at, source)) }
            t.speed?.let       { add(numeric("speed",       it, at, source)) }
            t.accelX?.let      { add(numeric("accel_x",     it, at, source)) }
            t.accelY?.let      { add(numeric("accel_y",     it, at, source)) }
            t.accelZ?.let      { add(numeric("accel_z",     it, at, source)) }
            t.gyroX?.let       { add(numeric("gyro_x",      it, at, source)) }
            t.gyroY?.let       { add(numeric("gyro_y",      it, at, source)) }
            t.gyroZ?.let       { add(numeric("gyro_z",      it, at, source)) }
            t.magX?.let        { add(numeric("mag_x",       it, at, source)) }
            t.magY?.let        { add(numeric("mag_y",       it, at, source)) }
            t.magZ?.let        { add(numeric("mag_z",       it, at, source)) }
            t.doppler?.let     { add(numeric("doppler",     it, at, source)) }
            t.rssi?.let        { add(integer("rssi",        it, at, source)) }
        }
        if (readings.isNotEmpty()) repo.saveAll(readings)
    }

    private fun buildReadings(s: TelemetrySnapshot, src: String): List<TelemetryReadingEntity> =
        buildList {
            val at = s.recordedAt
            s.temperature?.let  { add(numeric("temperature",   it.celsius,      at, src)) }
            s.pressure?.let     { add(numeric("pressure",      it.hPa,          at, src)) }
            s.voltage?.let      { add(numeric("voltage",       it.volts,        at, src)) }
            s.current?.let      { add(numeric("current",       it.amperes,      at, src)) }
            s.coordinates?.let {
                add(numeric("latitude",   it.latitude,    at, src))
                add(numeric("longitude",  it.longitude,   at, src))
                add(numeric("altitude",   it.altitudeKm,  at, src))
                add(numeric("speed",      it.speedKmS,    at, src))
            }
            s.imu?.let {
                add(numeric("accel_x", it.accelX, at, src))
                add(numeric("accel_y", it.accelY, at, src))
                add(numeric("accel_z", it.accelZ, at, src))
                add(numeric("gyro_x",  it.gyroX,  at, src))
                add(numeric("gyro_y",  it.gyroY,  at, src))
                add(numeric("gyro_z",  it.gyroZ,  at, src))
                add(numeric("mag_x",   it.magX,   at, src))
                add(numeric("mag_y",   it.magY,   at, src))
                add(numeric("mag_z",   it.magZ,   at, src))
            }
            s.rssi?.let   { add(integer("rssi",    it, at, src)) }
            s.doppler?.let{ add(numeric("doppler",  it, at, src)) }
        }

    private fun numeric(key: String, value: Double, at: Instant, src: String) =
        TelemetryReadingEntity(sensorKey = key, valueNumeric = value, recordedAt = at, source = src)

    private fun integer(key: String, value: Int, at: Instant, src: String) =
        TelemetryReadingEntity(sensorKey = key, valueInteger = value, recordedAt = at, source = src)
}
