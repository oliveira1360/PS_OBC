package org.example.service

import com.fazecast.jSerialComm.SerialPort
import com.fasterxml.jackson.databind.ObjectMapper
import org.example.model.*
import org.example.websocket.SatelliteWebSocketHandler
import org.springframework.stereotype.Service
import java.io.InputStream
import java.util.concurrent.CopyOnWriteArrayList
import java.util.concurrent.Executors
import java.util.concurrent.Future

@Service
class ComPortService(
    private val wsHandler: SatelliteWebSocketHandler,
    private val objectMapper: ObjectMapper
) {

    private var serialPort: SerialPort? = null
    private var readerFuture: Future<*>? = null
    private val executor = Executors.newSingleThreadExecutor()
    private val logs = CopyOnWriteArrayList<SatelliteLog>()
    private var latestTelemetry: TelemetryData = TelemetryData()
    private var status: SatelliteStatus = SatelliteStatus()
    private val maxLogs = 500

    fun getAvailablePorts(): List<String> {
        return SerialPort.getCommPorts().map { it.systemPortName }
    }

    fun connect(portName: String, baudRate: Int = 9600): Boolean {
        if (serialPort?.isOpen == true) disconnect()

        val port = SerialPort.getCommPorts().firstOrNull { it.systemPortName == portName }
            ?: return false

        port.baudRate = baudRate
        port.numDataBits = 8
        port.numStopBits = SerialPort.ONE_STOP_BIT
        port.parity = SerialPort.NO_PARITY
        port.setComPortTimeouts(SerialPort.TIMEOUT_READ_SEMI_BLOCKING, 100, 0)

        return if (port.openPort()) {
            serialPort = port
            status = status.copy(connected = true, comPort = portName, baudRate = baudRate)
            addLog(SatelliteLog(message = "Connected to $portName @ ${baudRate} baud", type = LogType.SYSTEM))
            broadcastStatus()
            startReading(port.inputStream)
            true
        } else {
            false
        }
    }

    fun disconnect() {
        readerFuture?.cancel(true)
        serialPort?.closePort()
        serialPort = null
        status = status.copy(connected = false, comPort = null)
        addLog(SatelliteLog(message = "Disconnected from COM port", type = LogType.SYSTEM))
        broadcastStatus()
    }

    fun isConnected(): Boolean = serialPort?.isOpen == true

    fun getStatus(): SatelliteStatus = status

    fun getLatestTelemetry(): TelemetryData = latestTelemetry

    fun getLogs(): List<SatelliteLog> = logs.takeLast(200)

    fun sendBytes(bytes: ByteArray): Boolean {
        val port = serialPort ?: return false
        if (!port.isOpen) return false
        return port.outputStream.let {
            it.write(bytes)
            it.flush()
            true
        }
    }

    private fun startReading(inputStream: InputStream) {
        readerFuture = executor.submit {
            val buffer = StringBuilder()
            val byteBuffer = ByteArray(1024)
            try {
                while (!Thread.currentThread().isInterrupted && serialPort?.isOpen == true) {
                    val bytesRead = inputStream.read(byteBuffer)
                    if (bytesRead > 0) {
                        val chunk = String(byteBuffer, 0, bytesRead)
                        buffer.append(chunk)
                        // Process complete lines
                        while (buffer.contains('\n')) {
                            val newlineIdx = buffer.indexOf('\n')
                            val line = buffer.substring(0, newlineIdx).trim()
                            buffer.delete(0, newlineIdx + 1)
                            if (line.isNotEmpty()) {
                                processIncomingLine(line)
                            }
                        }
                    }
                }
            } catch (e: InterruptedException) {
                Thread.currentThread().interrupt()
            } catch (e: Exception) {
                if (serialPort?.isOpen == true) {
                    addLog(SatelliteLog(message = "Read error: ${e.message}", type = LogType.ERROR))
                    broadcastLog(logs.last())
                }
            }
        }
    }

    private fun processIncomingLine(line: String) {
        val rawHex = line.toByteArray().joinToString(" ") { "%02X".format(it) }
        // Try to parse as JSON telemetry
        try {
            val map = objectMapper.readValue(line, Map::class.java)
            val telemetry = TelemetryData(
                temperature = (map["temp"] as? Number)?.toDouble() ?: (map["temperature"] as? Number)?.toDouble(),
                voltage = (map["volt"] as? Number)?.toDouble() ?: (map["voltage"] as? Number)?.toDouble(),
                current = (map["curr"] as? Number)?.toDouble() ?: (map["current"] as? Number)?.toDouble(),
                batteryLevel = (map["bat"] as? Number)?.toInt() ?: (map["battery"] as? Number)?.toInt(),
                altitude = (map["alt"] as? Number)?.toDouble() ?: (map["altitude"] as? Number)?.toDouble(),
                latitude = (map["lat"] as? Number)?.toDouble(),
                longitude = (map["lon"] as? Number)?.toDouble() ?: (map["lng"] as? Number)?.toDouble(),
                signalStrength = (map["rssi"] as? Number)?.toInt() ?: (map["signal"] as? Number)?.toInt()
            )
            latestTelemetry = telemetry
            wsHandler.broadcast(WsMessage("TELEMETRY", telemetry))

            // Update subsystem status if present
            val powerOk = map["power"] as? Boolean
            val commsOk = map["comms"] as? Boolean
            if (powerOk != null || commsOk != null) {
                status = status.copy(
                    power = if (powerOk == true) SubsystemStatus.OK else SubsystemStatus.WARNING,
                    comms = if (commsOk == true) SubsystemStatus.OK else SubsystemStatus.WARNING
                )
                broadcastStatus()
            }
        } catch (e: Exception) {
            // Not JSON — treat as raw log/response
            val log = SatelliteLog(
                message = line,
                type = if (line.startsWith("ERR") || line.startsWith("ERROR")) LogType.ERROR
                       else if (line.startsWith("WARN")) LogType.WARNING
                       else LogType.RESPONSE,
                raw = rawHex
            )
            addLog(log)
            broadcastLog(log)
        }
    }

    private fun addLog(log: SatelliteLog) {
        logs.add(log)
        if (logs.size > maxLogs) logs.removeAt(0)
    }

    fun addLogAndBroadcast(log: SatelliteLog) {
        addLog(log)
        broadcastLog(log)
    }

    private fun broadcastLog(log: SatelliteLog) {
        wsHandler.broadcast(WsMessage("LOG", log))
    }

    private fun broadcastStatus() {
        wsHandler.broadcast(WsMessage("STATUS", status))
    }
}
