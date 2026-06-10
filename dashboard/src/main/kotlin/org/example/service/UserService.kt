package org.example.service

import org.example.domain.rbac.Role
import org.example.repository.InviteRepository
import org.example.repository.UserEntity
import org.example.repository.UserRepository
import org.example.service.error.*
import org.slf4j.LoggerFactory
import org.springframework.security.crypto.password.PasswordEncoder
import org.springframework.stereotype.Service
import org.springframework.transaction.annotation.Transactional
import java.time.Instant
import java.util.UUID

// ── DTOs de entrada/saída ──────────────────────────────────────────────────────

/**
 * Registo público — apenas possível com um código de convite válido.
 * O role NÃO é escolhido pelo utilizador: vem do convite criado por um admin.
 */
data class SignupRequest(
    val code: String,
    val username: String,
    val email: String,
    val password: String
)

data class ChangeRoleRequest(
    val userId: UUID,
    val newRole: String
)

data class UserDto(
    val id: UUID,
    val username: String,
    val email: String,
    val enabled: Boolean,
    val roles: Set<String>,
    val createdAt: String
)

// ── Interface ──────────────────────────────────────────────────────────────────

interface UserService {
    /** Registo via convite (público). Atómico: valida e consome o convite. */
    fun signup(request: SignupRequest): UserDto
    fun changeRole(request: ChangeRoleRequest): UserDto
    fun findByUsername(username: String): UserDto
    fun listAll(): List<UserDto>
}

// ── Implementação ──────────────────────────────────────────────────────────────

@Service
class UserServiceImpl(
    private val userRepo: UserRepository,
    private val inviteRepo: InviteRepository,
    private val passwordEncoder: PasswordEncoder
) : UserService {

    private val log = LoggerFactory.getLogger(javaClass)

    @Transactional
    override fun signup(request: SignupRequest): UserDto {
        // 1. Validação de entrada
        if (request.code.isBlank())      throw ValidationError("Código de convite em falta")
        if (request.username.isBlank())  throw ValidationError("Username não pode estar vazio")
        if (request.email.isBlank())     throw ValidationError("Email não pode estar vazio")
        if (request.password.length < 6) throw ValidationError("Password deve ter pelo menos 6 caracteres")

        // 2. Validar o convite
        val invite = inviteRepo.findByCode(request.code.trim().uppercase())
            ?: throw ValidationError("Código de convite inválido")

        when {
            invite.usedAt != null    -> throw ConflictError("Este convite já foi utilizado")
            invite.revokedAt != null -> throw ConflictError("Este convite foi revogado")
            !invite.expiresAt.isAfter(Instant.now()) -> throw ConflictError("Este convite expirou")
        }

        // Se o convite tem email fixo, o registo tem de respeitá-lo
        invite.email?.let { fixed ->
            if (!fixed.equals(request.email.trim(), ignoreCase = true))
                throw ValidationError("Este convite é apenas para o email $fixed")
        }

        val role = Role.entries.find { it.name == invite.roleName.uppercase() }
            ?: throw InternalError("Role do convite inválido: ${invite.roleName}")

        // 3. Unicidade
        if (userRepo.existsByUsername(request.username.trim()))
            throw ConflictError("Username '${request.username}' já existe")
        if (userRepo.existsByEmail(request.email.trim()))
            throw ConflictError("Email '${request.email}' já está registado")

        // 4. Criar utilizador com o role do convite
        val user = UserEntity(
            username     = request.username.trim(),
            email        = request.email.trim(),
            passwordHash = passwordEncoder.encode(request.password),
            roles        = mutableSetOf(role.name)
        )
        val saved = userRepo.save(user)

        // 5. Consumir o convite (uso único)
        invite.usedAt = Instant.now()
        invite.usedBy = saved.id
        inviteRepo.save(invite)

        log.info("Registo via convite {}: utilizador {} com role {}", invite.code, saved.username, role.name)
        return saved.toDto()
    }

    override fun changeRole(request: ChangeRoleRequest): UserDto {
        val role = Role.entries.find { it.name == request.newRole.uppercase() }
            ?: throw ValidationError("Role inválido: ${request.newRole}. Valores possíveis: ${Role.entries.joinToString { it.name }}")

        val user = userRepo.findById(request.userId).orElse(null)
            ?: throw NotFoundError("Utilizador com id ${request.userId} não encontrado")

        user.roles.clear()
        user.roles.add(role.name)

        val saved = userRepo.save(user)
        log.info("Role do utilizador {} alterado para {}", saved.username, role.name)
        return saved.toDto()
    }

    override fun findByUsername(username: String): UserDto {
        val user = userRepo.findByUsername(username)
            ?: throw NotFoundError("Utilizador '$username' não encontrado")
        return user.toDto()
    }

    override fun listAll(): List<UserDto> =
        userRepo.findAll().map { it.toDto() }

    // ── Helpers ────────────────────────────────────────────────────────────────

    private fun UserEntity.toDto() = UserDto(
        id        = id,
        username  = username,
        email     = email,
        enabled   = enabled,
        roles     = roles.toSet(),
        createdAt = createdAt.toString()
    )
}
