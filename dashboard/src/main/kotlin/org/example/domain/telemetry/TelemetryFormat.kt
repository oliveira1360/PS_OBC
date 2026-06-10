package org.example.domain.telemetry

/**
 * Formato da telemetria recebida na porta série.
 *
 *  - [RAW]   : frame binário compacto do firmware (ttc_send_telemetry).
 *  - [ASCII] : telemetria em texto — linha "TELEM:{...}" (JSON) ou tabela "+---".
 *  - [AUTO]  : deteta automaticamente (alimenta ambos os descodificadores;
 *              o RAW valida por checksum e o ASCII ignora não-imprimíveis,
 *              por isso não há ambiguidade na prática).
 *
 * Independentemente do formato de chegada, a telemetria é SEMPRE canonicalizada
 * para ASCII (TELEM: JSON) antes de ser persistida — ver [TelemetryAsciiFormat].
 */
enum class TelemetryFormat {
    RAW,
    ASCII,
    AUTO;

    companion object {
        /** Conversão tolerante a maiúsculas/minúsculas; default AUTO se desconhecido. */
        fun from(value: String?): TelemetryFormat =
            entries.firstOrNull { it.name.equals(value?.trim(), ignoreCase = true) } ?: AUTO
    }
}
