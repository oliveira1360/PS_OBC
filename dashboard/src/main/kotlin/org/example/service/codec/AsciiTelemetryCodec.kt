package org.example.service.codec

import com.fasterxml.jackson.databind.ObjectMapper
import org.example.domain.TelemetryData
import org.example.domain.telemetry.TelemetryFormat
import org.example.service.TelemetryFrameParser

/**
 * Descodifica telemetria em texto recebida da UART:
 *
 *  1. Linha "TELEM:{...}" — JSON compacto (formato canónico do sistema).
 *  2. Tabela "+--- TELEMETRY DATA ---+" — frame multi-linha do firmware antigo,
 *     reconstruído pelo [TelemetryFrameParser] por janela de silêncio.
 *
 * Qualquer outra linha imprimível é encaminhada como log de evento. Linhas com
 * demasiados bytes não-imprimíveis (ex.: um frame binário mal-encaminhado para
 * este codec) são descartadas — é isto que torna o modo AUTO seguro.
 */
class AsciiTelemetryCodec(
    private val sink: TelemetrySink,
    private val mapper: ObjectMapper
) : TelemetryCodec {

    override val format = TelemetryFormat.ASCII

    private val lineBuffer = StringBuilder()

    /** Parser da tabela multi-linha (firmware antigo). Emite por timeout. */
    private val frameParser = TelemetryFrameParser { t ->
        sink.onTelemetry(t, TelemetryAsciiFormat.encode(t, mapper))
    }

    // ── Framing por linha ─────────────────────────────────────────────────────

    override fun feed(data: ByteArray, length: Int) {
        lineBuffer.append(String(data, 0, length, Charsets.ISO_8859_1))
        while (lineBuffer.contains('\n')) {
            val arrivedAt = System.currentTimeMillis()
            val nl = lineBuffer.indexOf('\n')
            val line = lineBuffer.substring(0, nl).trimEnd('\r').trim()
            lineBuffer.delete(0, nl + 1)
            if (line.isNotEmpty()) processLine(line, arrivedAt)
        }
    }

    override fun reset() {
        lineBuffer.setLength(0)
        frameParser.reset()
    }

    /** Encerra o scheduler interno do parser de tabela (evita fuga de threads). */
    override fun close() {
        frameParser.shutdown()
    }

    // ── Processamento de cada linha ───────────────────────────────────────────

    private fun processLine(line: String, arrivedAt: Long) {
        if (!isPrintableLine(line)) return

        // 1) JSON "TELEM:{...}" — formato canónico.
        if (line.startsWith(TelemetryAsciiFormat.PREFIX)) {
            if (tryDecodeJson(line, arrivedAt)) return
            // JSON malformado — cai para os tratamentos seguintes.
        }

        // 2) Tabela multi-linha (firmware antigo) — alimenta o parser por timeout.
        frameParser.feedLine(line)
        if (isFrameLine(line)) return

        // 3) Linha de evento (log).
        sink.onLogLine(line, arrivedAt)
    }

    private fun tryDecodeJson(line: String, arrivedAt: Long): Boolean {
        return try {
            val json = line.removePrefix(TelemetryAsciiFormat.PREFIX)
            @Suppress("UNCHECKED_CAST")
            val map = mapper.readValue(json, Map::class.java) as Map<String, Any?>

            val t = TelemetryData(
                timestamp      = arrivedAt,
                latitude       = num(map, "lat"),
                longitude      = num(map, "lon", "lng"),
                altitude       = num(map, "alt"),
                speed          = num(map, "spd", "speed"),
                accelX         = num(map, "ax"),
                accelY         = num(map, "ay"),
                accelZ         = num(map, "az"),
                gyroX          = num(map, "gx"),
                gyroY          = num(map, "gy"),
                gyroZ          = num(map, "gz"),
                magX           = num(map, "mx"),
                magY           = num(map, "my"),
                magZ           = num(map, "mz"),
                pressure       = num(map, "pres", "pressure"),
                temperature    = num(map, "temp", "temperature"),
                voltage        = num(map, "volt", "voltage"),
                current        = num(map, "curr", "current"),
                batteryLevel   = num(map, "bat", "battery")?.toInt(),
                doppler        = num(map, "dop", "doppler"),
                signalStrength = num(map, "rssi", "signal")?.toInt(),
            )

            // Reencode para a forma canónica estável (chaves/arredondamento uniformes).
            sink.onTelemetry(t, TelemetryAsciiFormat.encode(t, mapper))
            sink.onSubsystemStatus(map["power"] as? Boolean, map["comms"] as? Boolean)
            true
        } catch (_: Exception) {
            false
        }
    }

    // ── Helpers (portados do ComPortService) ───────────────────────────────────

    private fun isPrintableLine(line: String): Boolean {
        if (line.isEmpty()) return false
        val bad = line.count { it.code < 32 || it.code > 126 }
        return bad.toDouble() / line.length < 0.2
    }

    private fun isFrameLine(line: String): Boolean =
        line.startsWith("+") || line.startsWith("|") || line.contains("TELEMETRY DATA")

    private fun num(map: Map<String, Any?>, vararg keys: String): Double? {
        for (k in keys) { val v = map[k]; if (v is Number) return v.toDouble() }
        return null
    }
}
