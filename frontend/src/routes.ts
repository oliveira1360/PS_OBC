import type { ComponentType } from 'react'

/**
 * Configuração declarativa de rotas/tabs.
 *
 * Para adicionar uma nova página:
 *  1. Cria o componente em src/components/
 *  2. Adiciona uma entrada nesta lista
 *  → O header, a permissão e o rendering são tratados automaticamente.
 */
export interface RouteConfig {
  /** Chave única (usada como valor do estado activeTab). */
  key: string
  /** Texto no tab. */
  label: string
  /** Emoji / ícone antes do label. */
  icon: string
  /** Permissão necessária (null = qualquer utilizador autenticado). */
  permission: string | null
  /**
   * Lazy-loaded component.
   * Usa () => import('./components/Foo') para code-splitting, ou
   * referência directa para bundles pequenos.
   */
  component: ComponentType<Record<string, unknown>>
  /** CSS class extra a aplicar ao <main> quando esta rota está activa. */
  mainClass?: string
}
