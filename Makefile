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
      src/hal/hal_i2c.c \
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

# Objetos
OBJ = build/main.o \
      build/modeSelecter.o \
      build/stateCheck.o \
      build/sensors.o \
      build/mission.o \
      build/hal_i2c.o \
      build/i2c_driver.o \
      build/gnss.o \
      build/imu.o \
      build/eps.o \
      build/pressure.o \
      build/temperature.o

# Regra principal
all: create_dir $(TARGET)

# Cria a pasta build
create_dir:
	@if not exist build mkdir build

# Linkagem
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

# Regras de compilação por pasta
build/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: src/app/%.c
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: src/hal/%.c
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: src/drivers/%.c
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: src/peripherals/%.c
	$(CC) $(CFLAGS) -c $< -o $@

test: create_dir
	$(CC) -Wall -Wextra -g -I inc -I test/inc -o $(TEST_TARGET) $(TEST_SRC)
	./$(TEST_TARGET)

# Limpeza
clean:
	@if exist build rmdir /s /q build