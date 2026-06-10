import { useState, useRef, useEffect } from 'react'
import type { OtaStatus } from '../types/satellite'
import {
  uploadFirmware, listPorts, scanBluetooth,
  type Transport, type BluetoothScanResult,
} from '../api/satellite'

interface Props {
  otaStatus: OtaStatus
  connected: boolean
}

const BAUD_OPTIONS = [9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600]

export default function OtaPanel({ otaStatus }: Props) {
  const [file,       setFile]      = useState<File | null>(null)
  const [transport,  setTransport] = useState<Transport>('COM_PORT')
  const [otaPort,    setOtaPort]   = useState('')
  const [baudRate,   setBaudRate]  = useState(115200)
  const [ports,      setPorts]     = useState<string[]>([])
  const [btScan,     setBtScan]    = useState<BluetoothScanResult | null>(null)
  const [scanning,   setScanning]  = useState(false)
  const [sending,    setSending]   = useState(false)
  const [localMsg,   setLocalMsg]  = useState('')
  const fileRef = useRef<HTMLInputElement>(null)

  // Load COM ports on mount
  useEffect(() => {
    listPorts().then(p => {
      setPorts(p)
      if (p.length > 0 && !otaPort) setOtaPort(p[0])
    }).catch(() => {})
  }, [])

  const handleScanBluetooth = async () => {
    setScanning(true)
    setBtScan(null)
    setLocalMsg('')
    try {
      const result: BluetoothScanResult = await scanBluetooth()
      setBtScan(result)
      // Auto-select the first Bluetooth COM port if found
      if (result.bluetoothComPorts.length > 0) {
        setOtaPort(result.bluetoothComPorts[0])
      }
    } catch {
      setLocalMsg('Erro ao pesquisar dispositivos Bluetooth.')
    }
    setScanning(false)
  }

  const handleFile = (e: React.ChangeEvent<HTMLInputElement>) => {
    const f = e.target.files?.[0]
    if (f) { setFile(f); setLocalMsg('') }
  }

  const handleDrop = (e: React.DragEvent) => {
    e.preventDefault()
    const f = e.dataTransfer.files?.[0]
    if (f) { setFile(f); setLocalMsg('') }
  }

  const handleSend = async () => {
    if (!file || !otaPort) return
    setSending(true)
    setLocalMsg('')
    try {
      const result = await uploadFirmware(file, transport, otaPort, baudRate)
      setLocalMsg(result.message ?? 'OTA iniciado')
    } catch {
      setLocalMsg('Falha no upload — o backend está a correr?')
    }
    setSending(false)
  }

  const isRunning = otaStatus.inProgress
  const canSend   = !!file && !!otaPort && !isRunning && !sending

  // Merged port list for Bluetooth tab: BT ports first, then all others
  const btPortList = btScan
    ? [...new Set([...btScan.bluetoothComPorts, ...btScan.allComPorts])]
    : ports

  return (
    <div className="card">
      <div className="card-header">
        <span className="card-title">🚀 OTA Update</span>
      </div>
      <div className="card-body">

        {/* Transport selector */}
        <div className="form-group">
          <label>Transporte</label>
          <div className="ota-transport-tabs">
            <button
              className={`ota-tab ${transport === 'COM_PORT' ? 'ota-tab-active' : ''}`}
              onClick={() => setTransport('COM_PORT')}
            >🔌 COM Port</button>
            <button
              className={`ota-tab ${transport === 'BLUETOOTH' ? 'ota-tab-active' : ''}`}
              onClick={() => { setTransport('BLUETOOTH'); setBtScan(null) }}
            >📶 Bluetooth (HC-05)</button>
          </div>
        </div>

        {/* ── Bluetooth tab content ──────────────────────────────────────── */}
        {transport === 'BLUETOOTH' && (
          <div className="bt-panel">

            {/* Pairing instructions */}
            <div className="bt-guide">
              <div className="bt-guide-title">📋 Como emparelhar o HC-05</div>
              <ol className="bt-guide-steps">
                <li>Liga o módulo HC-05 (LED a piscar = modo discoverable)</li>
                <li>Abre <strong>Definições → Bluetooth → Adicionar dispositivo</strong></li>
                <li>Selecciona <strong>HC-05</strong> na lista</li>
                <li>Usa o PIN <strong>1234</strong> ou <strong>0000</strong></li>
                <li>Após emparelhamento, clica <strong>"Pesquisar"</strong> abaixo</li>
              </ol>
            </div>

            {/* Scan button */}
            <button
              className="btn btn-sm btn-full"
              onClick={handleScanBluetooth}
              disabled={scanning}
              style={{ marginBottom: 8 }}
            >
              {scanning ? '🔍 A pesquisar…' : '🔍 Pesquisar dispositivos Bluetooth'}
            </button>

            {/* Scan results */}
            {btScan && (
              <div className="bt-results">
                {btScan.pairedDevices.length > 0 ? (
                  <>
                    <div className="bt-results-title">Dispositivos emparelhados:</div>
                    {btScan.pairedDevices.map((d, i) => (
                      <div key={i} className="bt-device-row">
                        <span className={`bt-dot ${d.status === 'OK' ? 'bt-dot-ok' : 'bt-dot-off'}`} />
                        {d.name}
                      </div>
                    ))}
                  </>
                ) : (
                  <div className="bt-no-devices">
                    Nenhum dispositivo emparelhado encontrado.<br />
                    Segue os passos acima para emparelhar o HC-05.
                  </div>
                )}

                {btScan.bluetoothComPorts.length > 0 ? (
                  <div className="bt-com-found">
                    ✅ Portas COM Bluetooth detectadas: {btScan.bluetoothComPorts.join(', ')}
                  </div>
                ) : (
                  <div className="bt-com-missing">
                    ⚠️ Nenhuma porta COM Bluetooth encontrada ainda.<br />
                    Após emparelhar, volta a pesquisar.
                  </div>
                )}
              </div>
            )}
          </div>
        )}

        {/* Port + baud rate (shown for both transports) */}
        <div className="form-group">
          <label>
            {transport === 'BLUETOOTH' ? 'Porta COM do HC-05' : 'Porta COM (OTA)'}
          </label>
          <div style={{ display: 'flex', gap: 6 }}>
            <select value={otaPort} onChange={e => setOtaPort(e.target.value)} style={{ flex: 1 }}>
              {(transport === 'BLUETOOTH' ? btPortList : ports).length === 0
                ? <option value="">Nenhuma porta disponível</option>
                : (transport === 'BLUETOOTH' ? btPortList : ports).map(p => (
                    <option key={p} value={p}>
                      {btScan?.bluetoothComPorts.includes(p) ? `📶 ${p}` : p}
                    </option>
                  ))
              }
            </select>
            <select
              value={baudRate}
              onChange={e => setBaudRate(Number(e.target.value))}
              style={{ width: 96 }}
            >
              {BAUD_OPTIONS.map(b => <option key={b} value={b}>{b}</option>)}
            </select>
          </div>
          {transport === 'BLUETOOTH' && (
            <div className="ota-transport-hint">
              O HC-05 usa por defeito <strong>9600</strong> ou <strong>38400</strong> baud.
              Verifica a configuração do teu módulo.
            </div>
          )}
        </div>

        {/* File dropzone */}
        <div
          className="dropzone"
          onClick={() => fileRef.current?.click()}
          onDrop={handleDrop}
          onDragOver={e => e.preventDefault()}
        >
          <input
            ref={fileRef}
            type="file"
            accept=".bin,.BIN,.hex,.HEX"
            style={{ display: 'none' }}
            onChange={handleFile}
          />
          {file ? (
            <>
              <span className="dropzone-file">
                {file.name.toLowerCase().endsWith('.bin') ? '🗂' : '📄'} {file.name}
              </span>
              <span className="dropzone-size"> ({(file.size / 1024).toFixed(1)} KB)</span>
            </>
          ) : (
            <span className="dropzone-hint">
              Clica ou arrasta o ficheiro <strong>.bin</strong> ou .hex
            </span>
          )}
        </div>

        {/* Progress bar */}
        {isRunning && (
          <div className="ota-progress">
            <div className="ota-progress-label">
              <span>{otaStatus.message}</span>
              <span>{otaStatus.sentRecords}/{otaStatus.totalRecords}</span>
            </div>
            <div className="progress-track">
              <div className="progress-fill" style={{ width: `${otaStatus.progress}%` }} />
            </div>
            <div className="progress-pct">{otaStatus.progress}%</div>
          </div>
        )}

        {/* Result messages */}
        {!isRunning && otaStatus.success === true && (
          <div className="ota-result success">✅ {otaStatus.message}</div>
        )}
        {!isRunning && otaStatus.success === false && otaStatus.message && (
          <div className="ota-result error">❌ {otaStatus.message}</div>
        )}
        {localMsg && <div className="ota-result warn">{localMsg}</div>}

        <button
          className="btn btn-success btn-full"
          onClick={handleSend}
          disabled={!canSend}
        >
          {isRunning
            ? `A enviar… ${otaStatus.progress}%`
            : transport === 'BLUETOOTH' ? '📶 Enviar via Bluetooth' : '⬆ Enviar via COM'}
        </button>
      </div>
    </div>
  )
}
