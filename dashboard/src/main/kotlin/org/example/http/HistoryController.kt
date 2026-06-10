package org.example.http

import org.example.service.HistoryService
import org.example.service.SensorReadingDto
import org.example.service.SensorStatsDto
import org.springframework.http.ResponseEntity
import org.springframework.security.access.prepost.PreAuthorize
import org.springframework.web.bind.annotation.*
import java.time.Instant

/**
 * Endpoints de histórico de sensores.
 * Todos exigem pelo menos a permissão HISTORY_READ.
 */
@RestController
@RequestMapping("/api/history")
class HistoryController(private val historyService: HistoryService) {

    @GetMapping("/sensors")
    @PreAuthorize("hasAuthority('history:read')")
    fun availableSensors(): ResponseEntity<List<String>> =
        ResponseEntity.ok(historyService.availableSensors())

    @GetMapping
    @PreAuthorize("hasAuthority('history:read')")
    fun history(
        @RequestParam sensors: List<String>,
        @RequestParam(required = false) from: Instant?,
        @RequestParam(required = false) to: Instant?
    ): ResponseEntity<List<SensorReadingDto>> =
        ResponseEntity.ok(historyService.history(sensors, from, to))

    @GetMapping("/stats")
    @PreAuthorize("hasAuthority('history:read')")
    fun stats(
        @RequestParam sensor: String,
        @RequestParam(required = false) from: Instant?,
        @RequestParam(required = false) to: Instant?
    ): ResponseEntity<SensorStatsDto> =
        ResponseEntity.ok(historyService.stats(sensor, from, to))
}
