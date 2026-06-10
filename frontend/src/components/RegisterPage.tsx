import { useState, FormEvent } from 'react'
import { useAuth } from '../context/AuthContext'
import { checkInvite } from '../api/admin'

const inputStyle: React.CSSProperties = {
  padding: '9px 12px', borderRadius: '8px', border: '1px solid #3f3f5f',
  background: '#0f0f1a', color: '#e2e8f0', fontSize: '0.9rem',
}
const labelStyle: React.CSSProperties = {
  display: 'flex', flexDirection: 'column', gap: '4px', fontSize: '0.8rem', color: '#94a3b8',
}

export default function RegisterPage({ onBackToLogin }: { onBackToLogin: () => void }) {
  const { signup } = useAuth()
  const [code,     setCode]     = useState('')
  const [username, setUsername] = useState('')
  const [email,    setEmail]    = useState('')
  const [password, setPassword] = useState('')
  const [error,    setError]    = useState<string | null>(null)
  const [loading,  setLoading]  = useState(false)
  const [roleHint, setRoleHint] = useState<string | null>(null)

  // Verifica o convite quando o utilizador acaba de escrever o código
  async function handleCodeBlur() {
    setRoleHint(null)
    const trimmed = code.trim()
    if (trimmed.length < 4) return
    try {
      const res = await checkInvite(trimmed)
      if (res.valid) {
        setRoleHint(`Convite válido — vais entrar como ${res.role}`)
        setError(null)
        if (res.email) setEmail(res.email)
      } else {
        setError(res.reason ?? 'Convite inválido')
      }
    } catch {
      /* silencioso — validação final é feita no submit */
    }
  }

  async function handleSubmit(e: FormEvent) {
    e.preventDefault()
    setError(null)
    setLoading(true)
    try {
      await signup({ code: code.trim().toUpperCase(), username, email, password })
      // Após signup o utilizador fica autenticado automaticamente (AuthContext)
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Não foi possível criar a conta')
    } finally {
      setLoading(false)
    }
  }

  return (
    <div style={{
      minHeight: '100vh', display: 'flex', alignItems: 'center',
      justifyContent: 'center', background: '#0a0a14',
    }}>
      <form onSubmit={handleSubmit} style={{
        background: '#12121f', padding: '40px', borderRadius: '16px',
        border: '1px solid #2a2a3e', width: '320px', display: 'flex',
        flexDirection: 'column', gap: '16px',
      }}>
        <div style={{ textAlign: 'center' }}>
          <div style={{ fontSize: '2rem' }}>🛰️</div>
          <h1 style={{ margin: '8px 0 4px', fontSize: '1.2rem', color: '#e2e8f0' }}>
            Criar Conta
          </h1>
          <p style={{ margin: 0, fontSize: '0.8rem', color: '#64748b' }}>
            Precisas de um código de convite de um administrador
          </p>
        </div>

        {error && (
          <div style={{
            padding: '10px', borderRadius: '8px', fontSize: '0.82rem',
            background: 'rgba(239,68,68,0.12)', color: '#fca5a5',
            border: '1px solid rgba(239,68,68,0.3)',
          }}>{error}</div>
        )}

        {roleHint && !error && (
          <div style={{
            padding: '10px', borderRadius: '8px', fontSize: '0.82rem',
            background: 'rgba(74,222,128,0.12)', color: '#86efac',
            border: '1px solid rgba(74,222,128,0.3)',
          }}>{roleHint}</div>
        )}

        <label style={labelStyle}>
          Código de convite
          <input
            type="text" value={code}
            onChange={e => setCode(e.target.value.toUpperCase())}
            onBlur={handleCodeBlur}
            required maxLength={10}
            placeholder="ABCD234XYZ"
            style={{ ...inputStyle, letterSpacing: '0.15em', fontFamily: 'monospace' }}
          />
        </label>

        <label style={labelStyle}>
          Utilizador
          <input type="text" value={username} onChange={e => setUsername(e.target.value)} required style={inputStyle} />
        </label>

        <label style={labelStyle}>
          Email
          <input type="email" value={email} onChange={e => setEmail(e.target.value)} required style={inputStyle} />
        </label>

        <label style={labelStyle}>
          Password
          <input type="password" value={password} onChange={e => setPassword(e.target.value)} required minLength={6} style={inputStyle} />
        </label>

        <button type="submit" disabled={loading} style={{
          padding: '11px', borderRadius: '8px', border: 'none',
          background: 'linear-gradient(135deg,#4f9cf9,#6c3de0)',
          color: '#fff', fontWeight: 600, fontSize: '0.95rem', cursor: 'pointer',
          opacity: loading ? 0.6 : 1,
        }}>
          {loading ? 'A criar…' : 'Criar Conta'}
        </button>

        <button type="button" onClick={onBackToLogin} style={{
          background: 'none', border: 'none', color: '#64748b',
          fontSize: '0.82rem', cursor: 'pointer', textDecoration: 'underline',
        }}>
          Já tens conta? Entrar
        </button>
      </form>
    </div>
  )
}
