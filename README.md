# PS_OBC — Simulador de Sensores em Raspberry Pi Pico

Bancada de testes de hardware para o OBC do CubeSat PS_OBC, baseada em microcontroladores **RP2040 (Raspberry Pi Pico)**, usada para simular os sensores e periféricos do satélite quando o hardware real não está disponível.

## Objetivo

Permitir testar o firmware do OBC (branch [`main`](../../tree/main)) em condições realistas de barramento e temporização, sem depender do hardware final do satélite, simulando a dinâmica de uma órbita LEO.

## Simulação implementada

Um Pico atua como escravo I2C multi-endereço, expondo vários sensores simulados no mesmo barramento:

| Sensor      | Simulação                                                         |
|-------------|---------------------------------------------------------------------|
| GNSS        | Traçado de solo de uma órbita LEO a ~550 km (inclinação tipo ISS, 51.6°) |
| IMU         | Resíduos de micro-g, deriva de atitude e picos ocasionais de detumbling |
| Pressão     | Vácuo próximo, com ruído de sensor                                   |
| Temperatura | Ciclo entre exposição solar e eclipse (-20°C a +45°C)                |
| EPS         | Tensão/corrente a seguir o ciclo de iluminação solar                 |

Um segundo Pico simula o driver SPI do propulsor.

## Encaminhamento OTA

O Pico atua também como ponte entre o dashboard de solo (via USB/porta série) e o OBC (via UART), reencaminhando o firmware recebido em pacotes OTA de 128 bytes com verificação de tamanho, CRC32 e versão.

## Estrutura

```
cubesat_slave.c    Escravo I2C multi-endereço com simulação de sensores LEO
i2c_multi.c/.h     Driver I2C multi-endereço para o RP2040
pico2.c            Simulação do driver SPI do propulsor
```

## Stack

C · Raspberry Pi Pico SDK · RP2040 (I2C, SPI, UART, PIO)

## Relação com o projeto principal

Esta bancada permitiu validar a camada de drivers e a lógica de missão do OBC (branch `main`) e testar o fluxo completo de OTA do dashboard de solo (branch [`dashboard`](../../tree/dashboard)) sem acesso contínuo ao hardware final do CubeSat.
