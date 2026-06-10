import { api } from './client'

// ── Tipos ──────────────────────────────────────────────────────────────────────

export interface SensorReading {
  sensorKey:    string
  recordedAt:   string   // ISO-8601
  valueNumeric: number | null
  valueInteger: number | null
}

export interface SensorStats {
  sensorKey: string
  count:     number
  min:       number | null
  max:       number | null
  avg:       number | null
  latest:    number | null
}

// ── API (token injectado automaticamente pelo client) ──────────────────────────

export const fetchAvailableSensors = (): Promise<string[]> =>
  api.get('/api/history/sensors')

export const fetchHistory = (
  sensors: string[],
  from: Date,
  to: Date
): Promise<SensorReading[]> => {
  const params = new URLSearchParams({ from: from.toISOString(), to: to.toISOString() })
  sensors.forEach(s => params.append('sensors', s))
  return api.get(`/api/history?${params}`)
}

export const fetchStats = (
  sensor: string,
  from: Date,
  to: Date
): Promise<SensorStats> => {
  const params = new URLSearchParams({ sensor, from: from.toISOString(), to: to.toISOString() })
  return api.get(`/api/history/stats?${params}`)
}
