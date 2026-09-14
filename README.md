# PS_OBC — Dashboard de Controlo em Solo

Aplicação fullstack de ground control para o CubeSat PS_OBC: monitorização de telemetria em tempo real e envio de atualizações de firmware (OTA) via porta série (UART), com interface web.

## Stack

- **Backend**: Kotlin + Spring Boot (Gradle)
- **Frontend**: React + TypeScript + Vite
- **Comunicação em tempo real**: WebSocket
- **Ligação ao satélite**: porta COM/UART (simulada via Raspberry Pi Pico em bancada — ver branch [`pico_code`](../../tree/pico_code))

## Funcionalidades

- **Telemetria em tempo real**: receção e descodificação de dados do satélite (temperatura, tensão, corrente, bateria, altitude, posição GNSS, RSSI), transmitidos via WebSocket ao frontend.
- **Consola de logs**: distinção automática entre respostas normais, avisos e erros, a partir do stream série bruto.
- **Atualização OTA**: upload de um ficheiro `.hex` gerado no MPLAB IDE, com envio automático registo a registo via UART e acompanhamento do progresso em tempo real.
- **Gestão de porta COM**: listagem, ligação e desligamento de portas série diretamente pela interface, sem necessidade de ferramentas externas.
- **API REST** documentada para controlo da ligação série, consulta de estado dos subsistemas e gestão do processo OTA.

## Arquitetura

```
dashboard/    Backend (Kotlin + Spring Boot) — gestão da porta série, WebSocket, lógica OTA
frontend/     Frontend (React + Vite + TypeScript) — consola de telemetria e logs
```

O backend expõe a lógica de comunicação série por trás de uma interface de serviço (`ComPortService`), pensada para ser facilmente substituída por uma implementação equivalente quando o canal físico evoluir de UART por cabo para comunicação por antena, sem alterações no frontend.

## Como correr localmente

**Backend** (Java 21+, Gradle Wrapper incluído):
```bash
cd dashboard/dashboard
./gradlew bootRun
```

**Frontend** (Node.js 18+):
```bash
cd dashboard/frontend
npm install
npm run dev
```

Frontend disponível em `http://localhost:5173`, com proxy automático de `/api/*` para o backend em `http://localhost:8080`.

## Relação com o projeto principal

Este dashboard é a ferramenta de solo usada para testar e demonstrar o subsistema de telemetria e o bootloader OTA implementados no firmware do OBC, disponível na branch [`main`](../../tree/main).
