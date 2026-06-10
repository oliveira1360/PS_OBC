package org.example.service

import org.example.domain.TelemetryData
import java.util.concurrent.Executors
import java.util.concurrent.ScheduledFuture
import java.util.concurrent.TimeUnit

/**
 * Parses the OBC ASCII telemetry frame received line by line via UART.
 *
 * Strategy: accumulate data lines and emit via [onComplete] after a
 * FRAME_TIMEOUT_MS silence window. The opening "+---" / "TELEMETRY DATA"
 * lines mark the start of a new frame; no "+---" line ever triggers
 * emission (avoids false-early-emit when two interleaved frames arrive).
 */
class TelemetryFrameParser(
    private val onComplete: (TelemetryData) -> Unit
) {
    companion object {
        /** Silence after the last data line that counts as "frame done". */
        const val FRAME_TIMEOUT_MS = 800L
    }

    private val scheduler  = Executors.newSingleThreadScheduledExecutor()
    private val frameData  = mutableMapOf<String, Double>()
    private var inFrame    = false
    private var timeoutJob: ScheduledFuture<*>? = null

    // ── Regex patterns ─────────────────────────────────────────────────────

    private val patterns = listOf(
        "latitude"    to Regex("""Latitude\s*:\s*([-\d.]+)"""),
        "longitude"   to Regex("""Longitude\s*:\s*([-\d.]+)"""),
        "altitude"    to Regex("""Altitude\s*:\s*([-\d.]+)"""),
        "speed"       to Regex("""Speed\s*:\s*([-\d.]+)"""),
        "pressure"    to Regex("""Pressure\s*:\s*([-\d.]+)"""),
        "temperature" to Regex("""Temperature\s*:\s*([-\d.]+)"""),
        "voltage"     to Regex("""Voltage\s*:\s*([-\d.]+)"""),
        "current"     to Regex("""Current\s*:\s*([-\d.]+)"""),
        "doppler"     to Regex("""Doppler\s*:\s*([-\d.]+)"""),
        "accelX"      to Regex("""Accel\s*:.*?X\s*=\s*([-\d.]+)"""),
        "accelY"      to Regex("""Accel\s*:.*?Y\s*=\s*([-\d.]+)"""),
        "accelZ"      to Regex("""Accel\s*:.*?Z\s*=\s*([-\d.]+)"""),
        "gyroX"       to Regex("""Gyro\s*:.*?X\s*=\s*([-\d.]+)"""),
        "gyroY"       to Regex("""Gyro\s*:.*?Y\s*=\s*([-\d.]+)"""),
        "gyroZ"       to Regex("""Gyro\s*:.*?Z\s*=\s*([-\d.]+)"""),
        "magX"        to Regex("""Mag\s*:.*?X\s*=\s*([-\d.]+)"""),
        "magY"        to Regex("""Mag\s*:.*?Y\s*=\s*([-\d.]+)"""),
        "magZ"        to Regex("""Mag\s*:.*?Z\s*=\s*([-\d.]+)"""),
    )

    // ── Public API ─────────────────────────────────────────────────────────

    @Synchronized
    fun feedLine(line: String) {
        when {
            // Title line → start fresh collection
            line.contains("TELEMETRY DATA") -> {
                frameData.clear()
                inFrame = true
            }
            // Border line (+---) → just marks frame boundaries; never emit here
            line.startsWith("+---") -> {
                if (!inFrame) inFrame = true
                // intentionally no emit — rely on timeout only
            }
            // Data line inside frame
            inFrame -> {
                var matched = false
                for ((key, regex) in patterns) {
                    regex.find(line)?.groupValues?.getOrNull(1)
                        ?.toDoubleOrNull()
                        ?.let { frameData[key] = it; matched = true }
                }
                if (matched) scheduleTimeout()
            }
        }
    }

    fun reset() {
        cancelTimeout()
        frameData.clear()
        inFrame = false
    }

    fun shutdown() {
        reset()
        scheduler.shutdownNow()
    }

    // ── Internals ──────────────────────────────────────────────────────────

    private fun scheduleTimeout() {
        cancelTimeout()
        timeoutJob = scheduler.schedule({
            synchronized(this) {
                if (frameData.isNotEmpty()) emit()
            }
        }, FRAME_TIMEOUT_MS, TimeUnit.MILLISECONDS)
    }

    private fun cancelTimeout() {
        timeoutJob?.cancel(false)
        timeoutJob = null
    }

    private fun emit() {
        val t = TelemetryData(
            latitude    = frameData["latitude"],
            longitude   = frameData["longitude"],
            altitude    = frameData["altitude"],
            speed       = frameData["speed"],
            pressure    = frameData["pressure"],
            temperature = frameData["temperature"],
            voltage     = frameData["voltage"],
            current     = frameData["current"],
            doppler     = frameData["doppler"],
            accelX      = frameData["accelX"],
            accelY      = frameData["accelY"],
            accelZ      = frameData["accelZ"],
            gyroX       = frameData["gyroX"],
            gyroY       = frameData["gyroY"],
            gyroZ       = frameData["gyroZ"],
            magX        = frameData["magX"],
            magY        = frameData["magY"],
            magZ        = frameData["magZ"],
        )
        frameData.clear()
        inFrame = false
        onComplete(t)
    }
}
