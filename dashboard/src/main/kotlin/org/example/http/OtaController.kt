package org.example.http

import org.example.service.AuditAction
import org.example.service.AuditService
import org.example.service.OtaService
import org.springframework.http.ResponseEntity
import org.springframework.security.access.prepost.PreAuthorize
import org.springframework.web.bind.annotation.*
import org.springframework.web.multipart.MultipartFile

@RestController
@RequestMapping("/api/ota")
class OtaController(
    private val otaService: OtaService,
    private val auditService: AuditService
) {

    /** Upload and send a firmware file via OTA. */
    @PostMapping("/upload")
    @PreAuthorize("hasAuthority('ota:write')")
    fun uploadFirmware(
        @RequestParam("file")      file:      MultipartFile,
        @RequestParam("transport") transport: String,
        @RequestParam("port")      port:      String,
        @RequestParam("baudRate",  defaultValue = "115200") baudRate: Int
    ): ResponseEntity<Any> {
        auditService.record(
            AuditAction.OTA_UPLOAD, target = file.originalFilename ?: "firmware",
            details = "transport=$transport, port=$port, ${file.size} bytes"
        )
        return ResponseEntity.ok(otaService.sendFirmware(file, transport, port, baudRate))
    }

    @GetMapping("/status")
    @PreAuthorize("hasAuthority('dashboard:read')")
    fun getStatus() = ResponseEntity.ok(otaService.getStatus())

    /** Force-reset OTA state after a failed transfer without restarting the server. */
    @PostMapping("/reset")
    @PreAuthorize("hasAuthority('ota:write')")
    fun resetOta(): ResponseEntity<Any> {
        auditService.record(AuditAction.OTA_RESET)
        return ResponseEntity.ok(otaService.resetOta())
    }
}
