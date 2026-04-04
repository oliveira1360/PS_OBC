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
      src/peripherals/gnss.c \
      src/peripherals/imu.c \
      src/peripherals/eps.c \
      src/peripherals/pressure.c \
      src/peripherals/temperature.c

TEST_SRC = test/test_main.c \
           test/test_i2c.c \
           src/hal/hal_i2c.c \
           src/drivers/i2c_driver.c

OBJ = $(addprefix build/, $(notdir $(SRC:.c=.o)))

VPATH = src src/app src/hal src/drivers src/peripherals

all: create_dir $(TARGET)

create_dir:
	@if not exist build mkdir build

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

build/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: create_dir
	$(CC) $(CFLAGS) -I test/inc -o $(TEST_TARGET) $(TEST_SRC)
	./$(TEST_TARGET)

clean:
	@if exist build rmdir /s /q build