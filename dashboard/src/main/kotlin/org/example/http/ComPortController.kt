package org.example.http

import org.example.http.dto.ConnectRequest
import org.example.http.dto.SendRawRequest
import org.example.service.AuditAction
import org.example.service.AuditService
import org.example.service.ComPortService
import org.example.domain.telemetry.TelemetryFormat
import org.springframework.http.ResponseEntity
import org.springframework.security.access.prepost.PreAuthorize
import org.springframework.web.bind.annotation.*

@RestController
@RequestMapping("/api/com")
class ComPortController(
    private val comPortService: ComPortService,
    private val auditService: AuditService
) {

    @GetMapping("/ports")
    @PreAuthorize("hasAuthority('dashboard:read')")
    fun listPorts() = ResponseEntity.ok(mapOf("ports" to comPortService.getAvailablePorts()))

    @PostMapping("/connect")
    @PreAuthorize("hasAuthority('commands:write')")
    fun connect(@RequestBody body: ConnectRequest): ResponseEntity<Any> {
        val format = TelemetryFormat.from(body.format)
        val success = comPortService.connect(body.port, body.baudRate, format)
        auditService.record(
            AuditAction.COM_CONNECT, target = body.port,
            details = "baud=${body.baudRate} format=${format.name}", success = success
        )
        return if (success)
            ResponseEntity.ok(mapOf("success" to true, "message" to "Ligado a ${body.port}"))
        else
            ResponseEntity.badRequest().body(mapOf("success" to false, "message" to "Não foi possível abrir ${body.port}"))
    }

    @PostMapping("/disconnect")
    @PreAuthorize("hasAuthority('commands:write')")
    fun disconnect(): ResponseEntity<Any> {
        comPortService.disconnect()
        auditService.record(AuditAction.COM_DISCONNECT)
        return ResponseEntity.ok(mapOf("success" to true))
    }

    @GetMapping("/status")
    @PreAuthorize("hasAuthority('dashboard:read')")
    fun status() = ResponseEntity.ok(comPortService.getStatus())

    @PostMapping("/send")
    @PreAuthorize("hasAuthority('commands:write')")
    fun sendRaw(@RequestBody body: SendRawRequest): ResponseEntity<Any> {
        val bytes = body.hex.chunked(2).map { it.toInt(16).toByte() }.toByteArray()
        val success = comPortService.sendBytes(bytes)
        auditService.record(AuditAction.COMMAND_SEND, target = "COM (raw)", details = body.hex, success = success)
        return if (success)
            ResponseEntity.ok(mapOf("success" to true))
        else
            ResponseEntity.badRequest().body(mapOf("success" to false, "message" to "Não está ligado"))
    }

    @PostMapping("/send-text")
    @PreAuthorize("hasAuthority('commands:write')")
    fun sendText(@RequestBody body: Map<String, String>): ResponseEntity<Any> {
        val text = body["text"] ?: return ResponseEntity.badRequest().body(mapOf("success" to false, "message" to "Campo 'text' em falta"))
        val success = comPortService.sendText(text)
        auditService.record(AuditAction.COMMAND_SEND, target = "COM (texto)", details = text, success = success)
        return if (success)
            ResponseEntity.ok(mapOf("success" to true))
        else
            ResponseEntity.badRequest().body(mapOf("success" to false, "message" to "Não está ligado"))
    }
}
