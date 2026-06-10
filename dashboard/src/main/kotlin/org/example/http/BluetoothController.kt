package org.example.http

import org.example.http.dto.ConnectRequest
import org.example.service.AuditAction
import org.example.service.AuditService
import org.example.service.BluetoothService
import org.example.service.ComPortService
import org.springframework.http.ResponseEntity
import org.springframework.security.access.prepost.PreAuthorize
import org.springframework.web.bind.annotation.*

@RestController
@RequestMapping("/api/bluetooth")
class BluetoothController(
    private val bluetoothService: BluetoothService,
    private val comPortService: ComPortService,
    private val auditService: AuditService
) {

    /** Scan for paired Bluetooth devices and Bluetooth COM ports. */
    @GetMapping("/scan")
    @PreAuthorize("hasAuthority('dashboard:read')")
    fun scan(): ResponseEntity<Any> =
        ResponseEntity.ok(bluetoothService.scan())

    /** Connect via the COM port that Windows created after pairing. */
    @PostMapping("/connect")
    @PreAuthorize("hasAuthority('commands:write')")
    fun connect(@RequestBody body: ConnectRequest): ResponseEntity<Any> {
        val success = comPortService.connect(body.port, body.baudRate)
        auditService.record(
            AuditAction.COM_CONNECT, target = "BT ${body.port}",
            details = "baud=${body.baudRate}", success = success
        )
        return if (success)
            ResponseEntity.ok(mapOf("success" to true, "message" to "Bluetooth ligado em ${body.port}"))
        else
            ResponseEntity.badRequest().body(mapOf("success" to false, "message" to "Não foi possível abrir ${body.port}"))
    }

    /** Disconnect from the Bluetooth COM port. */
    @PostMapping("/disconnect")
    @PreAuthorize("hasAuthority('commands:write')")
    fun disconnect(): ResponseEntity<Any> {
        comPortService.disconnect()
        auditService.record(AuditAction.COM_DISCONNECT, target = "BT")
        return ResponseEntity.ok(mapOf("success" to true))
    }

    /** Current connection status. */
    @GetMapping("/status")
    @PreAuthorize("hasAuthority('dashboard:read')")
    fun status(): ResponseEntity<Any> =
        ResponseEntity.ok(comPortService.getStatus())

    /** Send a text message to the Pico via Bluetooth. */
    @PostMapping("/send")
    @PreAuthorize("hasAuthority('commands:write')")
    fun send(@RequestBody body: Map<String, String>): ResponseEntity<Any> {
        val text = body["text"]
            ?: return ResponseEntity.badRequest().body(mapOf("success" to false, "message" to "Campo 'text' em falta"))
        val success = comPortService.sendText(text)
        auditService.record(AuditAction.COMMAND_SEND, target = "BT (texto)", details = text, success = success)
        return if (success)
            ResponseEntity.ok(mapOf("success" to true))
        else
            ResponseEntity.badRequest().body(mapOf("success" to false, "message" to "Não está ligado"))
    }
}
