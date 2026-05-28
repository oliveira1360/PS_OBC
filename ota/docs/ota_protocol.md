# Protocolo OTA — PS_OBC CubeSat (ATSAMV71Q21B)

## 1. Visão geral

O sistema OTA usa um bootloader dedicado nos primeiros 64 KB da flash interna.
No arranque, o bootloader verifica se existe um firmware novo na flash externa (W25Q128)
e, caso exista e seja válido, aplica a actualização antes de arrancar a aplicação.

```
        Ground Station                  CubeSat (TTC → OBC)
             │                                  │
             │── 1. Firmware + metadata ────────►│  (pacotes OTA via TTC)
             │                                  │  otaMode() recebe e grava
             │                                  │  na flash externa
             │                                  │
             │── 2. Comando END_OTA ────────────►│
             │                                  │  OBC reinicia
             │                                  │  Bootloader verifica magic
             │                                  │  Aplica firmware
             │                                  │  Arranca nova aplicação
```

---

## 2. Mapa de memória

### Flash interna (ATSAMV71Q21B — 2 MB)

| Região       | Início       | Fim          | Tamanho  | Conteúdo              |
|:-------------|:-------------|:-------------|:---------|:----------------------|
| **Bootloader** | `0x00400000` | `0x0040FFFF` | 64 KB    | Bootloader OTA        |
| **Aplicação**  | `0x00410000` | `0x005FFFFF` | ~1984 KB | Firmware da aplicação |

### Flash externa — W25Q128 (16 MB)

| Região           | Início     | Fim        | Tamanho  | Conteúdo                  |
|:-----------------|:-----------|:-----------|:---------|:--------------------------|
| Reservado/Config | `0x000000` | `0x00FFFF` | 64 KB    | Config e reservado        |
| Telemetria       | `0x010000` | `0x0FFFFF` | ~960 KB  | Logs de telemetria        |
| **Metadados OTA**| `0x100000` | `0x1000FF` | 256 B    | `ota_metadata_t`          |
| **Firmware OTA** | `0x100100` | `0x1FFFFF` | ~1 MB    | Imagem do novo firmware   |

---

## 3. Estrutura de metadados OTA

Gravada em `0x100000` da flash externa (exactamente 256 bytes):

```c
typedef struct {
    uint32_t magic;           // OTA_MAGIC_PENDING = 0xAB12CD34 se válido
    uint32_t firmware_size;   // Tamanho do firmware em bytes
    uint32_t firmware_crc32;  // CRC32 do firmware (polinómio 0xEDB88320)
    uint32_t fw_version;      // Número de versão (livre)
    uint8_t  reserved[240];   // Padding até 256 bytes
} ota_metadata_t;
```

**Valores mágicos:**
- `0xAB12CD34` → imagem OTA válida, por aplicar (bootloader aplica na próxima reboot)
- `0xFFFFFFFF` → flash apagada, sem OTA pendente (arranque normal)

---

## 4. Fluxo do bootloader

```
Power-on Reset
      │
      ▼
Reset_Handler (startup_samv71.c)
  – copia .data para RAM
  – zera .bss
      │
      ▼
main() → system_boot_init()
  – desabilita WDT/RSWDT
  – configura flash wait states
  – activa clock QSPI
      │
      ▼
bootloader_run()
      │
      ▼
qspi_boot_init() → inicializa QSPI
      │
      ▼
Lê metadados (0x100000, 256 bytes)
      │
      ├─── magic != OTA_MAGIC_PENDING ──► jump_to_app()
      │
      ▼
firmware_size válido? ──── Não ──► apaga metadados → jump_to_app()
      │
      ▼
ota_verify_crc()
      │
      ├─── CRC FALHOU ──► apaga metadados → jump_to_app() (ou loop)
      │
      ▼
flash_efc_write_firmware(0x00410000, fw, size)
  – desabilita I-Cache
  – apaga sectores (8 KB cada, EPA command)
  – escreve páginas (512 B cada, WP command)
  – invalida I-Cache
      │
      ├─── ERRO ──► NÃO apaga metadados (retry na próxima reboot)
      │
      ▼
qspi_boot_erase_sector(0x100000) → limpa flag OTA
      │
      ▼
bootloader_jump_to_app()
  – desabilita IRQs e SysTick
  – SCB->VTOR = 0x00410000
  – MSP = app_vectors[0]
  – BX  app_vectors[1]  (Reset_Handler da aplicação)
```

---

## 5. Fluxo de envio OTA (lado do ground station)

### 5.1 Preparar o ficheiro

```python
import binascii, struct, os

firmware_path = "PS_OBC_app.bin"
fw_data = open(firmware_path, "rb").read()
fw_size = len(fw_data)
fw_crc  = binascii.crc32(fw_data) & 0xFFFFFFFF
fw_ver  = 2  # incrementar a cada release

print(f"Firmware: {fw_size} bytes, CRC32=0x{fw_crc:08X}, v{fw_ver}")
```

### 5.2 Construir metadados

```python
OTA_MAGIC_PENDING = 0xAB12CD34

metadata = struct.pack("<IIII",
    OTA_MAGIC_PENDING,
    fw_size,
    fw_crc,
    fw_ver
) + b'\xFF' * 240  # padding até 256 bytes

assert len(metadata) == 256
```

### 5.3 Protocolo de envio via TTC (pacotes OTA)

O campo `OTA_SYNC_WORD` (0xAA55) está definido em `board.h`.
Cada pacote OTA tem:

```
┌─────────────────────────────────────────────────────────────┐
│  Byte 0-1: Sync Word (0xAA55)                               │
│  Byte 2-3: Packet Index (big-endian, 0-indexed)             │
│  Byte 4-5: Total Packets (big-endian)                       │
│  Byte 6-133: Payload (128 bytes de dados)                   │
└─────────────────────────────────────────────────────────────┘
Total: 134 bytes = OTA_FULL_PACKET (definido em board.h)
```

Sequência de comandos TTC:
1. Enviar `CMD_START_OTA`
2. Enviar pacotes OTA com payload contendo:
   - Pacote 0: 128 bytes de metadados (parte 1)
   - Pacote 1: 128 bytes de metadados (parte 2, com padding)
   - Pacotes 2..N: firmware em chunks de 128 bytes
3. Enviar `CMD_END_OTA`
4. O OBC grava os metadados em `0x100000` e o firmware em `0x100100`
5. O OBC reinicia → bootloader detecta flag → aplica actualização

### 5.4 Gravação na flash externa (lado do OBC — `otaMode()`)

O `otaMode()` em `src/app/modeSelecter.c` deve:
1. Receber os pacotes OTA via TTC (já existente na FSM)
2. Gravar o firmware em `0x100100` (usando `qspi_write_async` do driver existente)
3. Gravar os metadados em `0x100000` **por último** (só após firmware completo)
4. Definir `magic = OTA_MAGIC_PENDING` nos metadados
5. Chamar `hal_system_reset()` para reiniciar

---

## 6. Configuração do projecto MPLAB X

### 6.1 Projecto do bootloader (novo)

Criar um projecto MPLAB X separado:
- **Device**: ATSAMV71Q21B
- **Compiler**: XC32 v5.10
- **Source files**: `ota/bootloader/src/*.c`
- **Include paths**: `ota/bootloader/inc/`
- **Linker script**: `ota/bootloader/linker/samv71q21_boot.ld`
- **XC32 flags**:
  ```
  -mthumb -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard
  -ffunction-sections -fdata-sections
  -Wl,--gc-sections
  ```
- **Output**: `bootloader.hex` — programar em primeiro lugar via JTAG

### 6.2 Projecto da aplicação (PS_OBC.X — existente)

**⚠ ATENÇÃO — Ficheiros existentes que necessitam de modificação:**

| Ficheiro | Alteração necessária |
|:---------|:--------------------|
| Linker script da aplicação | Mudar origem da flash de `0x00400000` para `0x00410000` |
| `src/app/modeSelecter.c` → `otaMode()` | Implementar gravação de firmware + metadados + reset |

Para o linker script da aplicação, usar `ota/bootloader/linker/samv71q21_app.ld`
ou editar o script existente alterando `ORIGIN = 0x00400000` para `ORIGIN = 0x00410000`.

**AVISO:** Estas alterações são necessárias para que a aplicação arranque correctamente
a partir de `0x00410000`. Sem estas alterações, o bootloader salta para `0x00410000`
mas a tabela de vectores da aplicação estará em `0x00400000` (posição errada).

### 6.3 Programação

```
1. Programar bootloader.hex → ocupa 0x00400000-0x0040FFFF
2. Programar PS_OBC.hex     → ocupa 0x00410000-0x005FFFFF
   (ou enviar via OTA após o passo 1)
```

---

## 7. Verificação rápida

### Verificar que o bootloader está correcto

Com o debugger MPLAB X / Atmel-ICE:
- Pôr breakpoint em `bootloader_run()` → confirma arranque
- Pôr breakpoint em `bootloader_jump_to_app()` → confirma salto para app
- Verificar `SCB->VTOR` (endereço `0xE000ED08`) = `0x00410000` antes do salto

### Testar OTA sem ground station

Usar o script Python:
```python
# Escrever metadados de teste na flash externa via debugger
# ou via JTAG (endereço externo via QSPI memory-mapped 0x80100000)
```

---

## 8. Segurança e robustez

| Cenário                    | Comportamento                                    |
|:---------------------------|:-------------------------------------------------|
| Sem flag OTA               | Arranca aplicação directamente (< 1 ms overhead) |
| CRC inválido               | Apaga metadados, arranca firmware anterior        |
| Firmware demasiado grande  | Apaga metadados, arranca firmware anterior        |
| Falha de escrita na flash  | Mantém metadados (retry na próxima reboot)       |
| Flash externa inacessível  | Arranca aplicação existente                      |
| Sem aplicação válida       | Loop de segurança (aguarda programação JTAG)     |
| WDT activo                 | Desabilitado no arranque do bootloader           |
