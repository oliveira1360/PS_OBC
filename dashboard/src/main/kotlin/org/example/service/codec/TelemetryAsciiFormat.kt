package org.example.service.codec

import com.fasterxml.jackson.databind.ObjectMapper
import org.example.domain.TelemetryData

/**
 * Representação ASCII canónica da telemetria: uma linha "TELEM:{...}" em JSON.
 *
 * É o ponto ÚNICO de canonicalização do sistema. Qualquer codec — binário ou
 * texto — passa o seu [TelemetryData] por aqui para produzir exatamente a mesma
 * string ASCII, garantindo que o que é guardado/difundido é independente do
 * formato de chegada (requisito: "guardar na BD será sempre em ASCII").
 *
 * As chaves coincidem com as lidas pelo [AsciiTelemetryCodec], pelo que o
 * resultado faz round-trip: encode(decode(x)) reproduz a mesma telemetria.
 */
object TelemetryAsciiFormat {

    const val PREFIX = "TELEM:"

    /** Serializa [t] como "TELEM:{...}". Só inclui campos não-nulos. */
    fun encode(t: TelemetryData, mapper: ObjectMapper): String {
        val m = LinkedHashMap<String, Any>()
        t.voltage?.let        { m["volt"] = round(it) }
        t.current?.let        { m["curr"] = round(it) }
        t.batteryLevel?.let   { m["bat"]  = it }
        t.temperature?.let    { m["temp"] = round(it) }
        t.pressure?.let       { m["pres"] = round(it) }
        t.latitude?.let       { m["lat"]  = round(it) }
        t.longitude?.let      { m["lon"]  = round(it) }
        t.altitude?.let       { m["alt"]  = round(it) }
        t.speed?.let          { m["spd"]  = round(it) }
        t.doppler?.let        { m["dop"]  = round(it) }
        t.accelX?.let         { m["ax"]   = round(it) }
        t.accelY?.let         { m["ay"]   = round(it) }
        t.accelZ?.let         { m["az"]   = round(it) }
        t.gyroX?.let          { m["gx"]   = round(it) }
        t.gyroY?.let          { m["gy"]   = round(it) }
        t.gyroZ?.let          { m["gz"]   = round(it) }
        t.magX?.let           { m["mx"]   = round(it) }
        t.magY?.let           { m["my"]   = round(it) }
        t.magZ?.let           { m["mz"]   = round(it) }
        t.rssi?.let           { m["rssi"] = it }
        t.signalStrength?.let { m["signal"] = it }
        return PREFIX + mapper.writeValueAsString(m)
    }

    /** Arredonda a 3 casas para manter a linha compacta e estável. */
    private fun round(v: Double): Double = Math.round(v * 1000.0) / 1000.0
}
