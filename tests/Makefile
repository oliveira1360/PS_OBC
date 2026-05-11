# ==============================================================================
# Makefile — Test Suite OBC (host build, USE_REAL_HW=0)
#
# Estrutura:
#   tests/
#     framework/   — test_runner.h (header-only)
#     mocks/       — mock_hal_*.h/.c
#     drivers/     — test_*_driver.c
#     peripherals/ — test_*.c (skeletons)
#     app/         — test_*.c (skeletons)
#
# Uso:
#   make              — compila e corre todos os testes de drivers
#   make test_spi     — compila e corre apenas SPI
#   make test_i2c     — compila e corre apenas I2C
#   make test_usart   — compila e corre apenas USART
#   make test_qspi    — compila e corre apenas QSPI
#   make run_all      — corre todos sequencialmente, reporta resultado final
#   make clean        — remove binários
# ==============================================================================

# ---------------------------------------------------------------------------
# Toolchain e flags
# ---------------------------------------------------------------------------
CC     = gcc
CFLAGS = -Wall -Wextra -std=c99 -g \
         -DUSE_REAL_HW=0 \
         -I../inc \
         -I./framework \
         -I./mocks

# Suprime redefinições de QSPI_TIMEOUT_MAX/QSPI_RETRY_MAX
# (definidas em board.h e em qspi_driver.h com o mesmo valor)
CFLAGS += -Wno-unused-macros

# ---------------------------------------------------------------------------
# Caminhos das fontes do projecto
# ---------------------------------------------------------------------------
SRC = ../src

# ---------------------------------------------------------------------------
# Ficheiros comuns
# ---------------------------------------------------------------------------
MOCK_SPI   = mocks/mock_hal_spi.c
MOCK_I2C   = mocks/mock_hal_i2c.c
MOCK_USART = mocks/mock_hal_usart.c
MOCK_QSPI  = mocks/mock_hal_qspi.c

DRV_SPI    = $(SRC)/drivers/spi_driver.c
DRV_I2C    = $(SRC)/drivers/i2c_driver.c
DRV_USART  = $(SRC)/drivers/usart_driver.c
DRV_QSPI   = $(SRC)/drivers/qspi_driver.c

# ---------------------------------------------------------------------------
# Targets de drivers (build + run)
# ---------------------------------------------------------------------------
.PHONY: all test_spi test_i2c test_usart test_qspi test_i2c_stress run_all clean

all: test_spi test_i2c test_usart test_qspi test_i2c_stress

# --- SPI ---
test_spi: bin/test_spi_driver
	@echo ""
	@echo "════════════════════════════════════════"
	@echo "  RUNNING: SPI Driver Tests"
	@echo "════════════════════════════════════════"
	@./bin/test_spi_driver; echo "SPI exit: $$?"

bin/test_spi_driver: drivers/test_spi_driver.c $(MOCK_SPI) $(DRV_SPI) | bin
	$(CC) $(CFLAGS) $^ -o $@

# --- I2C ---
test_i2c: bin/test_i2c_driver
	@echo ""
	@echo "════════════════════════════════════════"
	@echo "  RUNNING: I2C Driver Tests"
	@echo "════════════════════════════════════════"
	@./bin/test_i2c_driver; echo "I2C exit: $$?"

bin/test_i2c_driver: drivers/test_i2c_driver.c $(MOCK_I2C) $(DRV_I2C) | bin
	$(CC) $(CFLAGS) $^ -o $@

# --- USART ---
test_usart: bin/test_usart_driver
	@echo ""
	@echo "════════════════════════════════════════"
	@echo "  RUNNING: USART Driver Tests"
	@echo "════════════════════════════════════════"
	@./bin/test_usart_driver; echo "USART exit: $$?"

bin/test_usart_driver: drivers/test_usart_driver.c $(MOCK_USART) $(DRV_USART) | bin
	$(CC) $(CFLAGS) $^ -o $@

# --- I2C Stress ---
test_i2c_stress: bin/test_i2c_stress
	@echo ""
	@echo "════════════════════════════════════════"
	@echo "  RUNNING: I2C Stress Tests"
	@echo "════════════════════════════════════════"
	@./bin/test_i2c_stress; echo "I2C-Stress exit: $$?"

bin/test_i2c_stress: drivers/test_i2c_stress.c $(MOCK_I2C) $(DRV_I2C) | bin
	$(CC) $(CFLAGS) $^ -o $@

# --- QSPI ---
test_qspi: bin/test_qspi_driver
	@echo ""
	@echo "════════════════════════════════════════"
	@echo "  RUNNING: QSPI Driver Tests"
	@echo "════════════════════════════════════════"
	@./bin/test_qspi_driver; echo "QSPI exit: $$?"

bin/test_qspi_driver: drivers/test_qspi_driver.c $(MOCK_QSPI) $(DRV_QSPI) | bin
	$(CC) $(CFLAGS) $^ -o $@

# ---------------------------------------------------------------------------
# run_all: corre todos e reporta resultado global
# ---------------------------------------------------------------------------
run_all: all
	@echo ""
	@echo "╔══════════════════════════════════════╗"
	@echo "║        FULL TEST RUN SUMMARY         ║"
	@echo "╚══════════════════════════════════════╝"
	@TOTAL_FAIL=0; \
	for bin in bin/test_spi_driver bin/test_i2c_driver bin/test_i2c_stress bin/test_usart_driver bin/test_qspi_driver; do \
	    $$bin; \
	    TOTAL_FAIL=$$((TOTAL_FAIL + $$?)); \
	done; \
	echo ""; \
	if [ $$TOTAL_FAIL -eq 0 ]; then \
	    echo "  ✓ ALL DRIVER TESTS PASSED"; \
	else \
	    echo "  ✗ $$TOTAL_FAIL TEST SUITE(S) FAILED"; \
	fi; \
	exit $$TOTAL_FAIL

# ---------------------------------------------------------------------------
# Directório de binários
# ---------------------------------------------------------------------------
bin:
	mkdir -p bin

# ---------------------------------------------------------------------------
# Limpeza
# ---------------------------------------------------------------------------
clean:
	rm -rf bin
