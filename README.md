# PS_OBC — Validação de Drivers em Hardware (OBC_board)

Esta branch documenta a fase de bring-up e validação incremental dos drivers de baixo nível do OBC diretamente na placa física com o microcontrolador **ATSAMV71Q21B**, antes da integração no firmware de voo completo (branch `main`).

## Objetivo

Validar em hardware real, driver a driver, a camada de comunicação entre o MCU e os periféricos do CubeSat, garantindo que cada barramento funciona de forma estável antes de compor o sistema completo.

## Progressão da validação

O desenvolvimento seguiu uma abordagem incremental, confirmando a estabilidade de cada interface isoladamente e depois em conjunto com as restantes:

1. Validação inicial do driver **I2C** em hardware.
2. Estabilização do **I2C**.
3. Validação combinada de **UART + I2C** a funcionar em simultâneo sem interferência.
4. Extensão para **UART + I2C + SPI** em conjunto.
5. Testes de hardware dedicados para consolidar os resultados.

Esta abordagem permite isolar problemas de temporização e de partilha de barramento cedo no desenvolvimento, em vez de os descobrir apenas na integração final.

## Âmbito

- `src/` e `inc/`: drivers de baixo nível (UART, I2C, SPI) e camada HAL.
- `tests/`: testes de validação em hardware.

## Stack

C (bare-metal) · ARM Cortex-M7 · MPLAB X

## Relação com o projeto principal

Os drivers validados nesta branch foram posteriormente integrados e consolidados no firmware de voo completo, disponível na branch [`main`](../../tree/main).
