import { useState, useEffect } from 'react'
import { SatelliteStatus, TelemetryFormat } from '../types/satellite'
import { listPorts, connectPort, disconnectPort } from '../api/satellite'
import { ApiException } from '../api/client'

const BAUD_RATES = [1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400]

const TELEMETRY_FORMATS: { value: TelemetryFormat; label: string; hint: string }[] = [
  { value: 'AUTO',  label: 'AUTO — detetar',     hint: 'Deteta RAW ou ASCII automaticamente (recomendado).' },
  { value: 'RAW',   label: 'RAW — binário',      hint: 'Frame binário compacto do firmware (ttc_send_telemetry).' },
  { value: 'ASCII', label: 'ASCII — texto',      hint: 'Linhas TELEM:{...} ou tabela. Mais pesado a 9600 baud.' },
]

interface Props {
  status: SatelliteStatus
}

export default function ComPortPanel({ status }: Props) {
  const [ports, setPorts] = useState<string[]>([])
  const [selectedPort, setSelectedPort] = useState('')
  const [baudRate, setBaudRate] = useState(9600)
  const [format, setFormat] = useState<TelemetryFormat>('AUTO')
  const [loading, setLoading] = useState(false)
  const [error, setError] = useState('')

  const refresh = async () => {
    try {
      const p = await listPorts()
      setPorts(p)
      if (p.length > 0 && !selectedPort) setSelectedPort(p[0])
    } catch {
      setError('Backend não está acessível')
    }
  }

  useEffect(() => { refresh() }, [])

  const handleConnect = async () => {
    setError('')
    setLoading(true)
    try {
      const result = await connectPort(selectedPort, baudRate, format)
      // Se não houver mensagem no resultado, mostra uma mensagem genérica por omissão
      if (!result.success) setError(result.message ?? 'Erro desconhecido ao ligar à porta.');
    } catch (e) {
      // O cliente HTTP lança em qualquer resposta != 2xx (ex.: 400 "não foi
      // possível abrir a porta", 403 sem permissão). Mostra a causa real em vez
      // da mensagem genérica, que escondia o motivo.
      if (e instanceof ApiException) {
        const msg = e.body?.message ?? `Erro ${e.status} ao ligar.`
        setError(e.status === 403 ? `Sem permissão para ligar (precisa de 'commands:write'). ${msg}` : msg)
      } else {
        setError('Falha na ligação — o backend não respondeu (está a correr?).')
      }
    }
    setLoading(false)
  }

  const handleDisconnect = async () => {
    setLoading(true)
    try { await disconnectPort() } catch { /* ignore */ }
    setLoading(false)
  }

  return (
    <div className="card">
      <div className="card-header">
        <span className="card-title">🔌 Porta COM</span>
        <button className="btn btn-sm" onClick={refresh}>↻ Actualizar</button>
      </div>
      <div className="card-body">
        <div className="form-group">
          <label>PORTA</label>
          <select value={selectedPort} onChange={e => setSelectedPort(e.target.value)} disabled={status.connected}>
            {ports.length === 0 && <option value="">Nenhuma porta encontrada</option>}
            {ports.map(p => <option key={p} value={p}>{p}</option>)}
          </select>
        </div>
        <div className="form-group">
          <label>BAUD RATE</label>
          <select value={baudRate} onChange={e => setBaudRate(Number(e.target.value))} disabled={status.connected}>
            {BAUD_RATES.map(b => <option key={b} value={b}>{b}</option>)}
          </select>
        </div>
        <div className="form-group">
          <label>FORMATO TELEMETRIA</label>
          <select
            value={format}
            onChange={e => setFormat(e.target.value as TelemetryFormat)}
            disabled={status.connected}
            title={TELEMETRY_FORMATS.find(f => f.value === format)?.hint}
          >
            {TELEMETRY_FORMATS.map(f => <option key={f.value} value={f.value}>{f.label}</option>)}
          </select>
          <small className="form-hint">{TELEMETRY_FORMATS.find(f => f.value === format)?.hint}</small>
        </div>

        {error && <div className="error-msg">{error}</div>}

        {!status.connected ? (
          <button className="btn btn-primary btn-full" onClick={handleConnect} disabled={loading || !selectedPort}>
            {loading ? 'A ligar...' : '⚡ Ligar'}
          </button>
        ) : (
          <button className="btn btn-danger btn-full" onClick={handleDisconnect} disabled={loading}>
            {loading ? 'A desligar...' : '✕ Desligar'}
          </button>
        )}

        {status.connected && (
          <div className="connected-info">
            ● {status.comPort} @ {status.baudRate} baud · {status.telemetryFormat ?? 'AUTO'}
          </div>
        )}
      </div>
    </div>
  )
}
