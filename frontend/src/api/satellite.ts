import { api } from './client'
import type {
  SatelliteStatus, SatelliteLog, OtaStatus, TelemetryData, TelemetryFormat
} from '../types/satellite'

// ── COM Port ───────────────────────────────────────────────────────────────────

interface PortsResponse { ports: string[] }
interface ActionResult  { success: boolean; message?: string }

export const listPorts = (): Promise<string[]> =>
  api.get<PortsResponse>('/api/com/ports').then(r => r.ports ?? [])

export const connectPort = (
  port: string,
  baudRate: number,
  format: TelemetryFormat = 'AUTO'
): Promise<ActionResult> =>
  api.post('/api/com/connect', { port, baudRate, format })

export const disconnectPort = (): Promise<void> =>
  api.post('/api/com/disconnect')

export const sendRawHex = (hex: string): Promise<ActionResult> =>
  api.post('/api/com/send', { hex })

// ── Satellite ──────────────────────────────────────────────────────────────────

export const getStatus = (): Promise<SatelliteStatus> =>
  api.get('/api/satellite/status')

interface LogsResponse { logs: SatelliteLog[] }
export const getLogs = (): Promise<SatelliteLog[]> =>
  api.get<LogsResponse>('/api/satellite/logs').then(r => r.logs ?? [])

export const getTelemetry = (): Promise<TelemetryData> =>
  api.get('/api/satellite/telemetry')

// ── OTA ────────────────────────────────────────────────────────────────────────

export type Transport = 'COM_PORT' | 'BLUETOOTH'

export const uploadFirmware = (
  file: File,
  transport: Transport,
  port: string,
  baudRate: number
): Promise<OtaStatus> => {
  const fd = new FormData()
  fd.append('file', file)
  fd.append('transport', transport)
  fd.append('port', port)
  fd.append('baudRate', baudRate.toString())
  return api.upload('/api/ota/upload', fd)
}

export const getOtaStatus = (): Promise<OtaStatus> =>
  api.get('/api/ota/status')

// ── Bluetooth ──────────────────────────────────────────────────────────────────

export interface BluetoothDevice { name: string; status: string }
export interface BluetoothScanResult {
  pairedDevices: BluetoothDevice[]
  bluetoothComPorts: string[]
  allComPorts: string[]
}

export const scanBluetooth = (): Promise<BluetoothScanResult> =>
  api.get('/api/bluetooth/scan')
