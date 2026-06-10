package org.example.repository

import jakarta.persistence.*
import org.springframework.data.jpa.repository.JpaRepository
import org.springframework.stereotype.Repository
import java.time.Instant
import java.util.UUID

// ── Entity ─────────────────────────────────────────────────────────────────────

/**
 * Convite de registo. Um ADMIN gera um [code] de 10 caracteres associado a um
 * [roleName]. O convite é de uso único e expira em [expiresAt].
 *
 * Estado:
 *  - activo   → usedAt == null && revokedAt == null && expiresAt > now
 *  - usado    → usedAt != null
 *  - revogado → revokedAt != null
 *  - expirado → expiresAt <= now
 */
@Entity
@Table(name = "invites")
class InviteEntity(

    @Id
    val id: UUID = UUID.randomUUID(),

    @Column(nullable = false, unique = true, length = 10)
    val code: String,

    @Column(name = "role_name", nullable = false, length = 50)
    val roleName: String,

    @Column(length = 150)
    val email: String? = null,

    @Column(name = "created_by", nullable = false, length = 50)
    val createdBy: String,

    @Column(name = "created_at", nullable = false, updatable = false)
    val createdAt: Instant = Instant.now(),

    @Column(name = "expires_at", nullable = false)
    val expiresAt: Instant,

    @Column(name = "used_at")
    var usedAt: Instant? = null,

    @Column(name = "used_by")
    var usedBy: UUID? = null,

    @Column(name = "revoked_at")
    var revokedAt: Instant? = null
) {
    /** Convite utilizável: não usado, não revogado e dentro do prazo. */
    fun isUsable(now: Instant = Instant.now()): Boolean =
        usedAt == null && revokedAt == null && expiresAt.isAfter(now)
}

// ── Spring Data interface ──────────────────────────────────────────────────────

@Repository
interface InviteRepository : JpaRepository<InviteEntity, UUID> {
    fun findByCode(code: String): InviteEntity?
    fun existsByCode(code: String): Boolean
}
