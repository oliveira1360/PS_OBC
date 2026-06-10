import { SatelliteStatus, SubsystemStatus } from '../types/satellite'

interface Props {
  status: SatelliteStatus
}

const COLOR: Record<SubsystemStatus, string> = {
  OK:      '#4ade80',
  WARNING: '#fbbf24',
  ERROR:   '#f87171',
  UNKNOWN: '#475569',
}

const SUBSYSTEMS: { key: keyof SatelliteStatus; label: string; icon: string }[] = [
  { key: 'obc',     label: 'OBC',     icon: '🖥' },
  { key: 'power',   label: 'Power',   icon: '⚡' },
  { key: 'comms',   label: 'Comms',   icon: '📡' },
  { key: 'adcs',    label: 'ADCS',    icon: '🧭' },
  { key: 'payload', label: 'Payload', icon: '🔭' },
]

export default function StatusPanel({ status }: Props) {
  return (
    <div className="card">
      <div className="card-header">
        <span className="card-title">📊 Subsistemas</span>
      </div>
      <div className="card-body subsystem-list">
        {SUBSYSTEMS.map(({ key, label, icon }) => {
          const s = status[key] as SubsystemStatus
          const color = COLOR[s]
          return (
            <div key={key} className="subsystem-row" style={{ borderColor: color + '30', background: color + '10' }}>
              <span className="subsystem-label">{icon} {label}</span>
              <span className="subsystem-status" style={{ color }}>{s}</span>
            </div>
          )
        })}
      </div>
    </div>
  )
}
