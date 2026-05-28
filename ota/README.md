# OTA Bootloader — PS_OBC CubeSat

Bootloader dedicado para suporte a actualizações Over-The-Air (OTA) no
microcontrolador **ATSAMV71Q21B** (ARM Cortex-M7).

## Estrutura

```
ota/
├── bootloader/
│   ├── inc/
│   │   ├── bootloader.h       # API principal + mapa de memória + ota_metadata_t
│   │   ├── flash_efc.h        # Driver EFC (flash interna — erase + write)
│   │   ├── qspi_boot.h        # Driver QSPI bloqueante (flash externa W25Q128)
│   │   ├── ota_verify.h       # Verificação CRC32 do firmware
│   │   └── system_samv71.h    # Init mínimo, cache, IRQs
│   ├── src/
│   │   ├── main.c             # Ponto de entrada
│   │   ├── bootloader.c       # Lógica principal (detecta OTA, aplica, salta)
│   │   ├── flash_efc.c        # Erase/write da flash interna via EFC
│   │   ├── qspi_boot.c        # Leitura/escrita bloqueante da flash externa
│   │   ├── ota_verify.c       # CRC32 (tabela + verificação incremental)
│   │   ├── system_samv71.c    # WDT, cache, NVIC, PMC
│   │   └── startup_samv71.c   # Tabela de vectores + Reset_Handler + BSS/data init
│   └── linker/
│       ├── samv71q21_boot.ld  # Linker do bootloader (flash @ 0x00400000, 64 KB)
│       └── samv71q21_app.ld   # Linker da aplicação  (flash @ 0x00410000, ~1984 KB)
└── docs/
    └── ota_protocol.md        # Protocolo completo, fluxos, configuração MPLAB X
```

## Mapa de memória

| Região            | Endereço                      | Tamanho  |
|:------------------|:------------------------------|:---------|
| Bootloader        | `0x00400000` – `0x0040FFFF`   | 64 KB    |
| Aplicação         | `0x00410000` – `0x005FFFFF`   | ~1984 KB |
| Metadados OTA     | `0x100000` (flash externa)    | 256 B    |
| Firmware OTA      | `0x100100` (flash externa)    | até 1 MB |

## Ficheiros existentes que precisam de ser modificados

> **Avisar antes de alterar qualquer ficheiro do projecto principal.**

| Ficheiro | Motivo |
|:---------|:-------|
| Linker script da aplicação | Alterar `ORIGIN` de `0x00400000` para `0x00410000` |
| `src/app/modeSelecter.c` → `otaMode()` | Implementar gravação do firmware + metadados na flash externa + reset |

Ver `docs/ota_protocol.md` para instruções detalhadas.

## Como compilar o bootloader

```
xc32-gcc -mthumb -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard \
  -ffunction-sections -fdata-sections \
  -Iota/bootloader/inc \
  ota/bootloader/src/*.c \
  -T ota/bootloader/linker/samv71q21_boot.ld \
  -Wl,--gc-sections \
  -o bootloader.elf

xc32-objcopy -O ihex bootloader.elf bootloader.hex
```

Ou criar um projecto MPLAB X separado com os ficheiros de `ota/bootloader/`.
