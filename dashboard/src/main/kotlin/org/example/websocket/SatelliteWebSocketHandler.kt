package org.example.websocket

import com.fasterxml.jackson.databind.ObjectMapper
import org.example.domain.WsMessage
import org.springframework.stereotype.Component
import org.springframework.web.socket.CloseStatus
import org.springframework.web.socket.TextMessage
import org.springframework.web.socket.WebSocketSession
import org.springframework.web.socket.handler.TextWebSocketHandler
import java.util.concurrent.ConcurrentHashMap

@Component
class SatelliteWebSocketHandler(
    private val objectMapper: ObjectMapper
) : TextWebSocketHandler() {

    private val sessions = ConcurrentHashMap<String, WebSocketSession>()

    override fun afterConnectionEstablished(session: WebSocketSession) {
        sessions[session.id] = session
    }

    override fun afterConnectionClosed(session: WebSocketSession, status: CloseStatus) {
        sessions.remove(session.id)
    }

    fun broadcast(message: WsMessage) {
        val text = TextMessage(objectMapper.writeValueAsString(message))
        sessions.values.forEach { session ->
            try {
                if (session.isOpen) synchronized(session) { session.sendMessage(text) }
            } catch (_: Exception) {}
        }
    }
}
