import { useState, useEffect, useMemo, useRef } from 'react'
import { useAuth } from '../context/AuthContext'
import {
  fetchOrbitConfig, setManualTle, fetchTleFromCelestrak, setStation,
  type OrbitConfig, type GroundStation,
} from '../api/orbit'
import {
  isOrbitReady, makeSatrec, subPoint, groundTrack, nextPasses,
  type SubPoint, type Pass, type TrackPoint,
} from '../utils/orbit'

const project = (lat: number, lon: number) => ({ x: lon + 180, y: 90 - lat })

// Divide o ground track em segmentos sempre que cruza a linha de data (±180°)
function trackSegments(track: TrackPoint[]): TrackPoint[][] {
  const segs: TrackPoint[][] = []
  let cur: TrackPoint[] = []
  for (let i = 0; i < track.length; i++) {
    if (i > 0 && Math.abs(track[i].lon - track[i - 1].lon) > 180) {
      segs.push(cur); cur = []
    }
    cur.push(track[i])
  }
  if (cur.length) segs.push(cur)
  return segs
}

export default function OrbitPanel() {
  const { can } = useAuth()
  const isAdmin = can('users:manage')

  const [config, setConfig] = useState<OrbitConfig | null>(null)
  const [error, setError]   = useState<string | null>(null)
  const [ready, setReady]   = useState(isOrbitReady())
  const [now, setNow]       = useState(new Date())

  // Espera que a satellite.js carregue do CDN
  useEffect(() => {
    if (ready) return
    const t = setInterval(() => { if (isOrbitReady()) { setReady(true); clearInterval(t) } }, 500)
    return () => clearInterval(t)
  }, [ready])

  const load = () => {
    fetchOrbitConfig().then(setConfig).catch(() => setError('Não foi possível carregar a configuração orbital'))
  }
  useEffect(load, [])

  // Relógio para atualizar a posição ao vivo
  useEffect(() => {
    const t = setInterval(() => setNow(new Date()), 1000)
    return () => clearInterval(t)
  }, [])

  const satrec = useMemo(() => {
    if (!ready || !config?.tle) return null
    try { return makeSatrec(config.tle.line1, config.tle.line2) } catch { return null }
  }, [ready, config])

  const sub: SubPoint | null = useMemo(
    () => (satrec ? subPoint(satrec, now) : null),
    [satrec, now]
  )

  const track = useMemo(
    () => (satrec ? groundTrack(satrec, new Date(), 100, 30) : []),
    [satrec]
  )

  const passes: Pass[] = useMemo(
    () => (satrec && config ? nextPasses(satrec, config.station, new Date(), 48) : []),
    [satrec, config]
  )

  const station = config?.station

  return (
    <div className="card" style={{ maxWidth: 1000, margin: '0 auto' }}>
      <div className="card-header"><span className="card-title">🛰️ Rastreio Orbital</span></div>
      <div className="card-body">
        {error && <div className="error-msg" style={{ marginBottom: 12 }}>{error}</div>}

        {!config?.tle ? (
          <p style={{ color: '#94a3b8' }}>
            Ainda não há TLE configurado.{isAdmin ? ' Define um abaixo (manual ou via Celestrak).' : ' Pede a um administrador para configurar.'}
          </p>
        ) : !ready ? (
          <p style={{ color: '#64748b' }}>A carregar motor de propagação orbital…</p>
        ) : (
          <>
            {/* Estado atual */}
            <div style={{ display: 'flex', flexWrap: 'wrap', gap: 16, marginBottom: 12, fontSize: '0.85rem', color: '#cbd5e1' }}>
              <span><b style={{ color: '#93c5fd' }}>{config.tle.name ?? 'Satélite'}</b></span>
              {sub && <span>Lat {sub.lat.toFixed(2)}° · Lon {sub.lon.toFixed(2)}° · Alt {sub.altKm.toFixed(0)} km</span>}
              <span style={{ color: '#64748b' }}>Fonte: {config.tle.source}</span>
            </div>

            {/* Mapa equirectangular */}
            <svg viewBox="0 0 360 180" width="100%" style={{ background: '#0b1020', borderRadius: 8, border: '1px solid #2a2a3e' }}>
              {/* Graticule */}
              {[-150,-120,-90,-60,-30,0,30,60,90,120,150].map(lon => (
                <line key={`v${lon}`} x1={lon+180} y1={0} x2={lon+180} y2={180}
                      stroke={lon===0 ? '#3b4d6b' : '#1c2740'} strokeWidth={lon===0 ? 0.6 : 0.3} />
              ))}
              {[-60,-30,0,30,60].map(lat => (
                <line key={`h${lat}`} x1={0} y1={90-lat} x2={360} y2={90-lat}
                      stroke={lat===0 ? '#3b4d6b' : '#1c2740'} strokeWidth={lat===0 ? 0.6 : 0.3} />
              ))}

              {/* Ground track */}
              {trackSegments(track).map((seg, i) => (
                <polyline key={i} fill="none" stroke="#4f9cf9" strokeWidth={0.7} opacity={0.85}
                          points={seg.map(p => { const q = project(p.lat, p.lon); return `${q.x},${q.y}` }).join(' ')} />
              ))}

              {/* Estação terrestre */}
              {station && (() => { const q = project(station.latitude, station.longitude); return (
                <g key="st">
                  <circle cx={q.x} cy={q.y} r={2} fill="#22c55e" />
                  <text x={q.x + 3} y={q.y - 2} fontSize={5} fill="#86efac">{station.name}</text>
                </g>
              )})()}

              {/* Satélite */}
              {sub && (() => { const q = project(sub.lat, sub.lon); return (
                <g key="sat">
                  <circle cx={q.x} cy={q.y} r={3} fill="#f97316" stroke="#fff" strokeWidth={0.4} />
                </g>
              )})()}
            </svg>

            {/* Próximas passagens */}
            <h3 style={{ margin: '18px 0 8px', color: '#e2e8f0', fontSize: '1rem' }}>Próximas passagens (estação: {station?.name})</h3>
            {passes.length === 0 ? (
              <p style={{ color: '#64748b' }}>Sem passagens visíveis nas próximas 48 h.</p>
            ) : (
              <table className="admin-table">
                <thead><tr><th>Início (AOS)</th><th>Fim (LOS)</th><th>Duração</th><th>Elevação máx.</th></tr></thead>
                <tbody>
                  {passes.map((p, i) => (
                    <tr key={i}>
                      <td>{p.aos.toLocaleString('pt-PT')}</td>
                      <td>{p.los.toLocaleTimeString('pt-PT')}</td>
                      <td>{Math.round(p.durationSec / 60)} min</td>
                      <td>{p.maxElevation.toFixed(0)}°</td>
                    </tr>
                  ))}
                </tbody>
              </table>
            )}
          </>
        )}

        {isAdmin && <AdminControls onChanged={load} station={station} />}
      </div>
    </div>
  )
}

// ── Controlos de admin ─────────────────────────────────────────────────────────

function AdminControls({ onChanged, station }: { onChanged: () => void; station?: GroundStation }) {
  const [norad, setNorad] = useState('')
  const [name, setName]   = useState('')
  const [l1, setL1]       = useState('')
  const [l2, setL2]       = useState('')
  const [msg, setMsg]     = useState('')
  const [busy, setBusy]   = useState(false)

  const ref = useRef<GroundStation>({ name: '', latitude: 0, longitude: 0, altitudeM: 0 })
  const [stName, setStName] = useState('')
  const [stLat, setStLat]   = useState('')
  const [stLon, setStLon]   = useState('')
  const [stAlt, setStAlt]   = useState('')

  useEffect(() => {
    if (station && ref.current.name !== station.name) {
      ref.current = station
      setStName(station.name); setStLat(String(station.latitude))
      setStLon(String(station.longitude)); setStAlt(String(station.altitudeM))
    }
  }, [station])

  const run = async (fn: () => Promise<unknown>, ok: string) => {
    setBusy(true); setMsg('')
    try { await fn(); setMsg(ok); onChanged() }
    catch (e) { setMsg(e instanceof Error ? e.message : 'Erro') }
    finally { setBusy(false) }
  }

  return (
    <div style={{ marginTop: 24, borderTop: '1px solid #2a2a3e', paddingTop: 16 }}>
      <h3 style={{ margin: '0 0 12px', color: '#e2e8f0', fontSize: '1rem' }}>⚙️ Configuração (admin)</h3>

      {msg && <div className="error-msg" style={{ marginBottom: 10 }}>{msg}</div>}

      <div className="form-row" style={{ alignItems: 'flex-end' }}>
        <label>NORAD ID (Celestrak)
          <input value={norad} onChange={e => setNorad(e.target.value)} placeholder="ex: 25544 (ISS)" />
        </label>
        <button className="btn btn-sm" disabled={busy || !norad}
                onClick={() => run(() => fetchTleFromCelestrak(norad), 'TLE obtido do Celestrak')}>
          Obter do Celestrak
        </button>
      </div>

      <div style={{ marginTop: 12 }}>
        <label style={{ display: 'block', fontSize: '0.8rem', color: '#94a3b8', marginBottom: 4 }}>TLE manual</label>
        <input style={fullInput} value={name} onChange={e => setName(e.target.value)} placeholder="Nome (opcional)" />
        <input style={{ ...fullInput, fontFamily: 'monospace' }} value={l1} onChange={e => setL1(e.target.value)} placeholder="Linha 1 (1 25544U ...)" />
        <input style={{ ...fullInput, fontFamily: 'monospace' }} value={l2} onChange={e => setL2(e.target.value)} placeholder="Linha 2 (2 25544 ...)" />
        <button className="btn btn-sm" disabled={busy || !l1 || !l2}
                onClick={() => run(() => setManualTle(l1, l2, name || undefined), 'TLE guardado')}>
          Guardar TLE
        </button>
      </div>

      <div style={{ marginTop: 16 }}>
        <label style={{ display: 'block', fontSize: '0.8rem', color: '#94a3b8', marginBottom: 4 }}>Estação terrestre</label>
        <div className="form-row">
          <label>Nome<input value={stName} onChange={e => setStName(e.target.value)} /></label>
          <label>Latitude<input value={stLat} onChange={e => setStLat(e.target.value)} /></label>
          <label>Longitude<input value={stLon} onChange={e => setStLon(e.target.value)} /></label>
          <label>Altitude (m)<input value={stAlt} onChange={e => setStAlt(e.target.value)} /></label>
        </div>
        <button className="btn btn-sm" disabled={busy}
                onClick={() => run(() => setStation({
                  name: stName, latitude: parseFloat(stLat), longitude: parseFloat(stLon), altitudeM: parseFloat(stAlt) || 0,
                }), 'Estação atualizada')}>
          Guardar estação
        </button>
      </div>
    </div>
  )
}

const fullInput: React.CSSProperties = {
  display: 'block', width: '100%', boxSizing: 'border-box', marginBottom: 6,
  padding: '7px 10px', borderRadius: 6, border: '1px solid #3f3f5f',
  background: '#0f0f1a', color: '#e2e8f0', fontSize: '0.85rem',
}
