package org.example.service.codec

import com.fasterxml.jackson.databind.ObjectMapper
import org.example.domain.TelemetryData
import org.example.domain.telemetry.TelemetryFormat

/**
 * Descodifica o frame binário compacto emitido por `ttc_send_telemetry`
 * (firmware PS_OBC). Layout de 13 bytes, big-endian nos campos de 16 bits:
 *
 * ```
 *  [0]  0x20  marcador  (CMD_REQUEST_DATA)
 *  [1]  voltage * 10            -> volt = b/10
 *  [2]  current * 100           -> curr = b/100
 *  [3]  latitude  (parte int)
 *  [4]  latitude  (frac * 100)  -> lat = b3 + b4/100
 *  [5]  longitude (parte int)
 *  [6]  longitude (frac * 100)  -> lon = b5 + b6/100
 *  [7]  pressure  (high byte)
 *  [8]  pressure  (low  byte)   -> pres = (b7<<8)|b8
 *  [9]  temperature (high byte)
 *  [10] temperature (low  byte) -> temp = (b9<<8)|b10
 *  [11] estado do TT&C          (sem campo no modelo — ignorado)
 *  [12] checksum = XOR dos bytes [0..11]
 * ```
 *
 * O codec é uma máquina de estados byte-a-byte: caça o marcador, acumula 13
 * bytes e só emite se o checksum bater certo. Frames corrompidos disparam um
 * *resync* (procura o próximo marcador no buffer), tornando-o robusto a bytes
 * perdidos na UART. Esta auto-validação é o que permite usá-lo em modo AUTO
 * sem falsos positivos sobre tráfego ASCII.
 */
class RawBinaryTelemetryCodec(
    private val sink: TelemetrySink,
    private val mapper: ObjectMapper
) : TelemetryCodec {

    companion object {
        const val MARKER: Int = 0x20   // CMD_REQUEST_DATA
        // 0x20 + 18 floats big-endian + estado + checksum
        const val FRAME_LEN: Int = 75
    }

    override val format = TelemetryFormat.RAW

    private val frame = ByteArray(FRAME_LEN)
    private var idx = 0   // bytes já acumulados no frame atual

    override fun feed(data: ByteArray, length: Int) {
        var i = 0
        while (i < length) {
            val b = data[i].toInt() and 0xFF
            if (idx == 0) {
                // À procura do marcador de início de frame.
                if (b == MARKER) { frame[0] = b.toByte(); idx = 1 }
            } else {
                frame[idx++] = b.toByte()
                if (idx == FRAME_LEN) {
                    if (checksumOk()) { emit(); idx = 0 }
                    else resync()
                }
            }
            i++
        }
    }

    override fun reset() { idx = 0 }

    // ── Internos ────────────────────────────────────────────────────────────

    private fun checksumOk(): Boolean {
        var chk = 0
        for (j in 0 until FRAME_LEN - 1) chk = chk xor (frame[j].toInt() and 0xFF)
        return chk == (frame[FRAME_LEN - 1].toInt() and 0xFF)
    }

    /**
     * Checksum falhou: o frame está desalinhado (provável byte perdido/ruído).
     * Procura o próximo marcador dentro do buffer e desloca o resto para o
     * início, para não descartar um frame válido que tenha começado a meio.
     */
    private fun resync() {
        for (p in 1 until FRAME_LEN) {
            if ((frame[p].toInt() and 0xFF) == MARKER) {
                val remaining = FRAME_LEN - p
                System.arraycopy(frame, p, frame, 0, remaining)
                idx = remaining
                return
            }
        }
        idx = 0
    }

    /** Lê um float big-endian a partir do offset [i] do frame. */
    private fun f(i: Int): Double {
        val bits = ((frame[i].toInt()     and 0xFF) shl 24) or
                   ((frame[i + 1].toInt() and 0xFF) shl 16) or
                   ((frame[i + 2].toInt() and 0xFF) shl 8)  or
                    (frame[i + 3].toInt() and 0xFF)
        return Float.fromBits(bits).toDouble()
    }

    private fun emit() {
        // Layout: [0]=0x20, depois 18 floats BE, [73]=estado, [74]=checksum.
        val t = TelemetryData(
            timestamp   = System.currentTimeMillis(),
            voltage     = f(1),
            current     = f(5),
            latitude    = f(9),
            longitude   = f(13),
            altitude    = f(17),
            speed       = f(21),
            accelX      = f(25),
            accelY      = f(29),
            accelZ      = f(33),
            gyroX       = f(37),
            gyroY       = f(41),
            gyroZ       = f(45),
            magX        = f(49),
            magY        = f(53),
            magZ        = f(57),
            pressure    = f(61),
            temperature = f(65),
            doppler     = f(69),
            // frame[73] = estado do TT&C — sem campo correspondente em TelemetryData.
        )
        sink.onTelemetry(t, TelemetryAsciiFormat.encode(t, mapper))
    }
}
