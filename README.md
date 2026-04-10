## 📅 Roadmap e Planeamento de Tarefas (To-Do)

O desenvolvimento deste On-Board Computer (OBC) está dividido em 5 fases principais, alinhadas com os marcos de entrega (Milestones) do projeto. A arquitetura segue uma abordagem *bare-metal* rigorosa, assente em máquinas de estados e execução não-bloqueante.

### Fase 1: Especificação, Clocks e Timers (Fev - Mar)
**Objetivo:** Estabelecer a fundação bare-metal e garantir o determinismo do sistema.
- [x] Seleção do Microcontrolador (ATSAMV71Q21 - Cortex-M7 a 300MHz).
- [ ] Configuração do sistema de Clocks e PLLs do MCU.
- [ ] Configuração dos Timers de hardware para base de tempo.
- [ ] Implementação do ciclo principal de controlo (Super-loop / Cooperative Scheduler).
- [ ] Garantir alocação estática de memória (Zero uso de `malloc`/heap).
- 🚩 **M1: Entrega da Proposta - 09/03**

### Fase 2: Drivers de Baixo Nível (Mar - Abr)
**Objetivo:** Desenvolver a Hardware Abstraction Layer (HAL) puramente não-bloqueante.
- [ ] **UART:** Driver para sistema TT&C (Rádio) a 3.3/5V (até 1 MHz, amostragem 1 Hz).
- [ ] **I2C:** Driver para GNSS (GPS) a 400 kHz (amostragem 1 Hz).
- [ ] **I2C:** Driver para IMU a 400 kHz (amostragem 1-400 Hz).
- [ ] **SPI:** Driver para o Propulsor a 12V (amostragem 10 Hz).
- [ ] **QSPI:** Driver para Memória Externa a 10 MHz (amostragem 1 Hz).
- [ ] **ADC:** Leitura analógica para sensores de pressão e força (FlexiForce).
- [ ] Implementação de *timeouts* manuais em todas as operações de I/O para evitar bloqueios do ciclo principal.
- 🚩 **M2: Relatório de Progresso - 27/04**

### Fase 3: Lógica de Controlo / FSM (Abr - Jun)
**Objetivo:** Orquestrar a concorrência de periféricos em segurança.
- [ ] Desenho das Máquinas de Estados Finitas (FSM) para cada periférico.
- [ ] Implementação da lógica de gestão de concorrência cooperativa.
- [ ] Priorização de tarefas críticas (Gestão de Energia e Controlo de Atitude).
- [ ] Tratamento de exceções e prevenção de condições de corrida (*Race Conditions*).

### Fase 4: Integração e Telemetria (Mai - Jun)
**Objetivo:** Unir os módulos e processar dados de missão.
- [ ] Integração do módulo de recolha e envio de Telemetria (`ttc_data_t`).
- [ ] Processamento e descodificação de comandos de voo (Uplink).
- [ ] Implementação do sistema de monitorização da saúde do satélite (SOH).
- [ ] Mecanismo de Atualização OTA (*Over-The-Air*).
- 🚩 **M3: Versão Beta / Testes de Integração - 01/06**

### Fase 5: Validação e Relatório Final (Abr - Jul)
**Objetivo:** Testes rigorosos e documentação.
- [ ] Criação de *Mocks* de software para testar lógica quando o hardware real falhar ou não estiver disponível.
- [ ] Validação de *Jitter* e deriva temporal das taxas de amostragem no super-loop.
- [ ] Testes de robustez aos *timeouts* (simular falhas e ruído nos sensores I2C/SPI e UART).
- [ ] Escrita e revisão do Relatório Final do Projeto.
- 🚩 **M4: Entrega Final do Projeto - 11/07**
