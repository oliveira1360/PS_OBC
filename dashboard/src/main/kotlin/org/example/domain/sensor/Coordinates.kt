package org.example.domain.sensor

import kotlin.math.*

/**
 * Value Object — Posição geográfica GNSS.
 * Encapsula latitude, longitude e altitude com validação de domínio.
 */
data class Coordinates(
    val latitude: Double,    // -90..90 graus
    val longitude: Double,   // -180..180 graus
    val altitudeKm: Double,  // quilómetros acima do elipsóide WGS-84
    val speedKmS: Double = 0.0
) {
    init {
        require(latitude  in -90.0..90.0)   { "Latitude inválida: $latitude" }
        require(longitude in -180.0..180.0) { "Longitude inválida: $longitude" }
        require(altitudeKm >= 0.0)          { "Altitude negativa: $altitudeKm km" }
        require(speedKmS  >= 0.0)           { "Velocidade negativa: $speedKmS km/s" }
    }

    /** True se o satélite está em órbita LEO típica (200–2000 km). */
    fun isLEO(): Boolean = altitudeKm in 200.0..2000.0

    /**
     * Distância ao solo (geocêntrica aproximada).
     * Raio médio da Terra: 6371 km.
     */
    val distanceToGroundKm: Double get() = altitudeKm

    /**
     * Distância entre dois pontos em km (fórmula de Haversine).
     */
    fun distanceTo(other: Coordinates): Double {
        val r = 6371.0
        val dLat = Math.toRadians(other.latitude  - latitude)
        val dLon = Math.toRadians(other.longitude - longitude)
        val a = sin(dLat / 2).pow(2) +
                cos(Math.toRadians(latitude)) * cos(Math.toRadians(other.latitude)) *
                sin(dLon / 2).pow(2)
        return 2 * r * asin(sqrt(a))
    }

    override fun toString(): String =
        "%.4f°, %.4f° @ %.1f km (%.2f km/s)".format(latitude, longitude, altitudeKm, speedKmS)
}
