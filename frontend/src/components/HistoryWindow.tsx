import { useState, useEffect, useCallback } from 'react'
import {
  LineChart, Line, XAxis, YAxis, CartesianGrid,
  Tooltip, Legend, ResponsiveContainer
} from 'recharts'
import { useAuth } from '../context/AuthContext'
import { fetchAvailableSensors, fetchHistory, SensorReading } from '../api/history'
import SensorHeatmap from './SensorHeatmap'
import './HistoryWindow.css'

// ── Paleta de cores para até 12 sensores simultâneos ─────────────────────────
const COLORS = [
  '#4f9cf9','#f97316','#22c55e','#a855f7','#ef4444',
  '#eab308','#06b6d4','#ec4899','#64748b','#14b8a6',
  '#f59e0b','#8b5cf6',
]

// ── Labels legíveis por chave de sensor ──────────────────────────────────────
const SENSOR_LABELS: Record<string, string> = {
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

// ── Presets de janela temporal ────────────────────────────────────────────────
const PRESETS = [
  { label: '15 min', minutes: 15 },
  { label: '1 hora', minutes: 60 },
  { label: '6 horas', minutes: 360 },
  { label: '24 horas', minutes: 1440 },
  { label: '7 dias', minutes: 10080 },
]

interface ChartPoint {
  time: string
  [key: string]: string | number | null
}

export default function HistoryWindow(_props: Record<string, unknown>) {
  const { user, can } = useAuth()

  const [availableSensors, setAvailableSensors] = useState<string[]>([])
  const [selectedSensors,  setSelectedSensors]  = useState<string[]>([])
  const [dropdownOpen,     setDropdownOpen]      = useState(false)

  // Janela temporal
  const [presetMinutes, setPresetMinutes] = useState(60)
  const [customFrom,    setCustomFrom]    = useState('')
  const [customTo,      setCustomTo]      = useState('')
  const [useCustom,     setUseCustom]     = useState(false)

  const [chartData,  setChartData]  = useState<ChartPoint[]>([])
  const [loading,    setLoading]    = useState(false)
  const [error,      setError]      = useState<string | null>(null)
  const [view,       setView]       = useState<'lines' | 'heatmap'>('lines')

  // Verifica permissão
  const hasAccess = can('history:read')

  // Carrega lista de sensores disponíveis
  useEffect(() => {
    if (!user || !hasAccess) return
    fetchAvailableSensors()
      .then(setAvailableSensors)
      .catch(() => setError('Não foi possível carregar os sensores'))
  }, [user, hasAccess])

  // Devolve a janela temporal activa [from, to]
  const getTimeRange = useCallback((): [Date, Date] => {
    if (useCustom && customFrom && customTo) {
      return [new Date(customFrom), new Date(customTo)]
    }
    const to   = new Date()
    const from = new Date(to.getTime() - presetMinutes * 60_000)
    return [from, to]
  }, [useCustom, customFrom, customTo, presetMinutes])

  // Carrega histórico
  const loadHistory = useCallback(async () => {
    if (selectedSensors.length === 0) return
    setLoading(true)
    setError(null)
    try {
      const [from, to] = getTimeRange()
      const readings   = await fetchHistory(selectedSensors, from, to)
      setChartData(buildChartData(readings, selectedSensors))
    } catch (e: unknown) {
      setError(e instanceof Error ? e.message : 'Erro desconhecido')
    } finally {
      setLoading(false)
    }
  }, [selectedSensors, getTimeRange])

  // Adiciona / remove sensor seleccionado
  const toggleSensor = (key: string) => {
    setSelectedSensors(prev =>
      prev.includes(key) ? prev.filter(s => s !== key) : [...prev, key]
    )
  }

  const removeSensor = (key: string) => {
    setSelectedSensors(prev => prev.filter(s => s !== key))
  }

  if (!hasAccess) {
    return (
      <div className="hw-no-access">
        <span>🔒</span>
        <p>Não tens permissão para ver o histórico de sensores.</p>
        <small>Requer role: Operador ou Administrador</small>
      </div>
    )
  }

  return (
    <div className="history-window">
      {/* ── Cabeçalho ──────────────────────────────────────────────────── */}
      <div className="hw-header">
        <h2>📊 Histórico de Sensores</h2>
        <span className="hw-subtitle">Análise temporal de leituras do satélite</span>
      </div>

      <div className="hw-controls">
        {/* ── Selector de sensores ───────────────────────────────────── */}
        <div className="hw-section">
          <label className="hw-label">Sensores</label>

          {/* Bolhas (tags) dos sensores seleccionados */}
          <div className="hw-tags">
            {selectedSensors.map((key, i) => (
              <span key={key} className="hw-tag" style={{ borderColor: COLORS[i % COLORS.length] }}>
                <span className="hw-tag-dot" style={{ background: COLORS[i % COLORS.length] }} />
                {SENSOR_LABELS[key] ?? key}
                <button className="hw-tag-remove" onClick={() => removeSensor(key)} aria-label="Remover">×</button>
              </span>
            ))}

            {/* Botão para abrir dropdown */}
            <div className="hw-dropdown-wrap">
              <button
                className="hw-add-btn"
                onClick={() => setDropdownOpen(o => !o)}
                disabled={selectedSensors.length >= 12}
              >
                + Adicionar sensor
              </button>

              {dropdownOpen && (
                <div className="hw-dropdown">
                  {availableSensors.length === 0 && (
                    <p className="hw-dropdown-empty">Nenhum sensor disponível</p>
                  )}
                  {availableSensors.map(key => (
                    <label key={key} className="hw-dropdown-item">
                      <input
                        type="checkbox"
                        checked={selectedSensors.includes(key)}
                        onChange={() => toggleSensor(key)}
                      />
                      {SENSOR_LABELS[key] ?? key}
                    </label>
                  ))}
                  <button className="hw-dropdown-close" onClick={() => setDropdownOpen(false)}>
                    Fechar
                  </button>
                </div>
              )}
            </div>
          </div>
        </div>

        {/* ── Selector de janela temporal ───────────────────────────── */}
        <div className="hw-section">
          <label className="hw-label">Período</label>
          <div className="hw-timeframe">
            <div className="hw-presets">
              {PRESETS.map(p => (
                <button
                  key={p.minutes}
                  className={`hw-preset-btn ${!useCustom && presetMinutes === p.minutes ? 'active' : ''}`}
                  onClick={() => { setPresetMinutes(p.minutes); setUseCustom(false) }}
                >
                  {p.label}
                </button>
              ))}
              <button
                className={`hw-preset-btn ${useCustom ? 'active' : ''}`}
                onClick={() => setUseCustom(true)}
              >
                Personalizado
              </button>
            </div>

            {useCustom && (
              <div className="hw-custom-range">
                <label>De:
                  <input type="datetime-local" value={customFrom}
                    onChange={e => setCustomFrom(e.target.value)} />
                </label>
                <label>Até:
                  <input type="datetime-local" value={customTo}
                    onChange={e => setCustomTo(e.target.value)} />
                </label>
              </div>
            )}
          </div>
        </div>

        {/* ── Botão de pesquisa ─────────────────────────────────────── */}
        <button
          className="hw-search-btn"
          onClick={loadHistory}
          disabled={loading || selectedSensors.length === 0}
        >
          {loading ? '⏳ A carregar…' : '🔍 Pesquisar'}
        </button>
      </div>

      {/* ── Erro ───────────────────────────────────────────────────────── */}
      {error && <div className="hw-error">⚠️ {error}</div>}

      {/* ── Selector de vista ─────────────────────────────────────────── */}
      {chartData.length > 0 && (
        <div style={{ display: 'flex', gap: 6, margin: '4px 0 8px' }}>
          <button onClick={() => setView('lines')} style={viewTab(view === 'lines')}>📈 Linhas</button>
          <button onClick={() => setView('heatmap')} style={viewTab(view === 'heatmap')}>🟦 Heatmap</button>
        </div>
      )}

      {/* ── Gráfico ────────────────────────────────────────────────────── */}
      <div className="hw-chart-wrap">
        {chartData.length === 0 && !loading ? (
          <div className="hw-empty">
            <span>📡</span>
            <p>Selecciona sensores e clica em "Pesquisar" para ver o histórico</p>
          </div>
        ) : view === 'heatmap' ? (
          <SensorHeatmap data={chartData} sensors={selectedSensors} />
        ) : (
          <ResponsiveContainer width="100%" height={420}>
            <LineChart data={chartData} margin={{ top: 10, right: 30, left: 0, bottom: 0 }}>
              <CartesianGrid strokeDasharray="3 3" stroke="#2a2a3e" />
              <XAxis dataKey="time" tick={{ fontSize: 11, fill: '#94a3b8' }} />
              <YAxis tick={{ fontSize: 11, fill: '#94a3b8' }} />
              <Tooltip
                contentStyle={{ background: '#1e1e2e', border: '1px solid #3f3f5f', borderRadius: 8 }}
                labelStyle={{ color: '#94a3b8' }}
              />
              <Legend wrapperStyle={{ fontSize: 12 }} />
              {selectedSensors.map((key, i) => (
                <Line
                  key={key}
                  type="monotone"
                  dataKey={key}
                  name={SENSOR_LABELS[key] ?? key}
                  stroke={COLORS[i % COLORS.length]}
                  dot={false}
                  strokeWidth={2}
                  connectNulls
                />
              ))}
            </LineChart>
          </ResponsiveContainer>
        )}
      </div>
    </div>
  )
}

function viewTab(active: boolean): React.CSSProperties {
  return {
    padding: '6px 12px', borderRadius: 8, cursor: 'pointer', fontSize: '0.82rem',
    border: `1px solid ${active ? '#4f9cf9' : '#3f3f5f'}`,
    background: active ? 'rgba(79,156,249,0.15)' : 'transparent',
    color: active ? '#93c5fd' : '#94a3b8',
  }
}

// ── Utilitário: agrupa leituras por timestamp (minuto) ───────────────────────
function buildChartData(readings: SensorReading[], sensors: string[]): ChartPoint[] {
  const map = new Map<string, ChartPoint>()

  for (const r of readings) {
    const timeKey = formatTime(r.recordedAt)
    if (!map.has(timeKey)) {
      const pt: ChartPoint = { time: timeKey }
      sensors.forEach(s => (pt[s] = null))
      map.set(timeKey, pt)
    }
    const pt = map.get(timeKey)!
    pt[r.sensorKey] = r.valueNumeric ?? r.valueInteger ?? null
  }

  return Array.from(map.values()).sort((a, b) => a.time.localeCompare(b.time))
}

function formatTime(iso: string): string {
  const d = new Date(iso)
  return `${d.getHours().toString().padStart(2,'0')}:${d.getMinutes().toString().padStart(2,'0')}:${d.getSeconds().toString().padStart(2,'0')}`
}
