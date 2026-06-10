import { useState, useMemo } from 'react'
import {
  LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer,
} from 'recharts'
import type { ChartPoint } from '../utils/sensors'
import { SENSOR_COLORS, sensorLabel } from '../utils/sensors'
import SensorHeatmap from './SensorHeatmap'

type View = 'lines' | 'heatmap'

interface Props {
  data: ChartPoint[]
  availableSensors: string[]
  /** Sensores pré-seleccionados na primeira renderização. */
  initialSelected?: string[]
  title?: string
}

const MAX_SENSORS = 12

export default function SensorExplorer({ data, availableSensors, initialSelected, title }: Props) {
  const [view, setView] = useState<View>('lines')
  const [selected, setSelected] = useState<string[]>(
    initialSelected ?? availableSensors.slice(0, 3)
  )

  // Mantém apenas selecionados que ainda existem na lista disponível
  const active = useMemo(
    () => selected.filter(s => availableSensors.includes(s)),
    [selected, availableSensors]
  )

  const toggle = (key: string) =>
    setSelected(prev =>
      prev.includes(key) ? prev.filter(s => s !== key)
        : prev.length >= MAX_SENSORS ? prev : [...prev, key]
    )

  return (
    <div>
      <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', flexWrap: 'wrap', gap: 12, marginBottom: 12 }}>
        {title && <h3 style={{ margin: 0, color: '#e2e8f0', fontSize: '1rem' }}>{title}</h3>}
        <div style={{ display: 'flex', gap: 6 }}>
          <button onClick={() => setView('lines')} style={tabStyle(view === 'lines')}>📈 Linhas</button>
          <button onClick={() => setView('heatmap')} style={tabStyle(view === 'heatmap')}>🟦 Heatmap</button>
        </div>
      </div>

      {/* Selector de sensores (chips clicáveis) */}
      <div style={{ display: 'flex', flexWrap: 'wrap', gap: 6, marginBottom: 14 }}>
        {availableSensors.length === 0 && (
          <span style={{ color: '#64748b', fontSize: '0.85rem' }}>Sem sensores disponíveis ainda.</span>
        )}
        {availableSensors.map(key => {
          const on = active.includes(key)
          const color = SENSOR_COLORS[active.indexOf(key) % SENSOR_COLORS.length]
          return (
            <button key={key} onClick={() => toggle(key)} style={{
              display: 'inline-flex', alignItems: 'center', gap: 6,
              padding: '4px 10px', borderRadius: 999, cursor: 'pointer',
              fontSize: '0.78rem',
              border: `1px solid ${on ? color : '#3f3f5f'}`,
              background: on ? `${color}22` : 'transparent',
              color: on ? '#e2e8f0' : '#94a3b8',
            }}>
              <span style={{ width: 8, height: 8, borderRadius: '50%', background: on ? color : '#475569' }} />
              {sensorLabel(key)}
            </button>
          )
        })}
      </div>

      {active.length === 0 ? (
        <div style={{ color: '#64748b', textAlign: 'center', padding: '48px 0' }}>
          Selecciona um ou mais sensores acima.
        </div>
      ) : view === 'lines' ? (
        <ResponsiveContainer width="100%" height={400}>
          <LineChart data={data} margin={{ top: 10, right: 30, left: 0, bottom: 0 }}>
            <CartesianGrid strokeDasharray="3 3" stroke="#2a2a3e" />
            <XAxis dataKey="time" tick={{ fontSize: 11, fill: '#94a3b8' }} />
            <YAxis tick={{ fontSize: 11, fill: '#94a3b8' }} />
            <Tooltip
              contentStyle={{ background: '#1e1e2e', border: '1px solid #3f3f5f', borderRadius: 8 }}
              labelStyle={{ color: '#94a3b8' }}
            />
            <Legend wrapperStyle={{ fontSize: 12 }} />
            {active.map((key, i) => (
              <Line key={key} type="monotone" dataKey={key} name={sensorLabel(key)}
                    stroke={SENSOR_COLORS[i % SENSOR_COLORS.length]} dot={false} strokeWidth={2} connectNulls />
            ))}
          </LineChart>
        </ResponsiveContainer>
      ) : (
        <SensorHeatmap data={data} sensors={active} />
      )}
    </div>
  )
}

function tabStyle(active: boolean): React.CSSProperties {
  return {
    padding: '6px 12px', borderRadius: 8, cursor: 'pointer', fontSize: '0.82rem',
    border: `1px solid ${active ? '#4f9cf9' : '#3f3f5f'}`,
    background: active ? 'rgba(79,156,249,0.15)' : 'transparent',
    color: active ? '#93c5fd' : '#94a3b8',
  }
}
