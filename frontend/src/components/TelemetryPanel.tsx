import { useEffect, useState } from 'react'
import { TelemetryData } from '../types/satellite'

interface Props {
  telemetry: TelemetryData | null
  history: TelemetryData[]
  fieldTimestamps: Record<string, number>
}

function useNow() {
  const [now, setNow] = useState(() => Date.now())
  useEffect(() => {
    const id = setInterval(() => setNow(Date.now()), 1000)
    return () => clearInterval(id)
  }, [])
  return now
}

function ageLabel(ts: number | undefined, now: number): string {
  if (!ts) return ''
  const s = Math.floor((now - ts) / 1000)
  if (s < 5)  return 'agora'
  if (s < 60) return `${s}s`
  return `${Math.floor(s / 60)}m`
}

function ageColor(ts: number | undefined, now: number): string {
  if (!ts) return '#475569'
  const s = Math.floor((now - ts) / 1000)
  if (s < 5)  return '#4ade80'  // green  — fresh
  if (s < 30) return '#fbbf24'  // yellow — getting old
  return '#f87171'              // red    — stale
}

function Row({ label, value, unit, decimals = 2, ts, now }: {
  label: string
  value?: number | null
  unit: string
  decimals?: number
  ts?: number
  now: number
}) {
  const age   = ageLabel(ts, now)
  const color = ageColor(ts, now)
  return (
    <div className="telem-row">
      <span className="telem-label">{label}</span>
      <span className="telem-value">
        {value != null ? value.toFixed(decimals) : '—'}
        <span className="telem-unit"> {unit}</span>
      </span>
      {age && (
        <span className="telem-age" style={{ color }}>{age}</span>
      )}
    </div>
  )
}

function Section({ title, children }: { title: string; children: React.ReactNode }) {
  return (
    <div className="telem-section">
      <div className="telem-section-title">{title}</div>
      {children}
    </div>
  )
}

export default function TelemetryPanel({ telemetry, fieldTimestamps }: Props) {
  const now = useNow()
  const t   = telemetry
  const ts  = (key: string) => fieldTimestamps[key]

  return (
    <div className="card">
      <div className="card-header">
        <span className="card-title">📡 Telemetria</span>
        {t && (
          <span className="card-subtitle">
            {new Date(t.timestamp).toLocaleTimeString()}
          </span>
        )}
      </div>
      <div className="card-body">
        {!t ? (
          <div className="chart-empty">A aguardar dados do satélite...</div>
        ) : (
          <div className="telem-grid">

            <Section title="🛰 GNSS">
              <Row label="Latitude"  value={t.latitude}  unit="deg"  ts={ts('latitude')}  now={now} />
              <Row label="Longitude" value={t.longitude} unit="deg"  ts={ts('longitude')} now={now} />
              <Row label="Altitude"  value={t.altitude}  unit="km"   ts={ts('altitude')}  now={now} />
              <Row label="Speed"     value={t.speed}     unit="km/s" ts={ts('speed')}     now={now} />
            </Section>

            <Section title="🔄 IMU">
              <Row label="Accel X" value={t.accelX} unit="m/s²"  ts={ts('accelX')} now={now} />
              <Row label="Accel Y" value={t.accelY} unit="m/s²"  ts={ts('accelY')} now={now} />
              <Row label="Accel Z" value={t.accelZ} unit="m/s²"  ts={ts('accelZ')} now={now} />
              <Row label="Gyro X"  value={t.gyroX}  unit="deg/s" ts={ts('gyroX')}  now={now} />
              <Row label="Gyro Y"  value={t.gyroY}  unit="deg/s" ts={ts('gyroY')}  now={now} />
              <Row label="Gyro Z"  value={t.gyroZ}  unit="deg/s" ts={ts('gyroZ')}  now={now} />
              <Row label="Mag X"   value={t.magX}   unit="µT"    ts={ts('magX')}   now={now} />
              <Row label="Mag Y"   value={t.magY}   unit="µT"    ts={ts('magY')}   now={now} />
              <Row label="Mag Z"   value={t.magZ}   unit="µT"    ts={ts('magZ')}   now={now} />
            </Section>

            <Section title="🌡 Pressão &amp; Temperatura">
              <Row label="Pressão"     value={t.pressure}    unit="hPa" ts={ts('pressure')}    now={now} />
              <Row label="Temperatura" value={t.temperature} unit="°C"  ts={ts('temperature')} now={now} />
            </Section>

            <Section title="⚡ EPS">
              <Row label="Tensão"   value={t.voltage} unit="V" ts={ts('voltage')} now={now} />
              <Row label="Corrente" value={t.current} unit="A" ts={ts('current')} now={now} />
              {t.batteryLevel != null && (
                <Row label="Bateria" value={t.batteryLevel} unit="%" decimals={0} ts={ts('batteryLevel')} now={now} />
              )}
            </Section>

            <Section title="📻 USART / Doppler">
              <Row label="Doppler" value={t.doppler} unit="kHz" ts={ts('doppler')} now={now} />
            </Section>

          </div>
        )}

        {t?.latitude != null && t?.longitude != null && (
          <div className="position-bar">
            📍 {t.latitude.toFixed(4)}°, {t.longitude.toFixed(4)}°
            {t.altitude != null && ` · Alt: ${t.altitude.toFixed(1)} km`}
            {t.speed    != null && ` · ${t.speed.toFixed(2)} km/s`}
          </div>
        )}
      </div>
    </div>
  )
}
