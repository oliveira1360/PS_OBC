export interface TelemetryData {
  timestamp: number
  // EPS
  voltage?: number
  current?: number
  batteryLevel?: number
  // Pressure & Temperature
  temperature?: number
  pressure?: number
  // GNSS
  latitude?: number
  longitude?: number
  altitude?: number
  speed?: number
  // USART
  doppler?: number
  // IMU
  accelX?: number
  accelY?: number
  accelZ?: number
  gyroX?: number
  gyroY?: number
  gyroZ?: number
  magX?: number
  magY?: number
  magZ?: number
  // Comms
  rssi?: number
  signalStrength?: number
}

export type TelemetryFormat = 'RAW' | 'ASCII' | 'AUTO'

export interface SatelliteStatus {
  timestamp: number
  connected: boolean
  comPort?: string
  baudRate: number
  telemetryFormat?: TelemetryFormat
  obc: SubsystemStatus
  power: SubsystemStatus
  comms: SubsystemStatus
  adcs: SubsystemStatus
  payload: SubsystemStatus
}

export type SubsystemStatus = 'OK' | 'WARNING' | 'ERROR' | 'UNKNOWN'
export type LogType = 'INFO' | 'WARNING' | 'ERROR' | 'COMMAND' | 'RESPONSE' | 'SYSTEM'

export interface SatelliteLog {
  id: number
  timestamp: number
  type: LogType
  message: string
  raw?: string
}

export interface OtaStatus {
  inProgress: boolean
  progress: number
  totalRecords: number
  sentRecords: number
  success?: boolean
  message: string
}

export interface WsMessage {
  type: 'TELEMETRY' | 'STATUS' | 'LOG' | 'OTA_PROGRESS'
  data: TelemetryData | SatelliteStatus | SatelliteLog | OtaStatus
}

export const DEFAULT_STATUS: SatelliteStatus = {
  timestamp: Date.now(),
  connected: false,
  baudRate: 9600,
  telemetryFormat: 'AUTO',
  obc: 'UNKNOWN',
  power: 'UNKNOWN',
  comms: 'UNKNOWN',
  adcs: 'UNKNOWN',
  payload: 'UNKNOWN',
}
