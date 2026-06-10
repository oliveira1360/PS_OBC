import { useState, useEffect } from 'react'
import { listUsers, changeRole, listInvites, createInvite, revokeInvite } from '../api/admin'
import type { UserDto, InviteDto, InviteStatus } from '../api/admin'

const ROLES = ['ADMIN', 'OPERATOR', 'VIEWER'] as const

// ── Utilizadores ────────────────────────────────────────────────────────────────

function UserRow({ user, onRoleChanged }: { user: UserDto; onRoleChanged: () => void }) {
  const currentRole = user.roles[0] ?? 'VIEWER'
  const [newRole, setNewRole] = useState(currentRole)
  const [saving, setSaving]   = useState(false)
  const [msg, setMsg]         = useState('')

  async function handleChangeRole() {
    setSaving(true)
    setMsg('')
    try {
      await changeRole({ userId: user.id, newRole })
      setMsg('Alterado')
      onRoleChanged()
    } catch (e: unknown) {
      setMsg(e instanceof Error ? e.message : 'Erro')
    } finally {
      setSaving(false)
    }
  }

  return (
    <tr>
      <td>{user.username}</td>
      <td>{user.email}</td>
      <td>{currentRole}</td>
      <td>{user.enabled ? 'Sim' : 'Não'}</td>
      <td>
        <div style={{ display: 'flex', gap: 6, alignItems: 'center' }}>
          <select value={newRole} onChange={e => setNewRole(e.target.value)}>
            {ROLES.map(r => <option key={r} value={r}>{r}</option>)}
          </select>
          <button
            className="btn btn-sm"
            disabled={saving || newRole === currentRole}
            onClick={handleChangeRole}
          >
            {saving ? '…' : 'Alterar'}
          </button>
          {msg && <span style={{ fontSize: '0.8rem', color: msg === 'Alterado' ? '#4ade80' : '#f87171' }}>{msg}</span>}
        </div>
      </td>
    </tr>
  )
}

// ── Convites ──────────────────────────────────────────────────────────────────

const STATUS_COLORS: Record<InviteStatus, string> = {
  ACTIVE:  '#4ade80',
  USED:    '#64748b',
  REVOKED: '#f87171',
  EXPIRED: '#fbbf24',
}
const STATUS_LABEL: Record<InviteStatus, string> = {
  ACTIVE: 'Activo', USED: 'Usado', REVOKED: 'Revogado', EXPIRED: 'Expirado',
}

function GenerateInvite({ onCreated }: { onCreated: (inv: InviteDto) => void }) {
  const [role, setRole]       = useState('VIEWER')
  const [email, setEmail]     = useState('')
  const [loading, setLoading] = useState(false)
  const [error, setError]     = useState('')
  const [lastCode, setLastCode] = useState<string | null>(null)
  const [copied, setCopied]   = useState(false)

  async function handleGenerate(e: React.FormEvent) {
    e.preventDefault()
    setLoading(true)
    setError('')
    setCopied(false)
    try {
      const inv = await createInvite({ role, email: email.trim() || undefined })
      setLastCode(inv.code)
      setEmail('')
      onCreated(inv)
    } catch (err: unknown) {
      setError(err instanceof Error ? err.message : 'Erro ao gerar convite')
    } finally {
      setLoading(false)
    }
  }

  function copyCode() {
    if (!lastCode) return
    navigator.clipboard?.writeText(lastCode).then(() => {
      setCopied(true)
      setTimeout(() => setCopied(false), 2000)
    })
  }

  return (
    <form onSubmit={handleGenerate} className="admin-create-form">
      <h3 style={{ margin: '0 0 12px', color: '#e2e8f0', fontSize: '1rem' }}>Gerar Convite</h3>
      <p style={{ margin: '0 0 12px', fontSize: '0.82rem', color: '#64748b' }}>
        Cria um código de 10 caracteres (válido 7 dias, uso único). Partilha-o com a
        pessoa para que se registe na página de criação de conta.
      </p>
      <div className="form-row">
        <label>
          Role
          <select value={role} onChange={e => setRole(e.target.value)}>
            {ROLES.map(r => <option key={r} value={r}>{r}</option>)}
          </select>
        </label>
        <label>
          Email (opcional)
          <input type="email" value={email} onChange={e => setEmail(e.target.value)} placeholder="restringir a um email" />
        </label>
      </div>

      {error && <div className="error-msg" style={{ margin: '8px 0' }}>{error}</div>}

      {lastCode && (
        <div style={{
          display: 'flex', alignItems: 'center', gap: 12, margin: '12px 0',
          padding: '12px 16px', borderRadius: 8,
          background: 'rgba(79,156,249,0.1)', border: '1px solid rgba(79,156,249,0.3)',
        }}>
          <code style={{ fontSize: '1.3rem', letterSpacing: '0.2em', color: '#93c5fd', fontWeight: 700 }}>
            {lastCode}
          </code>
          <button type="button" className="btn btn-sm" onClick={copyCode}>
            {copied ? '✓ Copiado' : 'Copiar'}
          </button>
        </div>
      )}

      <button className="btn btn-primary" type="submit" disabled={loading}>
        {loading ? 'A gerar…' : 'Gerar Convite'}
      </button>
    </form>
  )
}

function InvitesTable({ invites, onRevoke }: { invites: InviteDto[]; onRevoke: () => void }) {
  const [busyId, setBusyId] = useState<string | null>(null)

  async function handleRevoke(id: string) {
    setBusyId(id)
    try {
      await revokeInvite(id)
      onRevoke()
    } catch {
      /* ignora — recarrega no fim */
    } finally {
      setBusyId(null)
    }
  }

  if (invites.length === 0) return <p style={{ color: '#64748b' }}>Sem convites.</p>

  return (
    <table className="admin-table">
      <thead>
        <tr>
          <th>Código</th>
          <th>Role</th>
          <th>Email</th>
          <th>Estado</th>
          <th>Expira</th>
          <th>Acção</th>
        </tr>
      </thead>
      <tbody>
        {invites.map(inv => (
          <tr key={inv.id}>
            <td><code style={{ letterSpacing: '0.1em' }}>{inv.code}</code></td>
            <td>{inv.role}</td>
            <td>{inv.email ?? '—'}</td>
            <td><span style={{ color: STATUS_COLORS[inv.status], fontWeight: 600 }}>{STATUS_LABEL[inv.status]}</span></td>
            <td>{new Date(inv.expiresAt).toLocaleDateString('pt-PT')}</td>
            <td>
              {inv.status === 'ACTIVE' ? (
                <button
                  className="btn btn-sm"
                  disabled={busyId === inv.id}
                  onClick={() => handleRevoke(inv.id)}
                >
                  {busyId === inv.id ? '…' : 'Revogar'}
                </button>
              ) : '—'}
            </td>
          </tr>
        ))}
      </tbody>
    </table>
  )
}

// ── Painel principal ───────────────────────────────────────────────────────────

export default function AdminPanel(_props: Record<string, unknown>) {
  const [users, setUsers]     = useState<UserDto[]>([])
  const [invites, setInvites] = useState<InviteDto[]>([])
  const [loading, setLoading] = useState(true)
  const [error, setError]     = useState<string | null>(null)

  function load() {
    setLoading(true)
    setError(null)
    Promise.all([listUsers(), listInvites()])
      .then(([u, i]) => { setUsers(u); setInvites(i) })
      .catch(() => setError('Erro ao carregar dados'))
      .finally(() => setLoading(false))
  }

  useEffect(load, [])

  return (
    <div className="card" style={{ maxWidth: 900, margin: '0 auto' }}>
      <div className="card-header">
        <span className="card-title">⚙️ Gestão de Utilizadores</span>
      </div>
      <div className="card-body">
        <GenerateInvite onCreated={() => load()} />

        <h3 style={{ margin: '24px 0 12px', color: '#e2e8f0', fontSize: '1rem' }}>Convites</h3>
        {error && <div className="error-msg">{error}</div>}
        {loading ? <p style={{ color: '#64748b' }}>A carregar…</p> : <InvitesTable invites={invites} onRevoke={load} />}

        <h3 style={{ margin: '24px 0 12px', color: '#e2e8f0', fontSize: '1rem' }}>Utilizadores</h3>
        {loading ? (
          <p style={{ color: '#64748b' }}>A carregar…</p>
        ) : (
          <table className="admin-table">
            <thead>
              <tr>
                <th>Username</th>
                <th>Email</th>
                <th>Role</th>
                <th>Activo</th>
                <th>Alterar Role</th>
              </tr>
            </thead>
            <tbody>
              {users.map(u => (
                <UserRow key={u.id} user={u} onRoleChanged={load} />
              ))}
            </tbody>
          </table>
        )}
      </div>
    </div>
  )
}
