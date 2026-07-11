package org.example.service

import com.fazecast.jSerialComm.SerialPort
import org.example.domain.*
import org.example.domain.telemetry.TelemetryFormat
import org.example.repository.LogRepository
import org.example.repository.StatusRepository
import org.example.repository.TelemetryRepository
import org.example.service.codec.TelemetryCodec
import org.example.service.codec.TelemetryCodecFactory
import org.example.service.codec.TelemetrySink
import org.example.websocket.SatelliteWebSocketHandler
import jakarta.annotation.PreDestroy
import org.slf4j.LoggerFactory
import org.springframework.stereotype.Service
import java.util.concurrent.Executors
import java.util.concurrent.Future
import java.util.concurrent.LinkedBlockingQueue
import java.util.concurrent.TimeUnit

/**
 * Serial transport. Kept thin on purpose: it reads raw bytes from the COM port
 * and feeds them to the [TelemetryCodec] chosen for the connection (RAW binary /
 * ASCII text / AUTO). All protocol/parsing lives in the codecs; this class is
 * their [TelemetrySink] — it merges, stores, persists and broadcasts results.
 */
@Service
class ComPortService(
    private val statusRepository: StatusRepository,
    private val telemetryRepository: TelemetryRepository,
    private val logRepository: LogRepository,
    private val wsHandler: SatelliteWebSocketHandler,
    private val persistence: TelemetryPersistenceService,
    private val codecFactory: TelemetryCodecFactory
) : TelemetrySink {

    private val log = LoggerFactory.getLogger(javaClass)

    private var serialPort: SerialPort? = null
    private var readerFuture: Future<*>? = null
    private var processorFuture: Future<*>? = null
    private var codec: TelemetryCodec? = null

    /** Reader enqueues raw chunks; processor feeds the codec (keeps the reader lean). */
    private val byteQueue = LinkedBlockingQueue<ByteArray>()
    private val readerExecutor    = Executors.newSingleThreadExecutor()
    private val processorExecutor = Executors.newSingleThreadExecutor()

    /**
     * Telemetry throttle: the source can flood much faster than is useful
     * (dozens of frames/s). We keep the freshest value in memory always, but
     * only broadcast/persist at most once per [telemetryMinIntervalMs] — keeps
     * the WebSocket and DB sane and the live view smooth.
     */
    @Volatile private var lastEmitMs = 0L
    private var framesSinceEmit = 0
    private val telemetryMinIntervalMs = 1000L

    /** Last successfully parsed telemetry — fills gaps in future partial frames. */
    @Volatile private var cachedTelemetry: TelemetryData? = null

    /** Deduplication: skip log entries identical to the previous one within 300 ms. */
    private var lastLogMessage = ""
    private var lastLogTimeMs  = 0L

    // ── Public API ─────────────────────────────────────────────────────────────

    fun getAvailablePorts(): List<String> =
        SerialPort.getCommPorts().map { it.systemPortName }

    fun getStatus()          = statusRepository.get()
    fun getLatestTelemetry() = telemetryRepository.getLatest()
    fun getLogs()            = logRepository.findRecent()
    fun isConnected(): Boolean = serialPort?.isOpen == true

    fun connect(
        portName: String,
        baudRate: Int = 9600,
        format: TelemetryFormat = TelemetryFormat.AUTO
    ): Boolean {
        if (serialPort?.isOpen == true) disconnect()

        val port = SerialPort.getCommPorts().firstOrNull { it.systemPortName == portName }
            ?: return false

        port.baudRate = baudRate
        port.numDataBits = 8
        port.numStopBits = SerialPort.ONE_STOP_BIT
        port.parity = SerialPort.NO_PARITY
        // SEMI_BLOCKING: return as soon as ≥1 byte is available (don't wait to
        // fill the whole 256-byte buffer). Otherwise a slow stream is delivered
        // in 256-byte bursts (~one burst per minute), which looks like buffering.
        port.setComPortTimeouts(SerialPort.TIMEOUT_READ_SEMI_BLOCKING, 0, 0)

        return if (port.openPort()) {
            serialPort = port
            codec?.close()
            codec = codecFactory.create(format, this)
            cachedTelemetry = null
            byteQueue.clear()
            statusRepository.save(statusRepository.get().copy(
                connected = true, comPort = portName, baudRate = baudRate
            ))
            log(SatelliteLog(message = "Ligado a $portName @ $baudRate baud (${format.name})", type = LogType.SYSTEM))
            broadcastStatus()
            startProcessor()
            startReader(port)
            true
        } else false
    }

    fun disconnect() {
        readerFuture?.cancel(true)
        processorFuture?.cancel(true)
        serialPort?.closePort()
        serialPort = null
        codec?.close()
        codec = null
        byteQueue.clear()
        statusRepository.save(statusRepository.get().copy(connected = false, comPort = null))
        log(SatelliteLog(message = "Desligado da porta COM", type = LogType.SYSTEM))
        broadcastStatus()
    }

    fun sendBytes(bytes: ByteArray): Boolean {
        val port = serialPort ?: return false
        if (!port.isOpen) return false
        port.outputStream.write(bytes)
        port.outputStream.flush()
        return true
    }

    fun sendText(text: String): Boolean {
        val line = if (text.endsWith("\n")) text else "$text\n"
        return sendBytes(line.toByteArray(Charsets.UTF_8))
    }

    fun log(entry: SatelliteLog) {
        logRepository.save(entry)
        wsHandler.broadcast(WsMessage(WsMessageType.LOG, entry))
    }

    /** Encerramento limpo do serviço: fecha a porta e termina as threads. */
    @PreDestroy
    fun shutdown() {
        if (serialPort?.isOpen == true) disconnect()
        readerExecutor.shutdownNow()
        processorExecutor.shutdownNow()
        log.info("ComPortService encerrado")
    }

    // ── Reader thread — lean: only reads raw bytes ─────────────────────────────

    private fun startReader(port: SerialPort) {
        readerFuture = readerExecutor.submit {
            val byteBuffer = ByteArray(256)
            val inputStream = port.inputStream

            while (!Thread.currentThread().isInterrupted) {
                if (!port.isOpen) break

                val bytesRead = try {
                    inputStream.read(byteBuffer)
                } catch (e: InterruptedException) {
                    Thread.currentThread().interrupt(); break
                } catch (e: Exception) {
                    if (!port.isOpen) break
                    log(SatelliteLog(message = "Erro de leitura COM: ${e.message}", type = LogType.ERROR))
                    break
                }

                if (bytesRead <= 0) continue
                byteQueue.offer(byteBuffer.copyOf(bytesRead))
            }
        }
    }

    // ── Processor thread — feeds the active codec ──────────────────────────────

    private fun startProcessor() {
        processorFuture = processorExecutor.submit {
            while (!Thread.currentThread().isInterrupted) {
                val chunk = try {
                    byteQueue.poll(5, TimeUnit.SECONDS)
                } catch (e: InterruptedException) {
                    Thread.currentThread().interrupt(); break
                } ?: continue
                codec?.feed(chunk, chunk.size)
            }
        }
    }

    // ── TelemetrySink — results from the codec ─────────────────────────────────

    override fun onTelemetry(data: TelemetryData, canonicalAscii: String) {
        // Keep the freshest value in memory on every frame (cheap).
        val merged = mergeTelemetry(data)
        cachedTelemetry = merged
        telemetryRepository.save(merged)

        // Throttle the expensive parts (WebSocket + DB) to a sane rate.
        framesSinceEmit++
        val now = System.currentTimeMillis()
        if (now - lastEmitMs >= telemetryMinIntervalMs) {
            val span = if (lastEmitMs == 0L) 0 else now - lastEmitMs
            log.debug("[TELEM] {} frames em {}ms  v={} t={} p={}",
                framesSinceEmit, span, merged.voltage, merged.temperature, merged.pressure)
            lastEmitMs = now
            framesSinceEmit = 0
            persistHistory(merged)
            wsHandler.broadcast(WsMessage(WsMessageType.TELEMETRY, merged))
        }
    }

    override fun onLogLine(line: String, arrivedAt: Long) {
        if (line == lastLogMessage && arrivedAt - lastLogTimeMs < 300L) return
        lastLogMessage = line
        lastLogTimeMs  = arrivedAt

        val type = when {
            line.startsWith("ERR",  ignoreCase = true) -> LogType.ERROR
            line.startsWith("WARN", ignoreCase = true) -> LogType.WARNING
            line.startsWith("CMD",  ignoreCase = true) -> LogType.COMMAND
            else                                        -> LogType.RESPONSE
        }
        log(SatelliteLog(timestamp = arrivedAt, message = line, type = type))
    }

    override fun onSubsystemStatus(powerOk: Boolean?, commsOk: Boolean?) {
        if (powerOk == null && commsOk == null) return
        var s = statusRepository.get()
        powerOk?.let { s = s.copy(power = if (it) SubsystemStatus.OK else SubsystemStatus.WARNING) }
        commsOk?.let { s = s.copy(comms = if (it) SubsystemStatus.OK else SubsystemStatus.WARNING) }
        statusRepository.save(s)
        broadcastStatus()
    }

    // ── Helpers ────────────────────────────────────────────────────────────────

    private fun mergeTelemetry(fresh: TelemetryData): TelemetryData {
        val c = cachedTelemetry ?: return fresh
        return fresh.copy(
            latitude       = fresh.latitude       ?: c.latitude,
            longitude      = fresh.longitude      ?: c.longitude,
            altitude       = fresh.altitude       ?: c.altitude,
            speed          = fresh.speed          ?: c.speed,
            accelX         = fresh.accelX         ?: c.accelX,
            accelY         = fresh.accelY         ?: c.accelY,
            accelZ         = fresh.accelZ         ?: c.accelZ,
            gyroX          = fresh.gyroX          ?: c.gyroX,
            gyroY          = fresh.gyroY          ?: c.gyroY,
            gyroZ          = fresh.gyroZ          ?: c.gyroZ,
            magX           = fresh.magX           ?: c.magX,
            magY           = fresh.magY           ?: c.magY,
            magZ           = fresh.magZ           ?: c.magZ,
            pressure       = fresh.pressure       ?: c.pressure,
            temperature    = fresh.temperature    ?: c.temperature,
            voltage        = fresh.voltage        ?: c.voltage,
            current        = fresh.current        ?: c.current,
            batteryLevel   = fresh.batteryLevel   ?: c.batteryLevel,
            doppler        = fresh.doppler        ?: c.doppler,
            rssi           = fresh.rssi           ?: c.rssi,
            signalStrength = fresh.signalStrength ?: c.signalStrength,
        )
    }

    private fun broadcastStatus() =
        wsHandler.broadcast(WsMessage(WsMessageType.STATUS, statusRepository.get()))

    /** Grava a telemetria na BD para o histórico. Nunca rebenta o fluxo ao vivo. */
    private fun persistHistory(t: TelemetryData) {
        try { persistence.persistData(t) }
        catch (e: Exception) {
            log(SatelliteLog(message = "Falha ao gravar histórico: ${e.message}", type = LogType.WARNING))
        }
    }
}
