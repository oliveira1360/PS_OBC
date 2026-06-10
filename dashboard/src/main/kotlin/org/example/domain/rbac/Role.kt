package org.example.domain.rbac

/**
 * Enum de Roles para RBAC.
 * Cada role tem um conjunto de permissões pré-definidas.
 */
enum class Role(val label: String, val permissions: Set<Permission>) {

    ADMIN(
        label       = "Administrador",
        permissions = Permission.entries.toSet()
    ),

    OPERATOR(
        label       = "Operador",
        permissions = setOf(
            Permission.DASHBOARD_READ,
            Permission.HISTORY_READ,
            Permission.HISTORY_EXPORT,
            Permission.OTA_WRITE,
            Permission.COMMANDS_WRITE
        )
    ),

    VIEWER(
        label       = "Visualizador",
        permissions = setOf(
            Permission.DASHBOARD_READ,
            Permission.HISTORY_READ
        )
    );

    fun hasPermission(permission: Permission): Boolean = permission in permissions
}
