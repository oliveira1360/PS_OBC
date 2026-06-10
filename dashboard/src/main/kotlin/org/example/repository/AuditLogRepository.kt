package org.example.repository

import jakarta.persistence.*
import org.springframework.data.domain.Pageable
import org.springframework.data.jpa.repository.JpaRepository
import org.springframework.data.jpa.repository.Query
import org.springframework.data.repository.query.Param
import org.springframework.stereotype.Repository
import java.time.Instant

// ── Entity ─────────────────────────────────────────────────────────────────────

@Entity
@Table(name = "audit_log")
class AuditLogEntity(

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    val id: Long = 0,

    @Column(name = "occurred_at", nullable = false)
    val occurredAt: Instant = Instant.now(),

    @Column(length = 50)
    val username: String? = null,

    @Column(nullable = false, length = 60)
    val action: String,

    @Column(length = 200)
    val target: String? = null,

    @Column(columnDefinition = "TEXT")
    val details: String? = null,

    @Column(name = "ip_address", length = 45)
    val ipAddress: String? = null,

    @Column(nullable = false)
    val success: Boolean = true
)

// ── Spring Data interface ──────────────────────────────────────────────────────

@Repository
interface AuditLogRepository : JpaRepository<AuditLogEntity, Long> {

    @Query(
        """
        SELECT a FROM AuditLogEntity a
        WHERE (:username IS NULL OR a.username = :username)
          AND (:action   IS NULL OR a.action   = :action)
          AND a.occurredAt BETWEEN :from AND :to
        ORDER BY a.occurredAt DESC
        """
    )
    fun search(
        @Param("username") username: String?,
        @Param("action")   action: String?,
        @Param("from")     from: Instant,
        @Param("to")       to: Instant,
        pageable: Pageable
    ): List<AuditLogEntity>
}
