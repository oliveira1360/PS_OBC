package org.example.http

import org.example.service.AuditEntryDto
import org.example.service.AuditService
import org.springframework.format.annotation.DateTimeFormat
import org.springframework.http.ResponseEntity
import org.springframework.security.access.prepost.PreAuthorize
import org.springframework.web.bind.annotation.*
import java.time.Instant
import java.time.temporal.ChronoUnit

@RestController
@RequestMapping("/api/audit")
@PreAuthorize("hasAuthority('audit:read')")
class AuditController(private val auditService: AuditService) {

    @GetMapping
    fun list(
        @RequestParam(required = false) username: String?,
        @RequestParam(required = false) action: String?,
        @RequestParam(required = false) @DateTimeFormat(iso = DateTimeFormat.ISO.DATE_TIME) from: Instant?,
        @RequestParam(required = false) @DateTimeFormat(iso = DateTimeFormat.ISO.DATE_TIME) to: Instant?,
        @RequestParam(defaultValue = "200") limit: Int
    ): ResponseEntity<List<AuditEntryDto>> {
        val toInstant   = to ?: Instant.now()
        val fromInstant = from ?: toInstant.minus(30, ChronoUnit.DAYS)
        return ResponseEntity.ok(auditService.search(username, action, fromInstant, toInstant, limit))
    }
}
