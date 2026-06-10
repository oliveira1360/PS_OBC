package org.example.http

import org.example.service.SatelliteService
import org.springframework.http.ResponseEntity
import org.springframework.security.access.prepost.PreAuthorize
import org.springframework.web.bind.annotation.*

@RestController
@RequestMapping("/api/satellite")
@PreAuthorize("hasAuthority('dashboard:read')")
class SatelliteController(private val satelliteService: SatelliteService) {

    @GetMapping("/status")
    fun getStatus() = ResponseEntity.ok(satelliteService.getStatus())

    @GetMapping("/telemetry")
    fun getTelemetry() = ResponseEntity.ok(satelliteService.getLatestTelemetry())

    @GetMapping("/logs")
    fun getLogs() = ResponseEntity.ok(mapOf("logs" to satelliteService.getRecentLogs()))
}
