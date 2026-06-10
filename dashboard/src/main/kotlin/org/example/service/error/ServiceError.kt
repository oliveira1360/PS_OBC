package org.example.service.error

/**
 * Hierarquia selada de erros de domínio/serviço.
 * Cada variante transporta uma mensagem legível e, opcionalmente, a causa.
 *
 * O [GlobalExceptionHandler] converte cada sub-tipo no código HTTP adequado.
 */
sealed class ServiceError(
    override val message: String,
    override val cause: Throwable? = null
) : RuntimeException(message, cause)

/** Recurso não encontrado (→ 404). */
class NotFoundError(message: String) : ServiceError(message)

/** Operação proibida para o utilizador actual (→ 403). */
class ForbiddenError(message: String = "Sem permissão para esta operação") : ServiceError(message)

/** Conflito de estado — ex.: username/email já existe (→ 409). */
class ConflictError(message: String) : ServiceError(message)

/** Dados de entrada inválidos (→ 400). */
class ValidationError(message: String) : ServiceError(message)

/** O serviço está ocupado ou num estado incompatível — ex.: OTA já em curso (→ 409). */
class BusyError(message: String) : ServiceError(message)

/** Erro interno inesperado (→ 500). */
class InternalError(message: String, cause: Throwable? = null) : ServiceError(message, cause)
