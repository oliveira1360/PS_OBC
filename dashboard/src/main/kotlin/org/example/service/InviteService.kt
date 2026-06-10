package org.example.service

import org.example.domain.rbac.Role
import org.example.repository.InviteEntity
import org.example.repository.InviteRepository
import org.example.service.error.*
import org.slf4j.LoggerFactory
import org.springframework.stereotype.Service
import java.security.SecureRandom
import java.time.Duration
import java.time.Instant
import java.util.UUID

// ── DTOs ───────────────────────────────────────────────────────────────────────

data class CreateInviteRequest(
    val role: String,
    val email: String? = null
)

/** Estado legível do convite para o frontend. */
enum class InviteStatus { ACTIVE, USED, REVOKED, EXPIRED }

data class InviteDto(
    val id: UUID,
    val code: String,
    val role: String,
    val email: String?,
    val createdBy: String,
    val createdAt: String,
    val expiresAt: String,
    val status: InviteStatus
)

/** Resultado da validação pública de um código (sem expor dados sensíveis). */
data class InviteCheckDto(
    val valid: Boolean,
    val role: String? = null,
    val email: String? = null,
    val reason: String? = null
)

// ── Interface ──────────────────────────────────────────────────────────────────

interface InviteService {
    fun create(request: CreateInviteRequest, createdBy: String): InviteDto
    fun list(): List<InviteDto>
    fun revoke(id: UUID)
    fun check(code: String): InviteCheckDto
}

// ── Implementação ──────────────────────────────────────────────────────────────

@Service
class InviteServiceImpl(
    private val inviteRepo: InviteRepository
) : InviteService {

    private val log = LoggerFactory.getLogger(javaClass)

    override fun create(request: CreateInviteRequest, createdBy: String): InviteDto {
        val role = Role.entries.find { it.name == request.role.uppercase() }
            ?: throw ValidationError(
                "Role inválido: ${request.role}. Valores possíveis: ${Role.entries.joinToString { it.name }}"
            )

        val email = request.email?.trim()?.takeIf { it.isNotBlank() }

        val entity = InviteEntity(
            code      = generateUniqueCode(),
            roleName  = role.name,
            email     = email,
            createdBy = createdBy,
            expiresAt = Instant.now().plus(VALIDITY)
        )

        val saved = inviteRepo.save(entity)
        log.info("Convite criado por {}: código {} (role {})", createdBy, saved.code, role.name)
        return saved.toDto()
    }

    override fun list(): List<InviteDto> =
        inviteRepo.findAll()
            .sortedByDescending { it.createdAt }
            .map { it.toDto() }

    override fun revoke(id: UUID) {
        val invite = inviteRepo.findById(id).orElse(null)
            ?: throw NotFoundError("Convite $id não encontrado")
        if (invite.usedAt != null)   throw ConflictError("Convite já foi usado")
        if (invite.revokedAt != null) throw ConflictError("Convite já estava revogado")
        invite.revokedAt = Instant.now()
        inviteRepo.save(invite)
        log.info("Convite {} revogado", invite.code)
    }

    override fun check(code: String): InviteCheckDto {
        val invite = inviteRepo.findByCode(code.trim().uppercase())
            ?: return InviteCheckDto(valid = false, reason = "Código inválido")
        return when {
            invite.usedAt != null    -> InviteCheckDto(false, reason = "Convite já utilizado")
            invite.revokedAt != null -> InviteCheckDto(false, reason = "Convite revogado")
            !invite.expiresAt.isAfter(Instant.now()) -> InviteCheckDto(false, reason = "Convite expirado")
            else -> InviteCheckDto(valid = true, role = invite.roleName, email = invite.email)
        }
    }

    // ── Geração do código ────────────────────────────────────────────────────────

    private fun generateUniqueCode(): String {
        repeat(10) {
            val code = randomCode()
            if (!inviteRepo.existsByCode(code)) return code
        }
        throw InternalError("Não foi possível gerar um código de convite único")
    }

    private fun randomCode(): String =
        (1..CODE_LENGTH).map { ALPHABET[RNG.nextInt(ALPHABET.length)] }.joinToString("")

    // ── Helpers ──────────────────────────────────────────────────────────────────

    private fun InviteEntity.toDto() = InviteDto(
        id        = id,
        code      = code,
        role      = roleName,
        email     = email,
        createdBy = createdBy,
        createdAt = createdAt.toString(),
        expiresAt = expiresAt.toString(),
        status    = statusOf(this)
    )

    private fun statusOf(i: InviteEntity): InviteStatus = when {
        i.usedAt != null    -> InviteStatus.USED
        i.revokedAt != null -> InviteStatus.REVOKED
        !i.expiresAt.isAfter(Instant.now()) -> InviteStatus.EXPIRED
        else                -> InviteStatus.ACTIVE
    }

    companion object {
        private const val CODE_LENGTH = 10
        // Alfabeto sem caracteres ambíguos (0/O, 1/I/L) para leitura/cópia fácil.
        private const val ALPHABET = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789"
        private val RNG = SecureRandom()
        private val VALIDITY: Duration = Duration.ofDays(7)
    }
}
