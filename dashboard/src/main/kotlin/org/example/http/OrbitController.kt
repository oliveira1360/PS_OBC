package org.example.http

import org.example.service.*
import org.springframework.http.ResponseEntity
import org.springframework.security.access.prepost.PreAuthorize
import org.springframework.web.bind.annotation.*

@RestController
@RequestMapping("/api/orbit")
class OrbitController(private val orbitService: OrbitService) {

    /** Config atual (TLE + estação) — qualquer utilizador autenticado com dashboard:read. */
    @GetMapping("/config")
    @PreAuthorize("hasAuthority('dashboard:read')")
    fun config(): ResponseEntity<OrbitConfigDto> =
        ResponseEntity.ok(orbitService.getConfig())

    /** Definir TLE manualmente (admin). */
    @PutMapping("/tle")
    @PreAuthorize("hasAuthority('users:manage')")
    fun setTle(@RequestBody req: ManualTleRequest): ResponseEntity<TleDto> =
        ResponseEntity.ok(orbitService.saveManualTle(req))

    /** Obter TLE do Celestrak por NORAD ID (admin). */
    @PostMapping("/tle/fetch")
    @PreAuthorize("hasAuthority('users:manage')")
    fun fetchTle(@RequestParam noradId: String): ResponseEntity<TleDto> =
        ResponseEntity.ok(orbitService.fetchFromCelestrak(noradId))

    /** Atualizar a localização da estação terrestre (admin). */
    @PutMapping("/station")
    @PreAuthorize("hasAuthority('users:manage')")
    fun setStation(@RequestBody req: GroundStationDto): ResponseEntity<GroundStationDto> =
        ResponseEntity.ok(orbitService.saveStation(req))
}
