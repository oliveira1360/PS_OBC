package org.example.service

import com.fazecast.jSerialComm.SerialPort
import org.example.domain.LogType
import org.example.domain.OtaStatus
import org.example.domain.SatelliteLog
import org.example.domain.WsMessage
import org.example.domain.WsMessageType
import org.example.repository.OtaRepository
import org.example.websocket.SatelliteWebSocketHandler
import org.springframework.stereotype.Service
import org.springframework.web.multipart.MultipartFile
import java.util.zip.CRC32

/**
 * OTA firmware update service.
 *
 * Supports two file formats:
 *   • .bin  — raw binary. Sent directly via the Pico (COM6, pure pass-through to OBC TTC UART).
 *             Protocol: CMD_START_OTA → N×134-byte packets [0xAA][0x55][seq(2BE)][len(2BE)][128B] → END packet
 *   • .hex  — Intel HEX, sent line-by-line (legacy / MPLAB bootloader direct flash)
 *
 * Transports:
 *   • COM_PORT   — any available COM port on the system
 *   • BLUETOOTH  — Bluetooth SPP adapter (virtual COM port)
 */
@Service
class OtaService(
    private val otaRepository: OtaRepository,
    private val comPortService: ComPortService,
    private val wsHandler: SatelliteWebSocketHandler
) {

    companion object {
        const val OTA_PKT_PAYLOAD  = 128          // firmware bytes per packet
        const val OTA_PKT_TOTAL    = 134          // 6-byte header + 128-byte payload
        const val OTA_CMD_DELAY    = 600L         // ms após CMD_START_OTA (OBC prepara frame scanner)
        const val HEX_RECORD_DELAY = 20L
        const val ACK_TIMEOUT_MS   = 3000L        // ms à espera de ACK/NACK por pacote
        const val MAX_RETRIES      = 3            // tentativas antes de abortar a transferência

        // Códigos do protocolo OTA (devem coincidir com ttc.c)
        const val OTA_ACK_CODE  = 0xAC
        const val OTA_NACK_CODE = 0x4E

        // CMD_START_OTA = 0x10  (ttc.h)
        val CMD_START_OTA = byteArrayOf(0x10, 0x00, 0x00, 0x00)

        /**
         * 134-byte OTA data packet.
         * Wire: [0xAA][0x55] | seq(2B BE) | len(2B BE) | payload(128B, 0xFF-padded)
         */
        fun buildPacket(seq: Int, payload: ByteArray): ByteArray {
            val len = payload.size
            val pkt = ByteArray(OTA_PKT_TOTAL) { 0xFF.toByte() }
            pkt[0] = 0xAA.toByte()
            pkt[1] = 0x55.toByte()
            pkt[2] = (seq ushr 8).toByte();  pkt[3] = (seq and 0xFF).toByte()
            pkt[4] = (len ushr 8).toByte();  pkt[5] = (len and 0xFF).toByte()
            payload.copyInto(pkt, 6)
            return pkt
        }

        /**
         * 134-byte END packet (seq=0xFFFF).
         * Payload (12B): firmware_size(4BE) + crc32(4BE) + version(4BE)
         */
        fun buildEndPacket(size: Int, crc: Long, version: Long): ByteArray {
            val pkt = ByteArray(OTA_PKT_TOTAL) { 0xFF.toByte() }
            pkt[0] = 0xAA.toByte(); pkt[1] = 0x55.toByte()
            pkt[2] = 0xFF.toByte(); pkt[3] = 0xFF.toByte()  // seq = 0xFFFF
            pkt[4] = 0x00;          pkt[5] = 0x0C            // len = 12
            var i = 6
            for (shift in 24 downTo 0 step 8) pkt[i++] = (size    ushr shift).toByte()
            for (shift in 24 downTo 0 step 8) pkt[i++] = (crc     ushr shift).toByte()
            for (shift in 24 downTo 0 step 8) pkt[i++] = (version ushr shift).toByte()
            return pkt
        }
    }

    fun getStatus(): OtaStatus = otaRepository.getStatus()

    /**
     * Force-reset OTA state after a failed transfer.
     * Clears inProgress so a new upload can be started without restarting the server.
     */
    fun resetOta(): OtaStatus {
        val reset = OtaStatus(inProgress = false, success = false, message = "OTA reset pelo utilizador")
        otaRepository.save(reset)
        broadcast(reset)
        return reset
    }

    /**
     * Start an OTA transfer.
     *
     * @param file       The firmware file (.bin or .hex).
     * @param transport  "COM_PORT" or "BLUETOOTH".
     * @param otaPort    The COM port to use (e.g. "COM4").
     * @param baudRate   Baud rate for the OTA port (e.g. 115200).
     */
    fun sendFirmware(
        file: MultipartFile,
        transport: String,
        otaPort: String,
        baudRate: Int
    ): OtaStatus {
        if (otaRepository.getStatus().inProgress)
            return OtaStatus(inProgress = true, message = "OTA já está em progresso")

        val filename = file.originalFilename ?: "firmware"

        // ── Validate file type ──────────────────────────────────────────────────
        val isBin = filename.endsWith(".bin", ignoreCase = true)
        val isHex = filename.endsWith(".hex", ignoreCase = true)
        if (!isBin && !isHex)
            return OtaStatus(
                message = "Formato não suportado. Usa .bin (binário) ou .hex (Intel HEX).",
                success = false
            )

        val transportLabel = if (transport == "BLUETOOTH") "Bluetooth ($otaPort)" else otaPort


        val port = openOtaPort(otaPort, baudRate)
            ?: return OtaStatus(
                message = "Não foi possível abrir a porta $otaPort ($transport). " +
                          "Verifica se está disponível e não está em uso.",
                success = false
            )

        if (isBin) {
            val bytes   = file.bytes
            println("[KOTLIN] FW[0..7]: " + bytes.take(8).joinToString(" "){ "%02X".format(it) })
            println("[KOTLIN] FW[128..135]: " + bytes.drop(128).take(8).joinToString(" "){ "%02X".format(it) })
            // val crc     = CRC32().also { it.update(bytes) }.value
            val crc = calculateCustomCrc32(bytes)
            val version = System.currentTimeMillis() / 1000L and 0xFFFFFFFFL
            val chunks = bytes.toList().chunked(OTA_PKT_PAYLOAD).map { it.toByteArray() }
            val total   = chunks.size + 1   // data packets + END packet

            val initial = OtaStatus(
                inProgress   = true,
                totalRecords = total,
                message      = "OTA .bin via $transportLabel — ${bytes.size} B, " +
                               "crc32=0x%08X, ${chunks.size} pacotes".format(crc)
            )
            otaRepository.save(initial)
            broadcast(initial)
            comPortService.log(SatelliteLog(
                message = "OTA iniciado: $filename (${bytes.size} B, crc32=0x%08X) via $transportLabel".format(crc),
                type    = LogType.COMMAND
            ))

            Thread {
                try {
                    try {
                        port.setComPortTimeouts(SerialPort.TIMEOUT_NONBLOCKING, 0, 0)
                        val stale = ByteArray(256)
                        var drained = 0
                        repeat(5) {
                            try { drained += maxOf(0, port.inputStream.read(stale, 0, stale.size)) }
                            catch (_: Exception) { }
                        }
                        if (drained > 0) println("  [drain] $drained bytes stale descartados")
                    } catch (_: Exception) { }

                    // Configura timeout de leitura bloqueante para ACKs
                    port.setComPortTimeouts(SerialPort.TIMEOUT_READ_BLOCKING, ACK_TIMEOUT_MS.toInt(), 0)

                    // 1. CMD_START_OTA
                    println("Sending START: ${CMD_START_OTA.toHex()}")
                    port.outputStream.write(CMD_START_OTA)
                    port.outputStream.flush()

                    // Aguarda sinal READY do OBC [0x10,0xAC,0x4B,0xF7] após apagar flash
                    waitForOtaReady(port, 10000L)   // timeout 10 s (erase demora alguns segundos)

                    // 2. Data packets — aguarda ACK após cada um
                    chunks.forEachIndexed { index, chunk ->
                        sendWithAck(port, buildPacket(index, chunk), index,
                                    "chunk $index/${chunks.size - 1}")
                        val updated = OtaStatus(
                            inProgress   = true,
                            totalRecords = total,
                            sentRecords  = index + 1,
                            progress     = ((index + 1) * 100) / total,
                            message      = "Pacote ${index + 1}/${chunks.size} confirmado (ACK)"
                        )
                        otaRepository.save(updated)
                        if ((index + 1) % 10 == 0 || index == chunks.size - 1) broadcast(updated)
                    }

                    // 3. END packet (seq=0xFFFF) — também aguarda ACK
                    println("Sending END: size=${bytes.size}, crc=0x%08X".format(crc))
                    sendWithAck(port, buildEndPacket(bytes.size, crc, version), 0xFFFF, "END")

                    finishOta(port, filename, total, transportLabel)
                } catch (e: Exception) {
                    failOta(port, e.message)
                }
            }.start()

            return otaRepository.getStatus()
        }

        // ── .hex path (Intel HEX / bootloader direct flash) ────────────────────
        val records = file.inputStream.bufferedReader().readLines()
            .filter { it.startsWith(":") }
        if (records.isEmpty()) {
            port.closePort()
            return OtaStatus(
                message = "Nenhum registo HEX válido encontrado no ficheiro.",
                success = false
            )
        }
        val initial = OtaStatus(
            inProgress   = true,
            totalRecords = records.size,
            message      = "OTA .hex via $transportLabel — ${records.size} registos"
        )
        otaRepository.save(initial)
        broadcast(initial)
        comPortService.log(SatelliteLog(
            message = "OTA iniciado: $filename (${records.size} registos HEX) via $transportLabel",
            type    = LogType.COMMAND
        ))

        Thread {
            try {
                records.forEachIndexed { index, record ->
                    port.outputStream.write((record + "\r\n").toByteArray(Charsets.US_ASCII))
                    port.outputStream.flush()
                    val updated = OtaStatus(
                        inProgress   = true,
                        totalRecords = records.size,
                        sentRecords  = index + 1,
                        progress     = ((index + 1) * 100) / records.size,
                        message      = "A enviar registo ${index + 1}/${records.size}…"
                    )
                    otaRepository.save(updated)
                    if ((index + 1) % 10 == 0 || index == records.size - 1) broadcast(updated)
                    Thread.sleep(HEX_RECORD_DELAY)
                }
                finishOta(port, filename, records.size, transportLabel)
            } catch (e: Exception) {
                failOta(port, e.message)
            }
        }.start()

        return otaRepository.getStatus()
    }

    // ── Protocolo ACK/NACK ────────────────────────────────────────────────────

    /**
     * Resultado da leitura de um ACK/NACK do OBC.
     */
    private enum class AckResult { ACK, NACK, TIMEOUT, BAD_CHECKSUM, WRONG_SEQ, UNKNOWN }

    /**
     * Lê 4 bytes do OBC e interpreta como ACK ou NACK.
     *
     * Formato ACK:  [0xAC][seq_hi][seq_lo][XOR dos 3 anteriores]
     * Formato NACK: [0x4E][next_seq_hi][next_seq_lo][XOR dos 3 anteriores]
     *
     * @param expectedSeq  Seq do pacote enviado (0xFFFF para END packet).
     */
    private fun readAck(port: SerialPort, expectedSeq: Int): AckResult {
        val buf      = ByteArray(4)
        var received = 0
        val deadline = System.currentTimeMillis() + ACK_TIMEOUT_MS

        while (received < 4 && System.currentTimeMillis() < deadline) {
            val n = try { port.inputStream.read(buf, received, 4 - received) }
                    catch (_: Exception) { return AckResult.TIMEOUT }
            if (n > 0) received += n else Thread.sleep(1L)
        }
        if (received < 4) return AckResult.TIMEOUT


        println("  [readAck] Bytes recebidos para análise: ${buf.toHex()}")

        val code  = buf[0].toInt() and 0xFF
        val seqHi = buf[1].toInt() and 0xFF
        val seqLo = buf[2].toInt() and 0xFF
        val chk   = buf[3].toInt() and 0xFF
        val seq   = (seqHi shl 8) or seqLo

        if ((code xor seqHi xor seqLo) != chk) return AckResult.BAD_CHECKSUM

        return when (code) {
            OTA_ACK_CODE  -> if (seq == expectedSeq) AckResult.ACK else AckResult.WRONG_SEQ
            OTA_NACK_CODE -> AckResult.NACK
            else          -> AckResult.UNKNOWN
        }
    }

    /**
     * Envia [packet] e aguarda ACK do OBC. Retransmite em NACK ou timeout
     * até [MAX_RETRIES] vezes. Lança exceção se todas as tentativas falharem.
     *
     * @param expectedSeq  Seq que o OBC deve confirmar (para validação).
     * @param label        Descrição para logging.
     */
    private fun sendWithAck(port: SerialPort, packet: ByteArray, expectedSeq: Int, label: String) {
        repeat(MAX_RETRIES) { attempt ->
            println("Sending $label (tentativa ${attempt + 1}): ${packet.take(8).toByteArray().toHex()}…")
            port.outputStream.write(packet)

            when (val result = readAck(port, expectedSeq)) {
                AckResult.ACK -> {
                    println("  ACK OK — $label confirmado")
                    return   // sucesso — sai da função
                }
                AckResult.NACK -> {
                    println("  NACK recebido para $label — a retransmitir…")
                    comPortService.log(SatelliteLog(
                        message = "OTA NACK: $label (tentativa ${attempt + 1}/$MAX_RETRIES)",
                        type    = LogType.ERROR
                    ))
                    // continua para próxima iteração (retransmissão)
                }
                else -> {
                    println("  Resultado inesperado para $label: $result")
                    comPortService.log(SatelliteLog(
                        message = "OTA sem ACK válido para $label: $result (tentativa ${attempt + 1}/$MAX_RETRIES)",
                        type    = LogType.ERROR
                    ))
                }
            }
        }
        throw Exception("Sem ACK para $label após $MAX_RETRIES tentativas")
    }


    /**
     * Aguarda o sinal READY do OBC [0x10, 0xAC, 0x4B, 0xF7].
     * Enviado por ttc_send_ota_ready() depois do erase da flash terminar.
     */
    private fun waitForOtaReady(port: SerialPort, timeoutMs: Long) {
        println("  A aguardar que o satelite apague a Flash (timeout: ${timeoutMs / 1000}s)...")
        port.setComPortTimeouts(SerialPort.TIMEOUT_READ_BLOCKING, timeoutMs.toInt(), 0)

        val buf      = ByteArray(4)
        var received = 0

        // Usa try/catch externo para capturar SerialPortTimeoutException do jSerialComm
        try {
            while (received < 4) {
                val n = port.inputStream.read(buf, received, 4 - received)
                if (n > 0) received += n else break
            }
        } catch (_: Exception) { /* timeout ou erro — verificado abaixo */ }

        if (received < 4)
            throw Exception("Timeout a aguardar READY do OBC (${received}/4 bytes)")

        val valid = buf[0] == 0x10.toByte() &&
                    buf[1] == 0xAC.toByte() &&
                    buf[2] == 0x4B.toByte() &&
                    buf[3] == 0xF7.toByte()
        if (!valid)
            throw Exception("Sinal READY invalido do OBC: ${buf.joinToString(" ") { "%02X".format(it) }}")

        println("  [OK] Satelite enviou ACK! Flash limpa. A iniciar pacotes...")
        port.setComPortTimeouts(SerialPort.TIMEOUT_READ_BLOCKING, ACK_TIMEOUT_MS.toInt(), 0)
    }

    private fun openOtaPort(portName: String, baudRate: Int): SerialPort? {
        val port = SerialPort.getCommPorts().firstOrNull { it.systemPortName == portName } ?: return null
        port.baudRate    = baudRate
        port.numDataBits = 8
        port.numStopBits = SerialPort.ONE_STOP_BIT
        port.parity      = SerialPort.NO_PARITY
        port.setComPortTimeouts(SerialPort.TIMEOUT_WRITE_BLOCKING, 30000, 30000)
        if (!port.openPort()) return null
        Thread.sleep(300L)
        return port
    }

    private fun finishOta(port: SerialPort, filename: String, total: Int, transport: String) {
        port.closePort()
        val done = OtaStatus(
            inProgress   = false,
            success      = true,
            progress     = 100,
            totalRecords = total,
            sentRecords  = total,
            message      = "OTA concluido: $filename ($total unidades via $transport)"
        )
        otaRepository.save(done)
        broadcast(done)
        comPortService.log(SatelliteLog(message = "OTA concluido via $transport", type = LogType.SYSTEM))
    }

    private fun failOta(port: SerialPort, error: String?) {
        port.closePort()
        val err = OtaStatus(success = false, message = "Erro OTA: $error")
        otaRepository.save(err)
        broadcast(err)
        comPortService.log(SatelliteLog(message = "Erro OTA: $error", type = LogType.ERROR))
    }

    private fun broadcast(status: OtaStatus) =
        wsHandler.broadcast(WsMessage(WsMessageType.OTA_PROGRESS, status))

    private fun ByteArray.toHex() = joinToString(" ") { "%02X".format(it) }

    /**
     * Cálculo de CRC32 100% manual e idêntico à implementação em C na placa.
     * Substitui a biblioteca java.util.zip.CRC32 para garantir paridade absoluta.
     */
        fun calculateCustomCrc32(bytes: ByteArray): Long {
        var crc = 0xFFFFFFFFL
        val poly = 0xEDB88320L
        for (b in bytes) {
            crc = crc xor (b.toLong() and 0xFFL)
            for (i in 0 until 8) {
                crc = if ((crc and 1L) != 0L) (crc ushr 1) xor poly else crc ushr 1
            }
        }
        return (crc xor 0xFFFFFFFFL) and 0xFFFFFFFFL
    }
}
