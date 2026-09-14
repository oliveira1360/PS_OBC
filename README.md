# PS_OBC — On-Board Computer de um CubeSat

Firmware bare-metal em C para o computador de bordo (OBC) de um CubeSat, desenvolvido para o microcontrolador **ATSAMV71Q21B** (ARM Cortex-M7 a 300 MHz). Projeto final de curso em Engenharia (ISEL), com arquitetura orientada a sistemas embebidos críticos: sem SO, sem alocação dinâmica de memória, e execução determinística baseada num super-loop cooperativo.

Esta branch (`main`) contém o firmware de voo completo, o bootloader OTA e a documentação técnica do projeto.

## Arquitetura

- **Bare-metal puro**: sem RTOS, zero `malloc`/heap, alocação estática de memória.
- **Super-loop cooperativo**: cada periférico é gerido por uma máquina de estados finita (FSM) não-bloqueante, evitando qualquer ciclo de espera ativa no caminho crítico.
- **Camadas separadas**: `hal/` (registos e periféricos do MCU) → `drivers/` (UART, I2C, SPI, QSPI) → `peripherals/` (GNSS, IMU, EPS, propulsor, sensores de pressão/temperatura, TT&C) → `app/` (lógica de missão, seleção de modos, deteção de SEU).

## Periféricos e interfaces suportadas

| Interface | Uso                                | Nota                        |
|-----------|-------------------------------------|------------------------------|
| UART      | Rádio TT&C (uplink/downlink)        | até 1 MHz                    |
| I2C       | GNSS, IMU                           | 400 kHz                      |
| SPI       | Controlo do propulsor               | 12V                          |
| QSPI      | Memória flash externa (W25Q128)     | logs, telemetria, firmware OTA |
| ADC       | Sensores de pressão/força (FlexiForce) | —                          |

## Destaques técnicos

- **Bootloader OTA** (`ota/`): atualização de firmware over-the-air a partir de uma imagem gravada na flash externa, com verificação de integridade antes do salto para a aplicação.
- **Tolerância a SEU** (Single Event Upset): proteção por redundância modular tripla (TMR) aplicada às estruturas de dados dos sensores, relevante no contexto de radiação espacial.
- **Testes automatizados** (`tests/`): suite host-based (CTest) com mocks das camadas HAL para I2C, SPI, QSPI e UART, incluindo testes de stress e de tempo real.
- **Análise de WCET**: medição do pior caso de tempo de execução das funções críticas do super-loop, para validar o cumprimento de deadlines temporais.
- **Documentação de engenharia**: relatório técnico final, auditoria técnica cruzando o relatório com o código-fonte linha a linha, e preparação de defesa com mais de 100 perguntas técnicas antecipadas.

## Estrutura do repositório

```
src/            Código-fonte da aplicação (hal, drivers, peripherals, app)
inc/            Headers correspondentes
tests/          Testes unitários e de integração (host-based, com mocks)
ota/            Bootloader OTA e documentação do protocolo
```

## Stack

C (bare-metal, sem SO) · ARM Cortex-M7 · Make/CMake · MPLAB X

## Branches relacionadas

- [`dashboard`](../../tree/dashboard): aplicação web fullstack (Kotlin + React) para monitorização em solo e envio de atualizações OTA.
- [`pico_code`](../../tree/pico_code): simulador de sensores em Raspberry Pi Pico, usado como banco de testes de hardware na ausência do satélite real.
- [`OBC_board`](../../tree/OBC_board): validação incremental dos drivers de baixo nível diretamente na placa física.
