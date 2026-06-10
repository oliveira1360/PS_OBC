import type { TelemetryData } from '../types/satellite'

// ── Paleta partilhada (até 12 sensores em simultâneo) ──────────────────────────
export const SENSOR_COLORS = [
  '#4f9cf9', '#f97316', '#22c55e', '#a855f7', '#ef4444',
  '#eab308', '#06b6d4', '#ec4899', '#64748b', '#14b8a6',
  '#f59e0b', '#8b5cf6',
]

// ── Labels legíveis por chave de sensor (sensor_key da BD) ──────────────────────
export const SENSOR_LABELS: Record<string, string> = {
  temperature:   'Temperatura (°C)',
  pressure:      'Pressão (hPa)',
  voltage:       'Tensão (V)',
  current:       'Corrente (A)',
  battery_level: 'Bateria (%)',
  latitude:      'Latitude (°)',
  longitude:     'Longitude (°)',
  altitude:      'Altitude (km)',
  speed:         'Velocidade (km/s)',
  accel_x: 'Accel X (g)', accel_y: 'Accel Y (g)', accel_z: 'Accel Z (g)',
  gyro_x:  'Gyro X (°/s)', gyro_y: 'Gyro Y (°/s)', gyro_z: 'Gyro Z (°/s)',
  mag_x:   'Mag X (µT)', mag_y: 'Mag Y (µT)', mag_z: 'Mag Z (µT)',
  rssi:    'RSSI (dBm)',
  doppler: 'Doppler (Hz)',
}

export const sensorLabel = (key: string): string => SENSOR_LABELS[key] ?? key

// ── Mapeamento campo camelCase (TelemetryData) → sensor_key (snake_case) ────────
const FIELD_TO_KEY: Record<keyof TelemetryData, string> = {
  timestamp: '',
  voltage: 'voltage', current: 'current', batteryLevel: 'battery_level',
  temperature: 'temperature', pressure: 'pressure',
  latitude: 'latitude', longitude: 'longitude', altitude: 'altitude', speed: 'speed',
  doppler: 'doppler',
  accelX: 'accel_x', accelY: 'accel_y', accelZ: 'accel_z',
  gyroX: 'gyro_x', gyroY: 'gyro_y', gyroZ: 'gyro_z',
  magX: 'mag_x', magY: 'mag_y', magZ: 'mag_z',
  rssi: 'rssi', signalStrength: 'rssi',
}

export interface ChartPoint {
  time: string
  t: number
  [key: string]: string | number | null
}

const fmtTime = (ms: number): string => {
  const d = new Date(ms)
  const p = (n: number) => n.toString().padStart(2, '0')
  return `${p(d.getHours())}:${p(d.getMinutes())}:${p(d.getSeconds())}`
}

/** Converte amostras de telemetria ao vivo (TelemetryData[]) em pontos de gráfico. */
export function telemetryToChartPoints(samples: TelemetryData[]): ChartPoint[] {
  return samples.map(s => {
    const pt: ChartPoint = { time: fmtTime(s.timestamp), t: s.timestamp }
    for (const field of Object.keys(s) as Array<keyof TelemetryData>) {
      const key = FIELD_TO_KEY[field]
      if (!key) continue
      const v = s[field]
      if (typeof v === 'number') pt[key] = v
    }
    return pt
  }).sort((a, b) => a.t - b.t)
}

/** Sensores presentes em pelo menos uma amostra (para preencher o selector ao vivo). */
export function sensorsPresent(points: ChartPoint[]): string[] {
  const set = new Set<string>()
  for (const p of points) {
    for (const k of Object.keys(p)) {
      if (k !== 'time' && k !== 't' && p[k] != null) set.add(k)
    }
  }
  return Array.from(set)
}
