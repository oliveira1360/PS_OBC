package org.example.websocket

import com.fasterxml.jackson.databind.ObjectMapper
import org.example.model.WsMessage
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
        println("[WS] Client connected: ${session.id}. Total: ${sessions.size}")
    }

    override fun afterConnectionClosed(session: WebSocketSession, status: CloseStatus) {
        sessions.remove(session.id)
        println("[WS] Client disconnected: ${session.id}. Total: ${sessions.size}")
    }

    fun broadcast(message: WsMessage) {
        val json = objectMapper.writeValueAsString(message)
        val textMessage = TextMessage(json)
        sessions.values.forEach { session ->
            try {
                if (session.isOpen) {
                    synchronized(session) {
                        session.sendMessage(textMessage)
                    }
                }
            } catch (e: Exception) {
                println("[WS] Error sending to ${session.id}: ${e.message}")
            }
        }
    }
}
