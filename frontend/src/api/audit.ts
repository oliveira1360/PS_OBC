import { api } from './client'

export interface AuditEntry {
  id: number
  occurredAt: string
  username: string | null
  action: string
  target: string | null
  details: string | null
  ipAddress: string | null
  success: boolean
}

export interface AuditFilter {
  username?: string
  action?: string
  from?: Date
  to?: Date
  limit?: number
}

export const fetchAudit = (f: AuditFilter = {}): Promise<AuditEntry[]> => {
  const p = new URLSearchParams()
  if (f.username) p.append('username', f.username)
  if (f.action)   p.append('action', f.action)
  if (f.from)     p.append('from', f.from.toISOString())
  if (f.to)       p.append('to', f.to.toISOString())
  p.append('limit', String(f.limit ?? 200))
  return api.get(`/api/audit?${p}`)
}
