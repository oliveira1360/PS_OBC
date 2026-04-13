# Nome do executável final
TARGET = main.exe
TEST_TARGET = test.exe

# Compilador
CC = gcc

# Flags de compilação
CFLAGS = -Wall -Wextra -g -I inc

# Ficheiros fonte
SRC = src/main.c \
      src/app/modeSelecter.c \
      src/app/stateCheck.c \
      src/app/sensors.c \
      src/app/mission.c \
      src/app/init.c \
      src/hal/hal_i2c.c \
      src/hal/hal_gpio.c \
      src/hal/hal_peripherals_init.c \
      src/hal/hal_qspi.c \
      src/hal/hal_spi.c \
      src/hal/hal_usart.c \
      src/hal/hal_system.c \
      src/drivers/i2c_driver.c \
      src/drivers/usart_driver.c \
      src/drivers/spi_driver.c \
      src/peripherals/gnss.c \
      src/peripherals/imu.c \
      src/peripherals/eps.c \
      src/peripherals/pressure.c \
      src/peripherals/propulsor.c \
      src/peripherals/temperature.c \
      src/peripherals/ttc.c

TEST_SRC = test/test_main.c \
           test/unit/drivers/test_i2c_driver.c \
           test/unit/drivers/test_spi_driver.c \
           test/unit/drivers/test_usart_driver.c \
           test/unit/test_i2c_queue.c \
           test/unit/sensors/test_gnss.c \
           test/unit/sensors/test_imu.c \
           test/unit/sensors/test_pressure.c \
           test/unit/sensors/test_temperature.c \
           test/unit/sensors/test_eps.c \
           test/unit/sensors/test_parse_boundary.c \
           test/unit/app/test_init.c \
           test/unit/app/test_modes.c \
           test/unit/app/test_state_machine.c \
           test/integration/test_sensor_pipeline.c \
           test/stress/test_ecss_robustness.c \
           src/hal/hal_i2c.c \
           src/hal/hal_spi.c \
           src/hal/hal_usart.c \
           src/hal/hal_gpio.c \
           src/hal/hal_peripherals_init.c \
           src/hal/hal_qspi.c \
           src/hal/hal_system.c \
           src/drivers/i2c_driver.c \
           src/drivers/spi_driver.c \
           src/drivers/usart_driver.c \
           src/peripherals/gnss.c \
           src/peripherals/imu.c \
           src/peripherals/eps.c \
           src/peripherals/pressure.c \
           src/peripherals/temperature.c \
           src/peripherals/propulsor.c \
           src/peripherals/ttc.c \
           src/app/init.c \
           src/app/modeSelecter.c \
           src/app/stateCheck.c \
           src/app/sensors.c \
           src/app/mission.c

OBJ = $(addprefix build/, $(notdir $(SRC:.c=.o)))

VPATH = src src/app src/hal src/drivers src/peripherals

.PHONY: all test clean create_dir

all: create_dir $(TARGET)

create_dir:
	@if not exist build mkdir build

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

build/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test:
	$(CC) $(CFLAGS) -I test/inc -o $(TEST_TARGET) $(TEST_SRC) -lm
	./$(TEST_TARGET)

clean:
	@if exist build rmdir /s /q build