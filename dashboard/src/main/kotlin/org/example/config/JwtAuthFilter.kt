package org.example.config

import jakarta.servlet.FilterChain
import jakarta.servlet.http.HttpServletRequest
import jakarta.servlet.http.HttpServletResponse
import org.example.service.JwtService
import org.example.service.UserDetailsServiceImpl
import org.springframework.security.authentication.UsernamePasswordAuthenticationToken
import org.springframework.security.core.context.SecurityContextHolder
import org.springframework.security.web.authentication.WebAuthenticationDetailsSource
import org.springframework.stereotype.Component
import org.springframework.web.filter.OncePerRequestFilter

@Component
class JwtAuthFilter(
    private val jwtService: JwtService,
    private val userDetailsService: UserDetailsServiceImpl
) : OncePerRequestFilter() {

    override fun doFilterInternal(
        request: HttpServletRequest,
        response: HttpServletResponse,
        chain: FilterChain
    ) {
        val header = request.getHeader("Authorization")
        if (header == null || !header.startsWith("Bearer ")) {
            chain.doFilter(request, response)
            return
        }

        val token    = header.removePrefix("Bearer ").trim()
        val username = runCatching { jwtService.extractUsername(token) }.getOrNull()
            ?: run { chain.doFilter(request, response); return }

        if (SecurityContextHolder.getContext().authentication == null) {
            val userDetails = runCatching { userDetailsService.loadUserByUsername(username) }.getOrNull()
                ?: run { chain.doFilter(request, response); return }

            if (jwtService.isTokenValid(token, userDetails)) {
                val auth = UsernamePasswordAuthenticationToken(userDetails, null, userDetails.authorities)
                    .also { it.details = WebAuthenticationDetailsSource().buildDetails(request) }
                SecurityContextHolder.getContext().authentication = auth
            }
        }
        chain.doFilter(request, response)
    }
}
