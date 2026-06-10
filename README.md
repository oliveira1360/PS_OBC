# Satellite Ground Control Dashboard

Aplicação fullstack para monitorização e atualização OTA de satélites via porta COM (UART).

---

## Estrutura

```
dashboard/
├── dashboard/    ← Backend (Kotlin + Spring Boot)
├── frontend/     ← Frontend (React + Vite + TypeScript)
└── README.md
```

---

## Arrancar o Backend

### Pré-requisitos
- Java 21+
- (o Gradle Wrapper está incluído — não precisas de instalar Gradle)

### Passos

```bash
cd dashboard/dashboard
./gradlew bootRun          # Linux/Mac
gradlew.bat bootRun        # Windows
```

O backend fica disponível em `http://localhost:8080`.

---

## Arrancar o Frontend

### Pré-requisitos
- Node.js 18+

### Passos

```bash
cd dashboard/frontend
npm install
npm run dev
```

O frontend fica disponível em `http://localhost:5173`.

> O Vite faz proxy automático de `/api/*` para o backend em `:8080`.

---

## Ligar ao Satélite via COM

1. Abre o browser em `http://localhost:5173`
2. No painel **Porta COM**, clica em **↻ Actualizar** para listar as portas
3. Seleciona a porta correta (ex: `COM3`) e o baud rate
4. Clica em **⚡ Ligar**

O dashboard começa a receber dados em tempo real via WebSocket.

---

## Formato de dados esperado (UART)

O backend aceita duas formas de dados enviados pelo satélite:

### JSON (telemetria automática)
```json
{"temp": 23.4, "volt": 3.7, "curr": 0.15, "bat": 82, "alt": 400.0, "lat": 38.7, "lon": -9.1, "rssi": -87}
```
Campos suportados: `temp`/`temperature`, `volt`/`voltage`, `curr`/`current`, `bat`/`battery`, `alt`/`altitude`, `lat`, `lon`/`lng`, `rssi`/`signal`

### Texto livre (logs)
Qualquer linha que não seja JSON é tratada como log:
- Linhas que começam por `ERR` ou `ERROR` → tipo ERROR (vermelho)
- Linhas que começam por `WARN` → tipo WARNING (amarelo)
- Restantes → tipo RESPONSE (verde)

---

## OTA — Enviar Firmware

1. Faz o build do firmware no MPLAB IDE
2. Localiza o ficheiro `.hex` gerado
3. No dashboard, abre o painel **OTA Update**
4. Arrasta ou seleciona o ficheiro `.hex`
5. Clica em **⬆ Enviar Firmware via OTA**

O backend envia os registos Intel HEX linha a linha via UART (com delay de 20ms entre registos para o bootloader processar cada um).

---

## API REST (porta 8080)

| Método | Endpoint                | Descrição                        |
|--------|-------------------------|----------------------------------|
| GET    | `/api/com/ports`        | Lista portas COM disponíveis     |
| POST   | `/api/com/connect`      | Liga à porta `{port, baudRate}`  |
| POST   | `/api/com/disconnect`   | Desliga da porta atual           |
| GET    | `/api/com/status`       | Estado da ligação COM            |
| POST   | `/api/com/send`         | Envia bytes raw `{hex: "AABB…"}` |
| GET    | `/api/satellite/status` | Estado dos subsistemas           |
| GET    | `/api/satellite/telemetry` | Última telemetria             |
| GET    | `/api/satellite/logs`   | Últimas 200 entradas do log      |
| POST   | `/api/ota/upload`       | Upload e envio de firmware .hex  |
| GET    | `/api/ota/status`       | Estado do OTA em curso           |

## WebSocket

`ws://localhost:8080/ws/satellite`

Mensagens recebidas pelo frontend:
```json
{ "type": "TELEMETRY",    "data": { ... } }
{ "type": "STATUS",       "data": { ... } }
{ "type": "LOG",          "data": { ... } }
{ "type": "OTA_PROGRESS", "data": { ... } }
```

---

## Próximos passos (antenas)

Quando for adicionado o suporte por antenas, bastará criar um novo serviço (ex: `AntennaService.kt`) que reimplemente a mesma interface que o `ComPortService` e um novo controller. O frontend não precisará de alterações — os dados chegam pelo mesmo WebSocket.
