package org.example.service

import io.jsonwebtoken.Claims
import io.jsonwebtoken.Jwts
import io.jsonwebtoken.security.Keys
import org.springframework.beans.factory.annotation.Value
import org.springframework.security.core.userdetails.UserDetails
import org.springframework.stereotype.Service
import java.util.Date
import javax.crypto.SecretKey

@Service
class JwtService(@Value("\${jwt.secret}") secret: String) {

    private val key: SecretKey = Keys.hmacShaKeyFor(secret.padEnd(64, '!').toByteArray())
    private val expirationMs = 8 * 60 * 60 * 1000L   // 8 horas

    fun generateToken(userDetails: UserDetails): String = Jwts.builder()
        .subject(userDetails.username)
        .claim("authorities", userDetails.authorities.map { it.authority })
        .issuedAt(Date())
        .expiration(Date(System.currentTimeMillis() + expirationMs))
        .signWith(key)
        .compact()

    fun extractUsername(token: String): String = claims(token).subject

    fun isTokenValid(token: String, userDetails: UserDetails): Boolean =
        extractUsername(token) == userDetails.username && !isExpired(token)

    private fun isExpired(token: String): Boolean =
        claims(token).expiration.before(Date())

    private fun claims(token: String): Claims = Jwts.parser()
        .verifyWith(key).build()
        .parseSignedClaims(token).payload
}
