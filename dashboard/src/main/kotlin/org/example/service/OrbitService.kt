package org.example.service

import org.example.repository.*
import org.example.service.error.*
import org.slf4j.LoggerFactory
import org.springframework.stereotype.Service
import java.net.URI
import java.net.http.HttpClient
import java.net.http.HttpRequest
import java.net.http.HttpResponse
import java.time.Duration

// ── DTOs ───────────────────────────────────────────────────────────────────────

data class TleDto(
    val name: String?,
    val line1: String,
    val line2: String,
    val noradId: String?,
    val source: String,
    val updatedAt: String
)

data class GroundStationDto(
    val name: String,
    val latitude: Double,
    val longitude: Double,
    val altitudeM: Double
)

data class OrbitConfigDto(
    val tle: TleDto?,
    val station: GroundStationDto
)

data class ManualTleRequest(
    val name: String? = null,
    val line1: String,
    val line2: String
)

// ── Serviço ──────────────────────────────────────────────────────────────────

@Service
class OrbitService(
    private val tleRepo: TleRepository,
    private val stationRepo: GroundStationRepository
) {
    private val log = LoggerFactory.getLogger(javaClass)
    private val http = HttpClient.newBuilder().connectTimeout(Duration.ofSeconds(10)).build()

    fun getConfig(): OrbitConfigDto =
        OrbitConfigDto(tleRepo.findTopByOrderByUpdatedAtDesc()?.toDto(), station().toDto())

    fun saveManualTle(req: ManualTleRequest): TleDto {
        val l1 = req.line1.trim()
        val l2 = req.line2.trim()
        validateTleLines(l1, l2)
        val entity = TleEntity(
            name    = req.name?.trim()?.takeIf { it.isNotBlank() },
            line1   = l1,
            line2   = l2,
            noradId = extractNoradId(l1),
            source  = "MANUAL"
        )
        return tleRepo.save(entity).toDto()
    }

    fun fetchFromCelestrak(noradId: String): TleDto {
        val id = noradId.trim()
        if (id.isBlank() || !id.all { it.isDigit() })
            throw ValidationError("NORAD ID inválido: $noradId")

        val url = "https://celestrak.org/NORAD/elements/gp.php?CATNR=$id&FORMAT=TLE"
        val body = try {
            val resp = http.send(
                HttpRequest.newBuilder(URI.create(url)).timeout(Duration.ofSeconds(15)).GET().build(),
                HttpResponse.BodyHandlers.ofString()
            )
            if (resp.statusCode() != 200) throw InternalError("Celestrak devolveu HTTP ${resp.statusCode()}")
            resp.body()
        } catch (ex: ValidationError) {
            throw ex
        } catch (ex: Exception) {
            throw InternalError("Não foi possível contactar o Celestrak: ${ex.message}", ex)
        }

        val lines = body.lines().map { it.trim() }.filter { it.isNotBlank() }
        if (lines.size < 2 || body.contains("No GP data", ignoreCase = true))
            throw NotFoundError("Nenhum TLE encontrado para o NORAD ID $id")

        // Formato TLE: pode vir com nome (3 linhas) ou só as 2 linhas de dados
        val (name, l1, l2) = if (lines[0].startsWith("1 ")) {
            Triple(null, lines[0], lines[1])
        } else {
            Triple(lines[0], lines[1], lines[2])
        }
        validateTleLines(l1, l2)

        val entity = TleEntity(name = name, line1 = l1, line2 = l2, noradId = id, source = "CELESTRAK")
        log.info("TLE obtido do Celestrak para NORAD {} ({})", id, name)
        return tleRepo.save(entity).toDto()
    }

    fun saveStation(req: GroundStationDto): GroundStationDto {
        if (req.latitude !in -90.0..90.0)   throw ValidationError("Latitude fora de [-90, 90]")
        if (req.longitude !in -180.0..180.0) throw ValidationError("Longitude fora de [-180, 180]")
        val s = station()
        s.name      = req.name.trim().ifBlank { "Estação" }
        s.latitude  = req.latitude
        s.longitude = req.longitude
        s.altitudeM = req.altitudeM
        return stationRepo.save(s).toDto()
    }

    // ── Helpers ────────────────────────────────────────────────────────────────

    private fun station(): GroundStationEntity =
        stationRepo.findById(1).orElseGet {
            stationRepo.save(GroundStationEntity(1, "Estação", 38.7566, -9.1163, 80.0))
        }

    private fun validateTleLines(l1: String, l2: String) {
        if (!l1.startsWith("1 ") || !l2.startsWith("2 "))
            throw ValidationError("TLE inválido: a linha 1 deve começar por '1 ' e a linha 2 por '2 '")
        if (l1.length < 60 || l2.length < 60)
            throw ValidationError("TLE inválido: linhas demasiado curtas")
    }

    private fun extractNoradId(line1: String): String? =
        line1.substring(2).takeWhile { it.isDigit() }.takeIf { it.isNotBlank() }

    private fun TleEntity.toDto() = TleDto(name, line1, line2, noradId, source, updatedAt.toString())
    private fun GroundStationEntity.toDto() = GroundStationDto(name, latitude, longitude, altitudeM)
}
