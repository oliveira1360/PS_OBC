import { useEffect, useRef } from 'react'
import { SatelliteLog, LogType } from '../types/satellite'

interface Props {
  logs: SatelliteLog[]
}

const LOG_COLOR: Record<LogType, string> = {
  INFO:     '#94a3b8',
  WARNING:  '#fbbf24',
  ERROR:    '#f87171',
  COMMAND:  '#60a5fa',
  RESPONSE: '#4ade80',
  SYSTEM:   '#a78bfa',
}

const LOG_TAG: Record<LogType, string> = {
  INFO:     'INFO',
  WARNING:  'WARN',
  ERROR:    'ERR ',
  COMMAND:  'CMD ',
  RESPONSE: 'RESP',
  SYSTEM:   'SYS ',
}

export default function LogsPanel({ logs }: Props) {
  const bottomRef = useRef<HTMLDivElement>(null)
  const containerRef = useRef<HTMLDivElement>(null)

  useEffect(() => {
    const el = containerRef.current
    if (!el) return
    const isAtBottom = el.scrollHeight - el.scrollTop - el.clientHeight < 80
    if (isAtBottom) bottomRef.current?.scrollIntoView({ behavior: 'smooth' })
  }, [logs])

  return (
    <div className="card logs-card">
      <div className="card-header">
        <span className="card-title">📋 Log de Eventos</span>
        <span className="card-subtitle">{logs.length} entradas</span>
      </div>
      <div className="logs-body" ref={containerRef}>
        {logs.length === 0 ? (
          <div className="logs-empty">
            Nenhum evento ainda. Liga-te a uma porta COM para começar.
          </div>
        ) : (
          logs.map(log => (
            <div key={log.id} className="log-row">
              <span className="log-time">{new Date(log.timestamp).toLocaleTimeString()}</span>
              <span className="log-tag" style={{ color: LOG_COLOR[log.type] }}>
                [{LOG_TAG[log.type]}]
              </span>
              <span className="log-msg" style={{ color: log.type === 'INFO' ? '#cbd5e1' : LOG_COLOR[log.type] }}>
                {log.message}
              </span>
            </div>
          ))
        )}
        <div ref={bottomRef} />
      </div>
    </div>
  )
}
