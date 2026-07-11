import { useEffect, useRef, useCallback } from 'react'
import { WsMessage } from '../types/satellite'

type MessageHandler = (msg: WsMessage) => void

export function useWebSocket(onMessage: MessageHandler) {
  const wsRef = useRef<WebSocket | null>(null)
  const reconnectTimer = useRef<ReturnType<typeof setTimeout> | null>(null)
  /**
   * Quando o componente desmonta fechamos o socket de propósito — mas o
   * onclose dispara na mesma e voltava a agendar uma reconexão, deixando
   * um WebSocket "fantasma" vivo. Esta flag corta esse ciclo.
   */
  const disposedRef = useRef(false)
  const handlerRef = useRef(onMessage)
  handlerRef.current = onMessage

  const connect = useCallback(() => {
    if (disposedRef.current) return
    try {
      const proto = window.location.protocol === 'https:' ? 'wss:' : 'ws:'
      const wsBase = import.meta.env.VITE_WS_URL ?? `${proto}//${window.location.host}`
      const ws = new WebSocket(`${wsBase}/ws/satellite`)
      wsRef.current = ws

      ws.onopen = () => console.log('[WS] Ligado ao backend')

      ws.onmessage = (event) => {
        try {
          const msg: WsMessage = JSON.parse(event.data)
          handlerRef.current(msg)
        } catch (e) {
          console.error('[WS] Erro ao fazer parse:', e)
        }
      }

      ws.onclose = () => {
        if (disposedRef.current) return
        console.log('[WS] Desligado, a reconectar em 3s...')
        reconnectTimer.current = setTimeout(connect, 3000)
      }

      ws.onerror = () => {
        ws.close()
      }
    } catch (e) {
      reconnectTimer.current = setTimeout(connect, 3000)
    }
  }, [])

  useEffect(() => {
    disposedRef.current = false
    connect()
    return () => {
      disposedRef.current = true
      if (reconnectTimer.current) clearTimeout(reconnectTimer.current)
      wsRef.current?.close()
    }
  }, [connect])

  return wsRef
}
