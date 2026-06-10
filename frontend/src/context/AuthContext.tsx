import { createContext, useContext, useState, useCallback, type ReactNode } from 'react'
import { api, ApiException } from '../api/client'
import { signup as signupApi, type SignupRequest } from '../api/admin'

// ── Tipos ──────────────────────────────────────────────────────────────────────

export interface AuthUser {
  username: string
  token: string
  permissions: string[]
}

interface AuthCtx {
  user: AuthUser | null
  login:  (username: string, password: string) => Promise<void>
  signup: (req: SignupRequest) => Promise<void>
  logout: () => void
  can:    (permission: string) => boolean
}

// ── Context ────────────────────────────────────────────────────────────────────

const AuthContext = createContext<AuthCtx | null>(null)

export function AuthProvider({ children }: { children: ReactNode }) {
  const [user, setUser] = useState<AuthUser | null>(() => {
    const saved = sessionStorage.getItem('auth')
    return saved ? JSON.parse(saved) : null
  })

  const login = useCallback(async (username: string, password: string) => {
    try {
      const data = await api.post<AuthUser>(
        '/api/auth/login',
        { username, password },
        { skipAuth: true }
      )
      sessionStorage.setItem('auth', JSON.stringify(data))
      setUser(data)
    } catch (e) {
      const msg = e instanceof ApiException ? e.body.message : 'Credenciais inválidas'
      throw new Error(msg)
    }
  }, [])

  const signup = useCallback(async (req: SignupRequest) => {
    try {
      await signupApi(req)
    } catch (e) {
      const msg = e instanceof ApiException ? e.body.message : 'Não foi possível criar a conta'
      throw new Error(msg)
    }
    // Registo bem-sucedido → autentica automaticamente
    await login(req.username, req.password)
  }, [login])

  const logout = useCallback(() => {
    sessionStorage.removeItem('auth')
    setUser(null)
  }, [])

  const can = useCallback(
    (permission: string) => user?.permissions.includes(permission) ?? false,
    [user]
  )

  return <AuthContext.Provider value={{ user, login, signup, logout, can }}>{children}</AuthContext.Provider>
}

export function useAuth() {
  const ctx = useContext(AuthContext)
  if (!ctx) throw new Error('useAuth must be used inside <AuthProvider>')
  return ctx
}
