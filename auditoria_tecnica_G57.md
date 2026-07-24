# Auditoria Técnica — PS_OBC (G57)

**Âmbito:** relatório final (rfG57.pdf, 65 pp.), poster (cG57.pdf), manual (infoG57.pdf), firmware (`PS_OBC.X`), bootloader (`ota/bootloader`), dashboard (Kotlin/React) e bancada Pico (`solder_test`).
**Método:** leitura integral dos documentos e verificação cruzada, ficheiro a ficheiro, das afirmações do relatório contra o código-fonte real.
**Data:** 2026-07-12

Legenda de impacto: **ALTO** / **MÉDIO** / **BAIXO**. Cada achado indica: descrição → porquê é problema → impacto → correção sugerida.

---

## 1. Achados críticos (relatório contradiz o código)

### C1. Endereço da aplicação: relatório diz 0x00410000, o código usa 0x00420000
- O relatório afirma em todo o Capítulo 6 (Tabela 6.1, Tabela 6.3, §6.4.3, Listagem 6.2, Listagem 6.6, §6.8) que a aplicação reside em `0x00410000`. No código, `bootloader.h:47` define `APP_START_ADDR = 0x00420000UL` e o linker `samv71q21_app.ld:34` tem `ORIGIN(rom) = 0x00420000`. Os próprios comentários do header e do linker ainda dizem 0x00410000.
- **Porquê:** o mapa de memória é a espinha dorsal do subsistema OTA. Um leitor (ou um colega que retome o projeto) que programe/depure com base no relatório vai trabalhar com endereços errados de VTOR, proteção EFC e salto do bootloader.
- **Impacto: ALTO.**
- **Correção:** decidir o endereço definitivo, atualizar relatório, comentários de código e linker de forma coerente; idealmente derivar tudo de uma única constante partilhada entre bootloader e aplicação.

### C2. Linker da aplicação ultrapassa o fim físico da flash
- `ORIGIN = 0x00420000` com `LENGTH = 0x001F0000` dá fim em `0x00610000`, mas a flash interna de 2 MB termina em `0x00600000`. `APP_MAX_SIZE = 0x001F0000` tem o mesmo erro (o máximo real a partir de 0x420000 é 0x1E0000). O `LENGTH` não foi reduzido quando o `ORIGIN` passou de 0x410000 para 0x420000.
- **Porquê:** o linker deixa de detetar quando a aplicação cresce para além da flash física; o bootloader pode tentar apagar/escrever páginas inexistentes.
- **Impacto: ALTO** (latente — só se manifesta quando o binário crescer).
- **Correção:** `LENGTH = 0x001E0000` no linker e `APP_MAX_SIZE = 0x001E0000UL` no bootloader.

### C3. Pacote de telemetria: 84 bytes no relatório, 75 bytes na realidade
- RF3, Tabela 5.5 e Tabela 8.2 descrevem um downlink de 84 bytes (`ttc_data_t` completo, com ranging, ota_active, last_command, cmd_status). O código (`ttc_send_telemetry()` em `ttc.c`) transmite um frame de **75 bytes**: `[0x20][18 floats BE][estado][checksum XOR]` — e o dashboard (`RawBinaryTelemetryCodec.kt`, `FRAME_LEN = 75`) espera exatamente isso. A `ttc_data_t` nunca é transmitida.
- **Porquê:** o requisito RF3 é dado como "Verificado" com base num pacote que não existe no fio. Campos anunciados (ranging, estado do comando) nunca chegam ao solo.
- **Impacto: ALTO** (integridade da matriz de rastreabilidade).
- **Correção:** documentar o frame real de 75 bytes (ou implementar de facto o pacote de 84 bytes) e corrigir RF3/Tabelas 5.5 e 8.2.

### C4. `BATTERY_STATUS` nunca é alimentada por dados reais — a FSM de modos por bateria é código morto em voo
- `BATTERY_STATUS` é inicializada a `100.0f` (`sensors.c:16`) e só é alterada por código de teste (`unit_tests.c`, `seu_injection_test.c`) e por `stateCheck()` (−20 %/chamada), que apenas é invocada dentro do teste WCET. Nenhum periférico (EPS incluído) deriva a percentagem de bateria.
- **Porquê:** as transições centrais do sistema — safe_mode a <30 %, ultra_low_power a <15 %, guarda de 70 % para OTA (Tabela 4.2, §4.4) — nunca podem disparar em operação real. O relatório apresenta RF5 como "Verificado".
- **Impacto: ALTO.**
- **Correção:** calcular o estado de carga a partir da leitura da EPS (ou do simulador Pico) e atualizá-lo em `sensors_tick()`; declarar honestamente no relatório que a percentagem de bateria é atualmente simulada/estática.

### C5. Proteção TMR do estado de missão não está ligada ao caminho de voo
- §4.5 afirma: "a variável de estado mission_state não é uma simples variável enum, mas um valor protegido por TMR no módulo seu.c (…) leitura via mission_state_get() a cada acesso". No código, a FSM de modos usa o enum global simples `States state` (`modeSelecter.c:8`), sem qualquer proteção; `mission_state_get()` só aparece em `value_dump.c` (código de teste). A TMR das *estruturas de sensores* (`seu_data.c`) está, essa sim, integrada em `sensors_tick()`.
- **Porquê:** é uma afirmação de fiabilidade (mecanismo anti-SEU no estado mais crítico) que não corresponde à implementação. Um SEU em `state` transita o satélite para um modo arbitrário sem deteção.
- **Impacto: ALTO.**
- **Correção:** registar `state` em `seu_data_protect()` (basta 1 linha em `sensors_seu_register()`) ou usar de facto a API de `seu.c`; em alternativa, corrigir o relatório.

### C6. "A verificação de saúde tem prioridade máxima" — falso em `ota_mode`
- §4.4 afirma que a verificação de saúde "precede qualquer lógica de modo". Em `getMode(ota_mode)` a única condição é `!OTA_REQUESTED`: nem `isSystemSafe()` nem bateria crítica são avaliadas durante uma OTA. Uma queda de tensão/bateria a meio da transferência mantém o sistema em ota_mode.
- **Porquê:** contradiz a justificação de segurança da guarda dos 70 % ("nenhuma escrita na flash é iniciada com energia insuficiente" — mas pode *continuar* com energia insuficiente).
- **Impacto: ALTO** (segurança da lógica de missão).
- **Correção:** em `ota_mode`, avaliar `battery_critical`/`isSystemSafe()` e abortar a OTA de forma limpa (via `OTA_SM_ERROR`) antes de transitar.

### C7. Flags de validade dos dados de sensores não existem
- §5.2.1 descreve um mecanismo em que "cada estrutura de dados de periférico mantém um campo de estado que indica se os dados foram atualizados com sucesso" e a lógica de missão "distingue dados atualizados de dados obsoletos". `eps_data_t`, `gnss_data_t`, `imu_data_t`, etc. não têm nenhum campo de estado; `isSystemSafe()` compara diretamente os últimos valores retidos, sem noção de staleness.
- **Porquê:** um sensor EPS silenciosamente avariado congela `voltage/current` num valor "saudável" e a monitorização perde eficácia — precisamente o cenário que o relatório afirma tratar.
- **Impacto: ALTO.**
- **Correção:** adicionar `uint8_t valid` / timestamp de última atualização a cada estrutura, preenchido pelos callbacks (0/−1), e tratá-lo em `isSystemSafe()`; ou reescrever §5.2.1.

### C8. Matriz de testes declara "Implementados" testes que são esqueletos vazios
- Tabela 8.1 marca `tests/app/` e `tests/peripherals/` como "Implementados". No código, `test_fsm.c` é um bloco de TODOs sem um único teste, e todos os `tests/peripherals/*.c` imprimem "Skeleton (não implementado)". O próprio `tests/Makefile` os descreve como "skeletons". A Tabela 8.2 usa-os como evidência: RF2 "Verificado" via `tests/peripherals/`, RF5 "Verificado" via `test_fsm.c`. `test_utils.h` (Tabela 8.1) não existe. Os testes de drivers (611–1184 linhas) são reais e sólidos.
- **Porquê:** num contexto profissional/ECSS, declarar verificação com evidência inexistente é a falha mais grave possível numa matriz de rastreabilidade.
- **Impacto: ALTO.**
- **Correção:** implementar os testes de FSM e periféricos (o TODO de `test_fsm.c` já é um bom plano de teste) ou reclassificar Tabelas 8.1/8.2 como "Planeado".

### C9. Números de WCET inconsistentes e fisicamente implausíveis
- §8.5.2: "mission_lifecycle() apresenta um WCET de 3075 ciclos, aproximadamente 497 µs a 12 MHz" — 3075 ciclos a 12 MHz são **256 µs**. "Com a margem de 30 %, o valor sobe para 3099 ciclos, aproximadamente 338 µs" — 3075×1,3 = 3998 ciclos ≈ 333 µs; e 3099 ciclos seriam 258 µs. A Tabela 4.4 usa ainda outro valor ("WCET ≈289 µs"). Além disso, `mission_lifecycle()` (3075 ciclos) engloba `sensors_tick()` (4594 ciclos) — o contentor não pode ter WCET inferior ao conteúdo, e a discrepância não é explicada.
- **Porquê:** a Secção 8.5 é a validação quantitativa central do projeto (RNF1/RNF2); com aritmética errada, a conclusão "cumpre o deadline com margem de 30 %" deixa de ser verificável pelo leitor.
- **Impacto: ALTO.**
- **Correção:** repetir a medição, publicar a tabela bruta (min/média/máx por função, IRQ on/off — o `wcet_test.c` já a produz) e refazer as conversões ciclos→µs; explicar o caso mission_lifecycle vs sensors_tick (provavelmente medições em modos diferentes).

### C10. Caminhos bloqueantes reais dentro da FSM OTA e do super-loop, além do printf reconhecido
- O relatório reconhece apenas o printf de telemetria (416 ms) como caminho bloqueante e afirma (§9.1.2) que "a configuração de voo não é afetada". No código:
  - `ota_crc32()` (`modes.c`) calcula o CRC de **toda a imagem** num único tick, bit a bit sem tabela (8 iterações/byte): para uma imagem de 512 KB a 12 MHz, o super-loop congela vários segundos;
  - `ota_handle_wait_erase()` contém um busy-wait de 100 000 iterações;
  - há ~95 `printf` bloqueantes espalhados por `app/`, `drivers/` e `peripherals/`, incluindo no caminho OTA;
  - `nominalMode()`/`safeMode()` chamam `sensors_print()` (o JSON de 416 ms) **incondicionalmente** — não existe nenhuma flag de build que distinga "compilação de voo" de "compilação de debug", pelo que a afirmação do §9.1.2 não tem suporte no código.
- **Porquê:** contradiz RNF1 ("todas as funções retornam imediatamente") e a afirmação do §4.3.1 de que "não existem ciclos internos (…) em nenhum caminho de execução". Durante o CRC bloqueante perdem-se comandos UART.
- **Impacto: ALTO.**
- **Correção:** CRC incremental por blocos dentro da FSM (um bloco por tick, com tabela de 256 entradas como no bootloader); remover o busy-wait usando o SysTick; encapsular printf num `DEBUG_LOG()` desativável por macro de compilação.

### C11. Validação de limites OTA incorreta/insuficiente
- `ota_handle_wait_erase_meta()` valida `firmware_size > (0xF00000UL - OTA_EXT_FW_ADDR)` ≈ **14,3 MB** — a região de firmware tem 1020 KB e o chip inteiro tem 2 MB (limite correto: `0x200000 − 0x101000 = 0xFF000`). O `seq` dos pacotes nunca é validado: `pkt_addr = 0x101000 + seq×128` com `seq` até 0xFFFE endereça até ~8,4 MB, fora da região (corrompendo logs/configuração por wrap). Um pacote fora de ordem que salte um setor escreve em flash não apagada (o erase segue apenas `s_next_fw_sector` sequencial).
- **Porquê:** o canal rádio é por definição ruidoso; um header corrompido que passe o sync word pode corromper outras regiões da flash externa.
- **Impacto: ALTO** para robustez OTA (o bootloader tem a sua própria validação com `APP_MAX_SIZE`, o que mitiga a escrita na flash interna, mas não a corrupção da externa).
- **Correção:** validar `seq × 128 < 0xFF000` e `firmware_size ≤ 0xFF000`; rejeitar pacotes fora da janela esperada.

---

## 2. Achados médios

### M1. Intervalos de amostragem: três fontes, três valores
- Tabela 3.1: todos os sensores a 0,2 Hz (5 s). Tabela 5.4: `SAFE_DATA_NOMINAL_MODE = 10 000 ms` e `SAFE_DATA_SAFE_MODE = 2 500 ms` com "ação associada". Código: uma única variável `TIME_TO_UPDATE_VALUES = 5000` partilhada por todos os modos; `SAFE_DATA_NOMINAL_MODE` nunca é usado e o segundo macro chama-se `zSAFE_DATA_SAFE_MODE` (typo que o tornou órfão). A afirmação do §4.3 de que "modos como ultra_low_power_mode ajustam a cadência de leitura" não tem implementação — `ultraLowPowerMode()` só chama `sensors_tick()`.
- **Impacto: MÉDIO.** Correção: usar os macros por modo (corrigindo o typo) ou corrigir Tabela 5.4/§4.3.

### M2. Frequência do CPU: poster diz 300 MHz, sistema corre a 12 MHz; `MCK_HZ` não está centralizado
- O poster (cG57) anuncia "Cortex-M7 a 300 MHz"; o relatório e o código operam a 12 MHz (RC interno). Além disso, §8.5.1 afirma que a frequência "é declarada numa única constante (MCK_HZ em board.h)" — `board.h` não tem `MCK_HZ`; a constante está **duplicada** em `hal_i2c.c`, `hal_usart.c`, `hal_systick.c` e `wcet_test.h`, com comentários contraditórios ("MCK ≈ 4 MHz", "6.25 MHz → 160 ns") que evidenciam o risco.
- **Impacto: MÉDIO** (coerência de baud rates/base de tempo se alguém alterar uma cópia). Correção: mover `MCK_HZ` para `board.h`, apagar duplicados, limpar comentários; corrigir o poster.

### M3. `main.c` real difere da Listagem 4.1 e executa testes a cada arranque
- O `main.c` corre `test_qspi_rw()`, `test_wcet()` e `test_seu_injection()` em todos os boots (escrevendo na flash externa e demorando segundos), imprime "beta 1.100" e 10 linhas vazias. A Listagem 4.1 do relatório mostra um super-loop limpo.
- **Impacto: MÉDIO** (o binário auditado não é o binário descrito; o teste QSPI consome ciclos de erase da flash a cada arranque). Correção: guardar testes atrás de uma macro `RUN_BOARD_TESTS` e alinhar a listagem.

### M4. Protocolo de sincronização de arranque descrito como ativo, mas desativado no código
- §5.4.3.1 descreve a busca de `[0xAA][0x55][0xAA][0x55]` no arranque; em `ttc.c:51`, `sync_state` inicia em `SYNC_DONE` com o comentário "no sync sequence is sent at boot". O mecanismo existe mas nunca corre.
- **Impacto: MÉDIO** (documentação de protocolo enganadora para quem implementar uma ground station). Correção: nota no relatório ("desativado na configuração atual com Pico passthrough").

### M5. Requisitos não funcionais desalinhados (RNF4 fantasma)
- O Capítulo 3 define RNF1–RNF3, mas §2.4.2 refere "RNF4" e a Tabela 8.2 verifica RNF1–RNF4 com descrições desfasadas (RNF3 é definido como memória estática, mas verificado como "testes de FSM"; RNF4 não existe na definição). A nota da Tabela 3.1 refere "frequência do núcleo (12) referida no RNF1" — RNF1 não menciona frequência e "(12)" está sem unidade.
- **Impacto: MÉDIO** (rastreabilidade requisitos→verificação, que é o propósito do capítulo). Correção: renumerar e alinhar definição↔verificação.

### M6. Telemetria em falta no segmento de solo: bateria e logging em flash
- O dashboard tem `batteryLevel` (campo "bat" no codec ASCII), mas o OBC nunca envia bateria — nem no JSON de `sensors_print()` nem no frame binário de 75 bytes. O painel de bateria nunca terá dados reais. Adicionalmente, `sensors_save_to_flash()` (logging de telemetria na região de 960 KB da flash externa, Tabela 6.2) **nunca é chamada** — e nem copia `gnss` para a estrutura — pelo que a região "Logs de telemetria" está morta.
- **Impacto: MÉDIO.** Correção: incluir bateria no frame; ligar (ou remover) o logging em flash.

### M7. Poster e manual com afirmações desatualizadas
- Poster: "300 MHz" (ver M2); lista 5 modos ("Nominal, Comms, OTA, Safe e Ultra-Low-Power"), omitindo decommissioning (o relatório diz 6); "jitter ,1/2 ticks" é ilegível (símbolo perdido na composição). Manual (infoG57): datado "Julho 2025" quando o projeto é do semestre 2025/2026 e o relatório é de julho de 2026; porta "COM3" apresentada como fixa quando depende da máquina.
- **Impacto: MÉDIO** (documentos públicos/entregáveis com factos contraditórios entre si). Correção: harmonizar os três documentos.

### M8. Ambiguidade honesta vs. omissão no §9.1.2
- §9.1.2 afirma que o printf bloqueante "pertence exclusivamente à instrumentação de desenvolvimento" e que "o downlink de telemetria operacional segue pelo canal TT&C através da FSM não bloqueante". Verdade parcial: `ttc_send_telemetry()` é de facto não bloqueante, mas `nominalMode()` chama sempre `sensors_print()` (bloqueante) no mesmo ramo — não há build de voo sem ele (ver C10).
- **Impacto: MÉDIO.** Correção: gate de compilação + reformular o parágrafo.

---

## 3. Achados menores

| # | Achado | Impacto | Correção |
|---|--------|---------|----------|
| m1 | Referência quebrada "Secção ??" em §2.6.2 | BAIXO | Corrigir `\ref` |
| m2 | §2.1.2 anuncia "Três propriedades" e enumera duas | BAIXO | Repor a 3.ª (determinismo) ou corrigir contagem |
| m3 | `ota_sm_t` tem 13 valores, relatório diz 12 (§6.5.4) | BAIXO | Corrigir contagem |
| m4 | Campo `raging` em `ttc_data_t` (typo de *ranging*) | BAIXO | Renomear |
| m5 | `MAX_INIT_RETRIES` está em `init.h`, não em `mission.h` (Tabela 5.4); valor `0x05` em hex sem razão | BAIXO | Alinhar |
| m6 | `attempts` partilhado entre retries de `init_all()` e `self_test()` em `init.c` — o self-test pode ficar sem tentativas | BAIXO | Contadores separados |
| m7 | Comentários obsoletos: "W25Q128" em `board.h` (chip é S25FL116K), "MCK ~4 MHz" em `hal_i2c.c`/`hal_debug_uart.c`, "6.25 MHz" em `wcet_test.h`, `TTC_BUF_LEN 80U (…) = 75 B`, "[12] checksum" no codec do dashboard | BAIXO | Limpar |
| m8 | `OTA_READY_CODE 0xE0` definido mas o READY real é `[0x10 0xAC 0x4B 0xF7]` (código morto/ambíguo) | BAIXO | Remover macro ou usá-la |
| m9 | Tabela 3.1 diz SPI "1–10 MHz"; Fig. 4.2 e código (SCBR=12 → 1 MHz) dizem 1 MHz | BAIXO | Uniformizar |
| m10 | `modeSelecter.c` (spelling) vs "modeSelector" no relatório (Tabela 4.1) — e `otaMode()` está em `modes.c`, não em `modeSelecter.c` como diz §6.5.4 | BAIXO | Alinhar nomes |
| m11 | Periférico `flexiforce.c/h` existe no código e não é mencionado no relatório | BAIXO | Documentar ou remover |
| m12 | `decommissioning_mode` só é alcançável a partir de `ultra_low_power_mode` (código); Tabela 4.2 sugere entrada genérica por fim de missão | BAIXO | Documentar a restrição |
| m13 | Linker do bootloader declara `QSPIMEM LENGTH = 16M` — o chip tem 16 **Mbit** (2 MB) | BAIXO | Corrigir (inócuo mas repete a confusão Mbit/MB) |
| m14 | Relatório Tabela 8.1 refere `test_utils.h` inexistente | BAIXO | Ver C8 |

---

## 4. O que está sólido (verificado positivamente)

- **Drivers e FSMs**: o driver I2C corresponde ao relatório (10 estados confirmados em `i2c_driver.h`, `I2C_TIMEOUT_MAX = 1000`, `bus_locked`, recovery após 3 erros); testes de drivers extensos e reais (test_i2c_stress.c com 917 linhas).
- **TMR de estruturas de sensores (`seu_data.c`)**: implementado como descrito (bancos separados, votação 2-de-3 byte a byte, scrub+commit em `sensors_tick()`), coerente com §8.6.
- **Bootloader**: sequência da Tabela 6.4 confirmada no código; proteção EFC contra escrita abaixo de `APP_START_ADDR`; validação do vetor de reset (≠0, ≠0xFFFFFFFF); CRC32 com polinómio 0xEDB88320 idêntico nos dois lados; metadados de 256 bytes = 1 página (escrita atómica); decisão de manter metadados em falha de escrita e apagar em falha de CRC — bem raciocinado.
- **Aritmética do mapa da flash externa** (64+960+4+1020 = 2048 KB) e do pacote OTA (6+128 = 134 B) confere.
- **Protocolo OTA solo↔bordo é coerente entre dashboard e firmware** (ACK `[0xAC][seq][XOR]`, NACK 0x4E, READY `[0x10 0xAC 0x4B 0xF7]`, timeout 3 s, 3 retries) — o problema é o relatório descrevê-lo de forma incompleta, não a implementação.
- **Dashboard**: RBAC, JWT, convites e auditoria existem tal como descritos; docker-compose com PostgreSQL 16 + Nginx confirma a Tabela 7.1.
- **Honestidade nas limitações**: a ausência de autenticação criptográfica no OTA está corretamente declarada como limitação (§9.1.1), e a metodologia WCET no código (`wcet_test.c`) é mais rigorosa do que o texto sugere (documenta explicitamente que medição ≠ garantia formal).

---

## 5. Resumo executivo

O projeto é substancialmente real e de qualidade acima da média para o contexto: os drivers não bloqueantes existem e estão bem testados, o bootloader OTA está completo e defensivo, a TMR de dados está funcional e o ecossistema solo↔bordo é coerente. **O problema central não é o código — é o relatório descrever um sistema melhor do que o implementado.** Há um padrão sistemático de afirmações de fiabilidade sem suporte no código: TMR do estado de missão (C5), flags de validade de sensores (C7), prioridade da verificação de saúde (C6), lógica de bateria alimentada por uma constante (C4), testes "Implementados" que são esqueletos (C8) e um pacote de telemetria que não existe no fio (C3). A secção de validação quantitativa — o coração da tese "bare-metal determinístico" — contém aritmética errada e um resultado fisicamente implausível não explicado (C9), e o mapa de memória documentado diverge do código em 64 KB (C1), com um linker que ultrapassa a flash física (C2). Num contexto profissional exigente (e à luz da ECSS-E-ST-40C que o próprio relatório invoca), a matriz de rastreabilidade da Tabela 8.2 não sobreviveria a uma auditoria independente.

**Principais riscos:** (1) decisões de modo baseadas em bateria nunca disparam em operação; (2) super-loop congela segundos durante o CRC OTA e 416 ms por telemetria, sem build de voo que o evite; (3) OTA aceita tamanhos/sequências fora da região de flash; (4) binário pode crescer silenciosamente para além da flash física; (5) credibilidade documental — três documentos entregáveis contradizem-se entre si e com o código.

## 6. Lista priorizada

**P0 — corrigir antes de qualquer entrega/defesa**
1. C9 — aritmética WCET e explicação mission_lifecycle vs sensors_tick (é a validação central).
2. C8 — Tabelas 8.1/8.2: reclassificar testes esqueleto ou implementá-los.
3. C1/C2 — mapa de memória 0x410000 vs 0x420000 + LENGTH do linker.
4. C3 — pacote de telemetria 84 vs 75 bytes (RF3).

**P1 — correções de engenharia de curto prazo**
5. C4 — alimentar `BATTERY_STATUS` com dados reais.
6. C6 — guardas de segurança em `ota_mode`.
7. C10 — CRC OTA incremental + gate de compilação para printf.
8. C11 — validação de `seq`/`firmware_size` contra a região real.
9. C5 — proteger `state` com a TMR já existente (1 linha).
10. C7 — flags de validade nos sensores.

**P2 — coerência documental e limpeza**
11. M1–M8 (intervalos de amostragem, MCK_HZ centralizado, main.c vs listagem, RNF4, poster/manual).
12. m1–m14 (typos, comentários obsoletos, referências quebradas, código morto).

## 7. Recomendações gerais

1. **Uma única fonte de verdade para constantes partilhadas** — mapa de memória, MCK_HZ, formatos de frame — num header comum a aplicação, bootloader e (gerado/verificado) dashboard; os erros C1, C2, M2 e C3 nasceram todos de duplicação.
2. **Separar build de voo de build de debug** (`-DFLIGHT_BUILD`): sem isto, nenhuma medição temporal em bancada é representativa e as alegações de RNF1 ficam indefensáveis.
3. **Regra "sem evidência, sem Verificado"**: cada célula da matriz de rastreabilidade deve apontar para um artefacto executável (log de teste, screenshot de placa) — o projeto já produz vários; usar apenas esses.
4. **Rever o relatório contra o código, não de memória**: a maioria dos achados críticos são descrições de versões antigas ou intenções (0x410000, sync de arranque, 84 bytes, SAFE_DATA_*). Um passo final de "diff documental" evitaria quase todos.
5. **Fuzzing leve do parser OTA** (headers corrompidos, seq fora de ordem, len>128) na bancada Pico — os limites da C11 seriam apanhados de imediato.
