import { useMemo } from 'react'
import { sensorLabel } from '../utils/sensors'

// Aceita qualquer linha com `time` + valores por sensor (live ou histórico)
interface HeatPoint { time: string; [key: string]: string | number | null }

interface Props {
  data: HeatPoint[]
  sensors: string[]
  maxCols?: number
}

const LABEL_W = 140
const ROW_H   = 28
const AXIS_H  = 24
const MIN_CELL = 6

// Escala azul (baixo) → vermelho (alto): hue 240 → 0
function valueToColor(norm: number): string {
  const hue = 240 - 240 * Math.max(0, Math.min(1, norm))
  return `hsl(${hue}, 75%, 52%)`
}

export default function SensorHeatmap({ data, sensors, maxCols = 240 }: Props) {
  const { cols, ranges } = useMemo(() => {
    // Downsample no eixo do tempo se houver demasiados pontos
    let pts = data
    if (data.length > maxCols) {
      const step = data.length / maxCols
      pts = Array.from({ length: maxCols }, (_, i) => data[Math.floor(i * step)])
    }
    // Min/max por sensor (normalização independente por linha)
    const ranges: Record<string, { min: number; max: number }> = {}
    for (const s of sensors) {
      let min = Infinity, max = -Infinity
      for (const p of pts) {
        const v = p[s]
        if (typeof v === 'number') { if (v < min) min = v; if (v > max) max = v }
      }
      ranges[s] = { min, max }
    }
    return { cols: pts, ranges }
  }, [data, sensors, maxCols])

  if (sensors.length === 0)
    return <p style={{ color: '#64748b', padding: '24px 0' }}>Selecciona pelo menos um sensor.</p>
  if (cols.length === 0)
    return <p style={{ color: '#64748b', padding: '24px 0' }}>Sem dados no período seleccionado.</p>

  const cellW = Math.max(MIN_CELL, Math.floor(720 / cols.length))
  const gridW = cols.length * cellW
  const totalW = LABEL_W + gridW
  const totalH = sensors.length * ROW_H + AXIS_H

  // Marcas de tempo (início, meio, fim)
  const ticks = [0, Math.floor(cols.length / 2), cols.length - 1]
    .filter((v, i, a) => a.indexOf(v) === i)

  return (
    <div style={{ overflowX: 'auto', paddingBottom: 8 }}>
      <svg width={totalW} height={totalH} style={{ fontFamily: 'inherit' }}>
        {sensors.map((s, row) => {
          const { min, max } = ranges[s]
          const span = max - min
          return (
            <g key={s} transform={`translate(0, ${row * ROW_H})`}>
              <text x={LABEL_W - 8} y={ROW_H / 2} textAnchor="end" dominantBaseline="middle"
                    fontSize={11} fill="#cbd5e1">{sensorLabel(s)}</text>
              {cols.map((p, c) => {
                const v = p[s]
                const has = typeof v === 'number'
                const norm = has && span > 0 ? ((v as number) - min) / span : 0.5
                return (
                  <rect
                    key={c}
                    x={LABEL_W + c * cellW} y={2}
                    width={cellW - (cellW > 3 ? 1 : 0)} height={ROW_H - 4}
                    fill={has ? valueToColor(norm) : '#1a1a2a'}
                    opacity={has ? 1 : 0.4}
                  >
                    <title>{`${sensorLabel(s)}\n${p.time}: ${has ? (v as number) : 'sem dado'}`}</title>
                  </rect>
                )
              })}
            </g>
          )
        })}
        {/* Eixo de tempo */}
        <g transform={`translate(0, ${sensors.length * ROW_H})`}>
          {ticks.map(t => (
            <text key={t} x={LABEL_W + t * cellW} y={16} fontSize={10} fill="#94a3b8"
                  textAnchor={t === 0 ? 'start' : t === cols.length - 1 ? 'end' : 'middle'}>
              {cols[t].time}
            </text>
          ))}
        </g>
      </svg>
    </div>
  )
}
