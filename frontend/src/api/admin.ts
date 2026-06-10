import { api } from './client'

// ── Tipos ──────────────────────────────────────────────────────────────────────

export interface UserDto {
  id: string
  username: string
  email: string
  enabled: boolean
  roles: string[]
  createdAt: string
}

export interface ChangeRoleRequest {
  userId: string
  newRole: string
}

export type InviteStatus = 'ACTIVE' | 'USED' | 'REVOKED' | 'EXPIRED'

export interface InviteDto {
  id: string
  code: string
  role: string
  email: string | null
  createdBy: string
  createdAt: string
  expiresAt: string
  status: InviteStatus
}

export interface CreateInviteRequest {
  role: string
  email?: string
}

export interface InviteCheck {
  valid: boolean
  role?: string
  email?: string
  reason?: string
}

export interface SignupRequest {
  code: string
  username: string
  email: string
  password: string
}

// ── Utilizadores (admin) ────────────────────────────────────────────────────────

export const listUsers = (): Promise<UserDto[]> =>
  api.get('/api/auth/users')

export const changeRole = (req: ChangeRoleRequest): Promise<UserDto> =>
  api.put('/api/auth/role', req)

// ── Convites (admin) ──────────────────────────────────────────────────────────

export const listInvites = (): Promise<InviteDto[]> =>
  api.get('/api/auth/invites')

export const createInvite = (req: CreateInviteRequest): Promise<InviteDto> =>
  api.post('/api/auth/invites', req)

export const revokeInvite = (id: string): Promise<void> =>
  api.delete(`/api/auth/invites/${id}`)

// ── Registo por convite (público) ──────────────────────────────────────────────

export const checkInvite = (code: string): Promise<InviteCheck> =>
  api.get(`/api/auth/invites/check?code=${encodeURIComponent(code)}`, { skipAuth: true })

export const signup = (req: SignupRequest): Promise<UserDto> =>
  api.post('/api/auth/signup', req, { skipAuth: true })
