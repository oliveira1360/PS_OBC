package org.example.domain.rbac

/**
 * Permissões granulares do sistema — usadas tanto no backend (Spring Security)
 * como enviadas ao frontend para controlo de visibilidade de UI.
 */
enum class Permission(val key: String, val description: String) {
    DASHBOARD_READ ("dashboard:read",  "Ver dashboard em tempo real"),
    HISTORY_READ   ("history:read",   "Consultar histórico de sensores"),
    HISTORY_EXPORT ("history:export", "Exportar histórico para CSV/JSON"),
    OTA_WRITE      ("ota:write",      "Iniciar actualização OTA"),
    COMMANDS_WRITE ("commands:write", "Enviar comandos ao OBC"),
    USERS_MANAGE   ("users:manage",   "Criar/editar/remover utilizadores"),
    AUDIT_READ     ("audit:read",     "Ver registo de auditoria");

    companion object {
        fun fromKey(key: String): Permission? = entries.find { it.key == key }
    }
}
