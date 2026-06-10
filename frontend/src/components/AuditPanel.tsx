import { useState, useEffect, useCallback } from 'react'
import { fetchAudit, type AuditEntry } from '../api/audit'

const ACTIONS = [
  '', 'LOGIN_SUCCESS', 'LOGIN_FAILURE', 'SIGNUP', 'COMMAND_SEND',
  'COM_CONNECT', 'COM_DISCONNECT', 'OTA_UPLOAD', 'OTA_RESET',
  'INVITE_CREATE', 'INVITE_REVOKE', 'ROLE_CHANGE',
]

const ACTION_LABEL: Record<string, string> = {
  LOGIN_SUCCESS: 'Login OK', LOGIN_FAILURE: 'Login falhado', SIGNUP: 'Registo',
  COMMAND_SEND: 'Comando', COM_CONNECT: 'Ligação', COM_DISCONNECT: 'Desligar',
  OTA_UPLOAD: 'OTA upload', OTA_RESET: 'OTA reset',
  INVITE_CREATE: 'Convite criado', INVITE_REVOKE: 'Convite revogado', ROLE_CHANGE: 'Mudança de role',
}
const label = (a: string) => ACTION_LABEL[a] ?? a

export default function AuditPanel() {
  const [entries, setEntries] = useState<AuditEntry[]>([])
  const [username, setUsername] = useState('')
  const [action, setAction]     = useState('')
  const [loading, setLoading]   = useState(true)
  const [error, setError]       = useState<string | null>(null)

  const load = useCallback(() => {
    setLoading(true); setError(null)
    fetchAudit({ username: username || undefined, action: action || undefined, limit: 300 })
      .then(setEntries)
      .catch(() => setError('Não foi possível carregar a auditoria'))
      .finally(() => setLoading(false))
  }, [username, action])

  useEffect(() => { load() }, []) // eslint-disable-line react-hooks/exhaustive-deps

  return (
    <div className="card" style={{ maxWidth: 1000, margin: '0 auto' }}>
      <div className="card-header"><span className="card-title">📜 Registo de Auditoria</span></div>
      <div className="card-body">
        <div className="form-row" style={{ alignItems: 'flex-end', marginBottom: 12 }}>
          <label>Utilizador
            <input value={username} onChange={e => setUsername(e.target.value)} placeholder="(todos)" />
          </label>
          <label>Acção
            <select value={action} onChange={e => setAction(e.target.value)}>
              {ACTIONS.map(a => <option key={a} value={a}>{a === '' ? '(todas)' : label(a)}</option>)}
            </select>
          </label>
          <button className="btn btn-sm" onClick={load} disabled={loading}>
            {loading ? '…' : 'Filtrar'}
          </button>
        </div>

        {error && <div className="error-msg">{error}</div>}

        {loading ? (
          <p style={{ color: '#64748b' }}>A carregar…</p>
        ) : entries.length === 0 ? (
          <p style={{ color: '#64748b' }}>Sem registos.</p>
        ) : (
          <div style={{ overflowX: 'auto' }}>
            <table className="admin-table">
              <thead>
                <tr><th>Quando</th><th>Utilizador</th><th>Acção</th><th>Alvo</th><th>Detalhes</th><th>IP</th><th>OK</th></tr>
              </thead>
              <tbody>
                {entries.map(e => (
                  <tr key={e.id}>
                    <td style={{ whiteSpace: 'nowrap' }}>{new Date(e.occurredAt).toLocaleString('pt-PT')}</td>
                    <td>{e.username ?? '—'}</td>
                    <td>{label(e.action)}</td>
                    <td>{e.target ?? '—'}</td>
                    <td style={{ maxWidth: 240, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }} title={e.details ?? ''}>{e.details ?? '—'}</td>
                    <td>{e.ipAddress ?? '—'}</td>
                    <td style={{ color: e.success ? '#4ade80' : '#f87171', fontWeight: 700 }}>{e.success ? '✓' : '✗'}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        )}
      </div>
    </div>
  )
}
