package org.example.service.codec

import org.example.domain.TelemetryData
import org.example.domain.telemetry.TelemetryFormat

/**
 * Contrato de descodificação de telemetria vinda da UART.
 *
 * Cada implementação é dona do seu próprio *framing* (binário de comprimento
 * fixo, ou texto delimitado por '\n') e da conversão para o modelo canónico
 * [TelemetryData]. Os bytes crus entram por [feed]; os resultados saem pelo
 * [TelemetrySink] fornecido na construção. Isto mantém o [org.example.service.ComPortService]
 * fino: ele só lê bytes e escolhe o codec — toda a lógica de protocolo vive aqui.
 *
 * Padrão "interface + implementações": esta interface enumera as possibilidades
 * (RAW / ASCII / AUTO) e cada classe concreta implementa o seu protocolo.
 */
interface TelemetryCodec {

    /** Formato que este codec descodifica. */
    val format: TelemetryFormat

    /**
     * Alimenta [length] bytes lidos da porta série. A implementação acumula
     * internamente e invoca o sink para cada frame completo. Não bloqueia à
     * espera de mais dados — guarda o estado parcial entre chamadas.
     */
    fun feed(data: ByteArray, length: Int)

    /** Limpa o estado de framing (chamado ao ligar/desligar a porta). */
    fun reset()

    /**
     * Liberta recursos (threads/schedulers). Chamado quando o codec é
     * substituído ou a porta é fechada. Default vazio para codecs sem estado
     * assíncrono. Um codec fechado não deve voltar a ser usado.
     */
    fun close() {}
}

/**
 * Recetor dos resultados de um [TelemetryCodec].
 *
 * É a fronteira entre "como descodificar" (codec) e "o que fazer com o
 * resultado" (guardar na BD, difundir por WebSocket, registar logs).
 */
interface TelemetrySink {

    /**
     * Um frame de telemetria completo.
     *
     * @param data          o modelo canónico já descodificado.
     * @param canonicalAscii a serialização ASCII canónica (linha "TELEM:{...}").
     *                       É esta a forma que é, conceptualmente, persistida —
     *                       seja a origem RAW ou ASCII, o resultado é idêntico.
     */
    fun onTelemetry(data: TelemetryData, canonicalAscii: String)

    /**
     * Uma linha de texto imprimível que NÃO é telemetria (resposta a comando,
     * erro, aviso…). Só os codecs de texto a emitem.
     *
     * @param arrivedAt instante (epoch ms) em que a linha foi detetada.
     */
    fun onLogLine(line: String, arrivedAt: Long)

    /**
     * Estado de subsistemas embebido no frame ASCII (campos "power"/"comms").
     * Default vazio — só o codec ASCII o usa quando esses campos existem.
     */
    fun onSubsystemStatus(powerOk: Boolean?, commsOk: Boolean?) {}
}
