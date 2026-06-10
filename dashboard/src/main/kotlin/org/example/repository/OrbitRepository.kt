package org.example.repository

import jakarta.persistence.*
import org.springframework.data.jpa.repository.JpaRepository
import org.springframework.stereotype.Repository
import java.time.Instant

// ── TLE ────────────────────────────────────────────────────────────────────────

@Entity
@Table(name = "satellite_tle")
class TleEntity(

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    val id: Long = 0,

    @Column(length = 100)
    val name: String? = null,

    @Column(nullable = false, length = 80)
    val line1: String,

    @Column(nullable = false, length = 80)
    val line2: String,

    @Column(name = "norad_id", length = 10)
    val noradId: String? = null,

    @Column(nullable = false, length = 20)
    val source: String = "MANUAL",

    @Column(name = "updated_at", nullable = false)
    val updatedAt: Instant = Instant.now()
)

@Repository
interface TleRepository : JpaRepository<TleEntity, Long> {
    fun findTopByOrderByUpdatedAtDesc(): TleEntity?
}

// ── Estação terrestre ────────────────────────────────────────────────────────

@Entity
@Table(name = "ground_station")
class GroundStationEntity(

    @Id
    val id: Int = 1,

    @Column(nullable = false, length = 100)
    var name: String,

    @Column(nullable = false)
    var latitude: Double,

    @Column(nullable = false)
    var longitude: Double,

    @Column(name = "altitude_m", nullable = false)
    var altitudeM: Double = 0.0
)

@Repository
interface GroundStationRepository : JpaRepository<GroundStationEntity, Int>
