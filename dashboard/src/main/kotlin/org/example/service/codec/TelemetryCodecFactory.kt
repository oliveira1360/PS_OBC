package org.example.service.codec

import com.fasterxml.jackson.databind.ObjectMapper
import org.example.domain.telemetry.TelemetryFormat
import org.springframework.stereotype.Component

/**
 * Cria o [TelemetryCodec] adequado ao formato escolhido no frontend.
 *
 * Único ponto onde os formatos concretos são instanciados — adicionar um novo
 * protocolo resume-se a uma nova implementação de [TelemetryCodec] e um ramo aqui.
 */
@Component
class TelemetryCodecFactory(
    private val mapper: ObjectMapper
) {
    fun create(format: TelemetryFormat, sink: TelemetrySink): TelemetryCodec =
        when (format) {
            TelemetryFormat.RAW   -> RawBinaryTelemetryCodec(sink, mapper)
            TelemetryFormat.ASCII -> AsciiTelemetryCodec(sink, mapper)
            TelemetryFormat.AUTO  -> AutoTelemetryCodec(sink, mapper)
        }
}
