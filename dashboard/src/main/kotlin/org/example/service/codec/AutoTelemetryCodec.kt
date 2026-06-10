package org.example.service.codec

import com.fasterxml.jackson.databind.ObjectMapper
import org.example.domain.telemetry.TelemetryFormat

/**
 * Deteção automática de formato: alimenta o MESMO fluxo de bytes aos dois
 * descodificadores em paralelo.
 *
 * Funciona sem ambiguidade porque ambos se auto-validam:
 *  - o [RawBinaryTelemetryCodec] só emite com checksum correto, logo tráfego
 *    ASCII nunca produz um frame binário falso;
 *  - o [AsciiTelemetryCodec] descarta linhas com excesso de bytes não-imprimíveis,
 *    logo um frame binário nunca produz um log/telemetria de texto falso.
 *
 * Os logs de evento e o estado de subsistemas só podem vir do lado ASCII, por
 * isso o lado RAW recebe o sink "puro" (a sua única saída é onTelemetry).
 */
class AutoTelemetryCodec(
    sink: TelemetrySink,
    mapper: ObjectMapper
) : TelemetryCodec {

    override val format = TelemetryFormat.AUTO

    private val raw   = RawBinaryTelemetryCodec(sink, mapper)
    private val ascii = AsciiTelemetryCodec(sink, mapper)

    override fun feed(data: ByteArray, length: Int) {
        raw.feed(data, length)
        ascii.feed(data, length)
    }

    override fun reset() {
        raw.reset()
        ascii.reset()
    }

    override fun close() {
        raw.close()
        ascii.close()
    }
}
