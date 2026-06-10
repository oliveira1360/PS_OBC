import { useState, FormEvent } from 'react'
import { useAuth } from '../context/AuthContext'

export default function LoginPage({ onRegister }: { onRegister?: () => void }) {
  const { login }    = useAuth()
  const [username,   setUsername]  = useState('')
  const [password,   setPassword]  = useState('')
  const [error,      setError]     = useState<string | null>(null)
  const [loading,    setLoading]   = useState(false)

  async function handleSubmit(e: FormEvent) {
    e.preventDefault()
    setError(null)
    setLoading(true)
    try {
      await login(username, password)
    } catch {
      setError('Credenciais inválidas. Tenta novamente.')
    } finally {
      setLoading(false)
    }
  }

  return (
    <div style={{
      minHeight: '100vh', display: 'flex', alignItems: 'center',
      justifyContent: 'center', background: '#0a0a14'
    }}>
      <form onSubmit={handleSubmit} style={{
        background: '#12121f', padding: '40px', borderRadius: '16px',
        border: '1px solid #2a2a3e', width: '320px', display: 'flex',
        flexDirection: 'column', gap: '16px'
      }}>
        <div style={{ textAlign: 'center' }}>
          <div style={{ fontSize: '2rem' }}>🛰️</div>
          <h1 style={{ margin: '8px 0 4px', fontSize: '1.2rem', color: '#e2e8f0' }}>
            Satellite Ground Control
          </h1>
          <p style={{ margin: 0, fontSize: '0.8rem', color: '#64748b' }}>Introduz as tuas credenciais</p>
        </div>

        {error && (
          <div style={{
            padding: '10px', borderRadius: '8px', fontSize: '0.82rem',
            background: 'rgba(239,68,68,0.12)', color: '#fca5a5',
            border: '1px solid rgba(239,68,68,0.3)'
          }}>{error}</div>
        )}

        <label style={{ display: 'flex', flexDirection: 'column', gap: '4px', fontSize: '0.8rem', color: '#94a3b8' }}>
          Utilizador
          <input
            type="text" value={username} onChange={e => setUsername(e.target.value)} required
            style={{ padding: '9px 12px', borderRadius: '8px', border: '1px solid #3f3f5f',
              background: '#0f0f1a', color: '#e2e8f0', fontSize: '0.9rem' }}
          />
        </label>

        <label style={{ display: 'flex', flexDirection: 'column', gap: '4px', fontSize: '0.8rem', color: '#94a3b8' }}>
          Password
          <input
            type="password" value={password} onChange={e => setPassword(e.target.value)} required
            style={{ padding: '9px 12px', borderRadius: '8px', border: '1px solid #3f3f5f',
              background: '#0f0f1a', color: '#e2e8f0', fontSize: '0.9rem' }}
          />
        </label>

        <button type="submit" disabled={loading} style={{
          padding: '11px', borderRadius: '8px', border: 'none',
          background: 'linear-gradient(135deg,#4f9cf9,#6c3de0)',
          color: '#fff', fontWeight: 600, fontSize: '0.95rem', cursor: 'pointer',
          opacity: loading ? 0.6 : 1
        }}>
          {loading ? 'A entrar…' : 'Entrar'}
        </button>

        {onRegister && (
          <button type="button" onClick={onRegister} style={{
            background: 'none', border: 'none', color: '#64748b',
            fontSize: '0.82rem', cursor: 'pointer', textDecoration: 'underline'
          }}>
            Tens um código de convite? Criar conta
          </button>
        )}
      </form>
    </div>
  )
}
