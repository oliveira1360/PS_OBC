package org.example.service

import com.fazecast.jSerialComm.SerialPort
import org.springframework.stereotype.Service

data class BluetoothDevice(val name: String, val status: String)
data class BluetoothScanResult(
    val pairedDevices: List<BluetoothDevice>,
    val bluetoothComPorts: List<String>,
    val allComPorts: List<String>,
    val error: String? = null
)

/**
 * Scans for Bluetooth devices and COM ports on Windows using PowerShell.
 *
 * HC-05 / HC-06 workflow:
 *   1. Pair the device in Windows Bluetooth settings (PIN: 1234 or 0000).
 *   2. After pairing, Windows creates one or two COM ports
 *      (visible under Ports (COM & LPT) in Device Manager).
 *   3. These ports appear in [allComPorts] and are usable by jSerialComm.
 */
@Service
class BluetoothService {

    /** Returns all available information about Bluetooth devices and COM ports. */
    fun scan(): BluetoothScanResult {
        val pairedDevices  = scanPairedDevices()
        val btComPorts     = scanBluetoothComPorts()
        val allPorts       = SerialPort.getCommPorts().map { it.systemPortName }
        return BluetoothScanResult(
            pairedDevices    = pairedDevices,
            bluetoothComPorts = btComPorts,
            allComPorts      = allPorts
        )
    }

    // ── Private helpers ────────────────────────────────────────────────────────

    /**
     * Lists paired Bluetooth devices via PowerShell Get-PnpDevice.
     * Returns empty list if PowerShell is unavailable or fails.
     */
    private fun scanPairedDevices(): List<BluetoothDevice> = try {
        val output = runPowerShell(
            """
            Get-PnpDevice -Class Bluetooth -ErrorAction SilentlyContinue |
            Where-Object { ${'$'}_.Status -eq 'OK' -or ${'$'}_.Status -eq 'Unknown' } |
            Select-Object FriendlyName, Status |
            ForEach-Object { "${'$'}(${'$'}_.FriendlyName)|${'$'}(${'$'}_.Status)" }
            """.trimIndent()
        )
        output.lines()
            .filter { it.contains("|") }
            .map { line ->
                val parts = line.split("|")
                BluetoothDevice(
                    name   = parts.getOrElse(0) { "Unknown" }.trim(),
                    status = parts.getOrElse(1) { "Unknown" }.trim()
                )
            }
            .filter { it.name.isNotBlank() && it.name != "Unknown" }
    } catch (_: Exception) { emptyList() }

    /**
     * Lists COM ports whose description contains "Bluetooth".
     * These are the ports created after pairing an SPP device (e.g. HC-05).
     */
    private fun scanBluetoothComPorts(): List<String> = try {
        val output = runPowerShell(
            """
            Get-CimInstance Win32_SerialPort -ErrorAction SilentlyContinue |
            Where-Object { ${'$'}_.Description -match 'Bluetooth|BT' } |
            ForEach-Object { ${'$'}_.DeviceID }
            """.trimIndent()
        )
        output.lines().map { it.trim() }.filter { it.matches(Regex("COM\\d+")) }
    } catch (_: Exception) { emptyList() }

    private fun runPowerShell(script: String): String {
        val process = ProcessBuilder(
            "powershell", "-NoProfile", "-NonInteractive",
            "-ExecutionPolicy", "Bypass", "-Command", script
        )
            .redirectErrorStream(true)
            .start()

        val output = process.inputStream.bufferedReader().readText()
        process.waitFor()
        return output
    }
}
