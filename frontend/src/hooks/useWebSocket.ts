import { useEffect, useRef, useCallback } from 'react'
import { WsMessage } from '../types/satellite'

type MessageHandler = (msg: WsMessage) => void

export function useWebSocket(onMessage: MessageHandler) {
  const wsRef = useRef<WebSocket | null>(null)
  const reconnectTimer = useRef<ReturnType<typeof setTimeout> | null>(null)
  const handlerRef = useRef(onMessage)
  handlerRef.current = onMessage

  const connect = useCallback(() => {
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
    connect()
    return () => {
      if (reconnectTimer.current) clearTimeout(reconnectTimer.current)
      wsRef.current?.close()
    }
  }, [connect])

  return wsRef
}
