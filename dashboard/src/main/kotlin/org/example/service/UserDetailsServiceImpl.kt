package org.example.service

import org.example.repository.UserRepository
import org.slf4j.LoggerFactory
import org.springframework.security.core.authority.SimpleGrantedAuthority
import org.springframework.security.core.userdetails.User
import org.springframework.security.core.userdetails.UserDetails
import org.springframework.security.core.userdetails.UserDetailsService
import org.springframework.security.core.userdetails.UsernameNotFoundException
import org.springframework.stereotype.Service

@Service
class UserDetailsServiceImpl(private val userRepo: UserRepository) : UserDetailsService {

    private val log = LoggerFactory.getLogger(javaClass)

    override fun loadUserByUsername(username: String): UserDetails {
        val user = userRepo.findByUsername(username)
            ?: throw UsernameNotFoundException("Utilizador não encontrado: $username")

        log.debug("Login: user={}, roles={}", user.username, user.roles)

        val authorities = user.roles.flatMap { roleName ->
            val roleAuthority = SimpleGrantedAuthority("ROLE_$roleName")
            val permAuthorities = org.example.domain.rbac.Role
                .entries.find { it.name == roleName }
                ?.permissions?.map { SimpleGrantedAuthority(it.key) }
                ?: emptyList()
            listOf(roleAuthority) + permAuthorities
        }

        return User.builder()
            .username(user.username)
            .password(user.passwordHash)
            .authorities(authorities)
            .disabled(!user.enabled)
            .build()
    }
}
