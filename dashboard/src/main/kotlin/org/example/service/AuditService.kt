package org.example.service

import org.example.repository.AuditLogEntity
import org.example.repository.AuditLogRepository
import org.slf4j.LoggerFactory
import org.springframework.data.domain.PageRequest
import org.springframework.security.core.context.SecurityContextHolder
import org.springframework.stereotype.Service
import org.springframework.web.context.request.RequestContextHolder
import org.springframework.web.context.request.ServletRequestAttributes
import java.time.Instant

// ── DTOs ───────────────────────────────────────────────────────────────────────

data class AuditEntryDto(
    val id: Long,
    val occurredAt: String,
    val username: String?,
    val action: String,
    val target: String?,
    val details: String?,
    val ipAddress: String?,
    val success: Boolean
)

// ── Acções normalizadas (constantes para evitar typos) ─────────────────────────

object AuditAction {
    const val LOGIN_SUCCESS  = "LOGIN_SUCCESS"
    const val LOGIN_FAILURE  = "LOGIN_FAILURE"
    const val SIGNUP         = "SIGNUP"
    const val COMMAND_SEND   = "COMMAND_SEND"
    const val COM_CONNECT    = "COM_CONNECT"
    const val COM_DISCONNECT = "COM_DISCONNECT"
    const val OTA_UPLOAD     = "OTA_UPLOAD"
    const val OTA_RESET      = "OTA_RESET"
    const val INVITE_CREATE  = "INVITE_CREATE"
    const val INVITE_REVOKE  = "INVITE_REVOKE"
    const val ROLE_CHANGE    = "ROLE_CHANGE"
}

// ── Serviço ──────────────────────────────────────────────────────────────────

@Service
class AuditService(private val repo: AuditLogRepository) {

    private val log = LoggerFactory.getLogger(javaClass)

    /**
     * Regista uma acção. Nunca lança — auditoria não deve quebrar o fluxo principal.
     */
    fun record(
        action: String,
        target: String? = null,
        details: String? = null,
        success: Boolean = true,
        usernameOverride: String? = null
    ) {
        try {
            repo.save(
                AuditLogEntity(
                    occurredAt = Instant.now(),
                    username   = usernameOverride ?: currentUsername(),
                    action     = action,
                    target     = target,
                    details    = details,
                    ipAddress  = currentIp(),
                    success    = success
                )
            )
        } catch (ex: Exception) {
            log.warn("Falha ao registar auditoria ({}): {}", action, ex.message)
        }
    }

    fun search(username: String?, action: String?, from: Instant, to: Instant, limit: Int): List<AuditEntryDto> =
        repo.search(
            username?.takeIf { it.isNotBlank() },
            action?.takeIf { it.isNotBlank() },
            from, to,
            PageRequest.of(0, limit.coerceIn(1, 1000))
        ).map { it.toDto() }

    // ── Helpers ────────────────────────────────────────────────────────────────

    private fun currentUsername(): String? =
        SecurityContextHolder.getContext().authentication
            ?.name
            ?.takeIf { it.isNotBlank() && it != "anonymousUser" }

    private fun currentIp(): String? = try {
        (RequestContextHolder.getRequestAttributes() as? ServletRequestAttributes)
            ?.request?.remoteAddr
    } catch (ex: Exception) {
        null
    }

    private fun AuditLogEntity.toDto() = AuditEntryDto(
        id, occurredAt.toString(), username, action, target, details, ipAddress, success
    )
}
