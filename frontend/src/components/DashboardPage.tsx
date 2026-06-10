import type { SatelliteStatus, TelemetryData, SatelliteLog, OtaStatus } from '../types/satellite'
import ComPortPanel from './ComPortPanel'
import TelemetryPanel from './TelemetryPanel'
import LogsPanel from './LogsPanel'
import OtaPanel from './OtaPanel'

interface Props {
  status: SatelliteStatus
  telemetry: TelemetryData | null
  telemetryHistory: TelemetryData[]
  fieldTimestamps: Record<string, number>
  logs: SatelliteLog[]
  otaStatus: OtaStatus
  [key: string]: unknown   // aceita props extra sem erro
}

export default function DashboardPage({
  status, telemetry, telemetryHistory, fieldTimestamps, logs, otaStatus,
}: Props) {
  return (
    <>
      <aside className="col-left">
        <ComPortPanel status={status} />
        <OtaPanel otaStatus={otaStatus} connected={status.connected} />
      </aside>
      <section className="col-main">
        <TelemetryPanel
          telemetry={telemetry}
          history={telemetryHistory}
          fieldTimestamps={fieldTimestamps}
        />
        <LogsPanel logs={logs} />
      </section>
    </>
  )
}
