package org.example.repository

import org.example.domain.OtaStatus
import org.springframework.stereotype.Repository

interface OtaRepository {
    fun getStatus(): OtaStatus
    fun save(status: OtaStatus)
}

@Repository
class InMemoryOtaRepository : OtaRepository {
    @Volatile private var status = OtaStatus()
    override fun getStatus() = status
    override fun save(status: OtaStatus) { this.status = status }
}
