/**
 * Cliente HTTP centralizado com injecção automática do token JWT
 * e tratamento uniforme de erros.
 *
 * Todas as chamadas API passam por aqui — nunca usar fetch() directamente.
 */

const BASE_URL = import.meta.env.VITE_API_BASE_URL ?? ''

// ── Tipos ──────────────────────────────────────────────────────────────────────

export interface ApiError {
  error: string
  message: string
  timestamp: string
}

export class ApiException extends Error {
  constructor(
    public readonly status: number,
    public readonly body: ApiError
  ) {
    super(body.message)
  }
}

// ── Token management ───────────────────────────────────────────────────────────

function getToken(): string | null {
  try {
    const raw = sessionStorage.getItem('auth')
    if (!raw) return null
    return (JSON.parse(raw) as { token?: string }).token ?? null
  } catch {
    return null
  }
}

// ── Core request ───────────────────────────────────────────────────────────────

async function request<T>(
  method: string,
  path: string,
  body?: unknown,
  opts?: { skipAuth?: boolean }
): Promise<T> {
  const headers: Record<string, string> = {}

  if (body !== undefined && !(body instanceof FormData)) {
    headers['Content-Type'] = 'application/json'
  }

  if (!opts?.skipAuth) {
    const token = getToken()
    if (token) headers['Authorization'] = `Bearer ${token}`
  }

  const res = await fetch(`${BASE_URL}${path}`, {
    method,
    headers,
    body: body instanceof FormData
      ? body
      : body !== undefined
        ? JSON.stringify(body)
        : undefined,
  })

  if (!res.ok) {
    let errorBody: ApiError
    try {
      errorBody = await res.json()
    } catch {
      errorBody = { error: res.statusText, message: `HTTP ${res.status}`, timestamp: new Date().toISOString() }
    }
    throw new ApiException(res.status, errorBody)
  }

  const text = await res.text()
  return text ? JSON.parse(text) : ({} as T)
}

// ── Convenience methods ────────────────────────────────────────────────────────

export const api = {
  get:    <T>(path: string, opts?: { skipAuth?: boolean }) => request<T>('GET', path, undefined, opts),
  post:   <T>(path: string, body?: unknown, opts?: { skipAuth?: boolean }) => request<T>('POST', path, body, opts),
  put:    <T>(path: string, body?: unknown) => request<T>('PUT', path, body),
  delete: <T>(path: string) => request<T>('DELETE', path),

  /** POST com FormData (para uploads). */
  upload: <T>(path: string, data: FormData) => request<T>('POST', path, data),
}
