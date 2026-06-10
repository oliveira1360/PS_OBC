package org.example.http

import org.example.service.*
import org.example.service.error.ValidationError
import org.slf4j.LoggerFactory
import org.springframework.http.HttpStatus
import org.springframework.http.ResponseEntity
import org.springframework.security.access.prepost.PreAuthorize
import org.springframework.security.authentication.AuthenticationManager
import org.springframework.security.authentication.UsernamePasswordAuthenticationToken
import org.springframework.security.core.Authentication
import org.springframework.security.core.AuthenticationException
import org.springframework.web.bind.annotation.*
import java.util.UUID

@RestController
@RequestMapping("/api/auth")
class AuthController(
    private val authManager: AuthenticationManager,
    private val jwtService: JwtService,
    private val userDetailsService: UserDetailsServiceImpl,
    private val userService: UserService,
    private val inviteService: InviteService,
    private val auditService: AuditService
) {
    private val log = LoggerFactory.getLogger(javaClass)

    // ── Público ─────────────────────────────────────────────────────────────────

    @PostMapping("/login")
    fun login(@RequestBody req: LoginRequest): ResponseEntity<LoginResponse> {
        try {
            authManager.authenticate(
                UsernamePasswordAuthenticationToken(req.username, req.password)
            )
        } catch (ex: AuthenticationException) {
            log.warn("Login falhado para '{}': {}", req.username, ex.message)
            auditService.record(
                AuditAction.LOGIN_FAILURE, target = req.username,
                details = "Credenciais inválidas", success = false, usernameOverride = req.username
            )
            throw ValidationError("Credenciais inválidas")
        }

        val ud    = userDetailsService.loadUserByUsername(req.username)
        val token = jwtService.generateToken(ud)
        val perms = ud.authorities.map { it.authority }
        auditService.record(AuditAction.LOGIN_SUCCESS, usernameOverride = req.username)
        return ResponseEntity.ok(LoginResponse(token, req.username, perms))
    }

    /** Registo público — só funciona com um código de convite válido. */
    @PostMapping("/signup")
    fun signup(@RequestBody req: SignupRequest): ResponseEntity<UserDto> {
        val created = userService.signup(req)
        auditService.record(
            AuditAction.SIGNUP, target = created.username,
            details = "role=${created.roles.joinToString()}", usernameOverride = created.username
        )
        return ResponseEntity.status(HttpStatus.CREATED).body(created)
    }

    /** Verifica um código de convite (para feedback no formulário de registo). */
    @GetMapping("/invites/check")
    fun checkInvite(@RequestParam code: String): ResponseEntity<InviteCheckDto> =
        ResponseEntity.ok(inviteService.check(code))

    @GetMapping("/me")
    fun me(principal: Authentication): ResponseEntity<MeResponse> {
        val ud    = userDetailsService.loadUserByUsername(principal.name)
        val perms = ud.authorities.map { it.authority }
        return ResponseEntity.ok(MeResponse(principal.name, perms))
    }

    // ── Gestão de utilizadores (requer permissão users:manage) ────────────────────

    @PutMapping("/role")
    @PreAuthorize("hasAuthority('users:manage')")
    fun changeRole(@RequestBody req: ChangeRoleRequest): ResponseEntity<UserDto> {
        val updated = userService.changeRole(req)
        auditService.record(
            AuditAction.ROLE_CHANGE, target = updated.username,
            details = "novo role=${req.newRole}"
        )
        return ResponseEntity.ok(updated)
    }

    @GetMapping("/users")
    @PreAuthorize("hasAuthority('users:manage')")
    fun listUsers(): ResponseEntity<List<UserDto>> =
        ResponseEntity.ok(userService.listAll())

    // ── Gestão de convites (requer permissão users:manage) ────────────────────────

    @PostMapping("/invites")
    @PreAuthorize("hasAuthority('users:manage')")
    fun createInvite(
        @RequestBody req: CreateInviteRequest,
        principal: Authentication
    ): ResponseEntity<InviteDto> {
        val invite = inviteService.create(req, principal.name)
        auditService.record(
            AuditAction.INVITE_CREATE, target = invite.code,
            details = "role=${invite.role}${invite.email?.let { ", email=$it" } ?: ""}"
        )
        return ResponseEntity.status(HttpStatus.CREATED).body(invite)
    }

    @GetMapping("/invites")
    @PreAuthorize("hasAuthority('users:manage')")
    fun listInvites(): ResponseEntity<List<InviteDto>> =
        ResponseEntity.ok(inviteService.list())

    @DeleteMapping("/invites/{id}")
    @PreAuthorize("hasAuthority('users:manage')")
    fun revokeInvite(@PathVariable id: UUID): ResponseEntity<Void> {
        inviteService.revoke(id)
        auditService.record(AuditAction.INVITE_REVOKE, target = id.toString())
        return ResponseEntity.noContent().build()
    }
}

// ── DTOs do controller ─────────────────────────────────────────────────────────

data class LoginRequest(val username: String, val password: String)
data class LoginResponse(val token: String, val username: String, val permissions: List<String>)
data class MeResponse(val username: String, val permissions: List<String>)
