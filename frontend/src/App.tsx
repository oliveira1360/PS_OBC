import { useState, useCallback, useEffect, useRef, useMemo } from 'react'
import {
  TelemetryData, SatelliteStatus, SatelliteLog,
  OtaStatus, WsMessage, DEFAULT_STATUS,
} from './types/satellite'
import { useWebSocket } from './hooks/useWebSocket'
import { getStatus, getLogs, getOtaStatus } from './api/satellite'
import { useAuth } from './context/AuthContext'
import { telemetryToChartPoints, sensorsPresent } from './utils/sensors'
import ComPortPanel   from './components/ComPortPanel'
import TelemetryPanel from './components/TelemetryPanel'
import LogsPanel      from './components/LogsPanel'
import OtaPanel       from './components/OtaPanel'
import HistoryWindow  from './components/HistoryWindow'
import LoginPage      from './components/LoginPage'
import RegisterPage   from './components/RegisterPage'
import AdminPanel     from './components/AdminPanel'
import OrbitPanel     from './components/OrbitPanel'
import AuditPanel     from './components/AuditPanel'
import SensorExplorer from './components/SensorExplorer'
import './App.css'

type Tab = 'dashboard' | 'history' | 'orbit' | 'admin' | 'audit'
const MAX_HISTORY = 60

// ==========================================
// 1. COMPONENTE PRINCIPAL (Porteiro)
// ==========================================
export default function App() {
  const { user } = useAuth()
  const [authMode, setAuthMode] = useState<'login' | 'register'>('login')

  if (!user) {
    return authMode === 'register'
      ? <RegisterPage onBackToLogin={() => setAuthMode('login')} />
      : <LoginPage onRegister={() => setAuthMode('register')} />
  }

  return <MainDashboard />
}

// ==========================================
// 2. COMPONENTE DASHBOARD
// ==========================================
function MainDashboard() {
  const { user, logout, can } = useAuth()

  const [activeTab,       setActiveTab]      = useState<Tab>('dashboard')
  const [status,          setStatus]         = useState<SatelliteStatus>(DEFAULT_STATUS)
  const [telemetry,       setTelemetry]      = useState<TelemetryData | null>(null)
  const [telemetryHistory,setHistory]        = useState<TelemetryData[]>([])
  const [logs,            setLogs]           = useState<SatelliteLog[]>([])
  const [fieldTimestamps, setFieldTimestamps] = useState<Record<string, number>>({})
  const [otaStatus,       setOtaStatus]      = useState<OtaStatus>({
    inProgress: false, progress: 0, totalRecords: 0, sentRecords: 0, message: '',
  })
  const [wsOk, setWsOk] = useState(false)

  const seenLogIds = useRef(new Set<number>())

  const handleWsMessage = useCallback((msg: WsMessage) => {
    setWsOk(true)
    switch (msg.type) {
      case 'TELEMETRY': {
        const t = msg.data as TelemetryData
        setTelemetry(t)
        setHistory(prev => [...prev, t].slice(-MAX_HISTORY))
        setFieldTimestamps(prev => {
          const updated = { ...prev }
          for (const key of Object.keys(t) as Array<keyof TelemetryData>) {
            if (key === 'timestamp') continue
            if (t[key] != null) updated[key] = t.timestamp
          }
          return updated
        })
        break
      }
      case 'STATUS':
        setStatus(msg.data as SatelliteStatus)
        break
      case 'LOG': {
        const incoming = msg.data as SatelliteLog
        if (seenLogIds.current.has(incoming.id)) break
        seenLogIds.current.add(incoming.id)
        if (seenLogIds.current.size > 200) {
          const first = seenLogIds.current.values().next().value as number | undefined
          if (first !== undefined) seenLogIds.current.delete(first)
        }
        setLogs(prev => [...prev, incoming].slice(-10))
        break
      }
      case 'OTA_PROGRESS':
        setOtaStatus(msg.data as OtaStatus)
        break
    }
  }, [])

  useWebSocket(handleWsMessage)

  useEffect(() => {
    getStatus().then(setStatus).catch(() => {})
    getLogs().then(logs => {
      logs.forEach((l: SatelliteLog) => seenLogIds.current.add(l.id))
      setLogs(logs.slice(-10))
    }).catch(() => {})
    getOtaStatus().then(setOtaStatus).catch(() => {})
  }, [])

  // Dados para o explorador de sensores ao vivo (janela deslizante)
  const livePoints  = useMemo(() => telemetryToChartPoints(telemetryHistory), [telemetryHistory])
  const liveSensors = useMemo(() => sensorsPresent(livePoints), [livePoints])

  return (
    <div className="app">
      <header className="app-header">
        <div className="header-brand">
          <span className="header-icon">🛰️</span>
          <div>
            <h1>Satellite Ground Control</h1>
            <span className="header-sub">OBC Dashboard · via COM</span>
          </div>
        </div>

        <nav className="header-tabs">
          <button className={`tab-btn ${activeTab === 'dashboard' ? 'active' : ''}`} onClick={() => setActiveTab('dashboard')}>
            📡 Live
          </button>
          {can('history:read') && (
            <button className={`tab-btn ${activeTab === 'history' ? 'active' : ''}`} onClick={() => setActiveTab('history')}>
              📊 Histórico
            </button>
          )}
          <button className={`tab-btn ${activeTab === 'orbit' ? 'active' : ''}`} onClick={() => setActiveTab('orbit')}>
            🛰️ Órbita
          </button>
          {can('users:manage') && (
            <button className={`tab-btn ${activeTab === 'admin' ? 'active' : ''}`} onClick={() => setActiveTab('admin')}>
              ⚙️ Utilizadores
            </button>
          )}
          {can('audit:read') && (
            <button className={`tab-btn ${activeTab === 'audit' ? 'active' : ''}`} onClick={() => setActiveTab('audit')}>
              📜 Auditoria
            </button>
          )}
        </nav>

        <div className="header-badges">
          <div className={`badge ${wsOk ? 'badge-ok' : 'badge-off'}`}>
            <span className="badge-dot" /> WS {wsOk ? 'Connected' : 'Waiting'}
          </div>
          <div className={`badge ${status.connected ? 'badge-ok' : 'badge-off'}`}>
            <span className="badge-dot" />
            {status.connected ? `${status.comPort} · ${status.baudRate}` : 'No COM'}
          </div>
          <div className="badge badge-user" title={user?.permissions?.join(', ') || 'Sem permissões'}>
            👤 {user?.username}
          </div>
          <button className="logout-btn" onClick={logout}>Sair</button>
        </div>
      </header>

      {activeTab === 'dashboard' && (
        <main className="app-grid">
          <aside className="col-left">
            <ComPortPanel status={status} />
            <OtaPanel     otaStatus={otaStatus} connected={status.connected} />
          </aside>
          <section className="col-main">
            <TelemetryPanel
              telemetry={telemetry}
              history={telemetryHistory}
              fieldTimestamps={fieldTimestamps}
            />
            <div className="card">
              <div className="card-header"><span className="card-title">🔬 Explorador de Sensores (ao vivo)</span></div>
              <div className="card-body">
                <SensorExplorer
                  data={livePoints}
                  availableSensors={liveSensors}
                  title={`Últimas ${telemetryHistory.length} amostras`}
                />
              </div>
            </div>
            <LogsPanel logs={logs} />
          </section>
        </main>
      )}

      {activeTab === 'history' && (
        <main className="app-history"><HistoryWindow /></main>
      )}

      {activeTab === 'orbit' && (
        <main className="app-history"><OrbitPanel /></main>
      )}

      {activeTab === 'admin' && can('users:manage') && (
        <main className="app-history"><AdminPanel /></main>
      )}

      {activeTab === 'audit' && can('audit:read') && (
        <main className="app-history"><AuditPanel /></main>
      )}
    </div>
  )
}
