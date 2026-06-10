package org.example.repository

import org.example.domain.SatelliteStatus
import org.springframework.stereotype.Repository

interface StatusRepository {
    fun get(): SatelliteStatus
    fun save(status: SatelliteStatus)
}

@Repository
class InMemoryStatusRepository : StatusRepository {
    @Volatile private var status = SatelliteStatus()
    override fun get() = status
    override fun save(status: SatelliteStatus) { this.status = status }
}
