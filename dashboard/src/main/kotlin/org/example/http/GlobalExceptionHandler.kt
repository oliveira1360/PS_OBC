package org.example.http

import org.example.service.error.*
import org.slf4j.LoggerFactory
import org.springframework.http.HttpStatus
import org.springframework.http.ResponseEntity
import org.springframework.security.access.AccessDeniedException
import org.springframework.security.core.AuthenticationException
import org.springframework.web.bind.annotation.ExceptionHandler
import org.springframework.web.bind.annotation.RestControllerAdvice
import java.time.Instant

/**
 * Traduz erros de serviço (e excepções Spring Security) em respostas HTTP uniformes.
 *
 * Formato do body:
 * ```json
 * { "error": "Tipo", "message": "Descrição legível", "timestamp": "ISO-8601" }
 * ```
 */
@RestControllerAdvice
class GlobalExceptionHandler {

    private val log = LoggerFactory.getLogger(javaClass)

    @ExceptionHandler(NotFoundError::class)
    fun handleNotFound(ex: NotFoundError) = error(HttpStatus.NOT_FOUND, ex)

    @ExceptionHandler(ForbiddenError::class)
    fun handleForbidden(ex: ForbiddenError) = error(HttpStatus.FORBIDDEN, ex)

    @ExceptionHandler(ConflictError::class)
    fun handleConflict(ex: ConflictError) = error(HttpStatus.CONFLICT, ex)

    @ExceptionHandler(ValidationError::class)
    fun handleValidation(ex: ValidationError) = error(HttpStatus.BAD_REQUEST, ex)

    @ExceptionHandler(BusyError::class)
    fun handleBusy(ex: BusyError) = error(HttpStatus.CONFLICT, ex)

    @ExceptionHandler(InternalError::class)
    fun handleInternal(ex: InternalError): ResponseEntity<ErrorBody> {
        log.error("Erro interno: ${ex.message}", ex.cause)
        return error(HttpStatus.INTERNAL_SERVER_ERROR, ex)
    }

    @ExceptionHandler(AccessDeniedException::class)
    fun handleAccessDenied(ex: AccessDeniedException) =
        error(HttpStatus.FORBIDDEN, ForbiddenError("Acesso negado"))

    @ExceptionHandler(AuthenticationException::class)
    fun handleAuth(ex: AuthenticationException) =
        error(HttpStatus.UNAUTHORIZED, ex)

    @ExceptionHandler(IllegalArgumentException::class)
    fun handleIllegalArg(ex: IllegalArgumentException) =
        error(HttpStatus.BAD_REQUEST, ex)

    // ── Helpers ────────────────────────────────────────────────────────────────

    private fun error(status: HttpStatus, ex: Exception): ResponseEntity<ErrorBody> =
        ResponseEntity.status(status).body(
            ErrorBody(
                error     = status.reasonPhrase,
                message   = ex.message ?: "Erro desconhecido",
                timestamp = Instant.now().toString()
            )
        )
}

data class ErrorBody(
    val error: String,
    val message: String,
    val timestamp: String
)
