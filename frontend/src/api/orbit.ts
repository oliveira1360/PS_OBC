import { api } from './client'

export interface Tle {
  name: string | null
  line1: string
  line2: string
  noradId: string | null
  source: string
  updatedAt: string
}

export interface GroundStation {
  name: string
  latitude: number
  longitude: number
  altitudeM: number
}

export interface OrbitConfig {
  tle: Tle | null
  station: GroundStation
}

export const fetchOrbitConfig = (): Promise<OrbitConfig> =>
  api.get('/api/orbit/config')

export const setManualTle = (line1: string, line2: string, name?: string): Promise<Tle> =>
  api.put('/api/orbit/tle', { line1, line2, name })

export const fetchTleFromCelestrak = (noradId: string): Promise<Tle> =>
  api.post(`/api/orbit/tle/fetch?noradId=${encodeURIComponent(noradId)}`)

export const setStation = (s: GroundStation): Promise<GroundStation> =>
  api.put('/api/orbit/station', s)
