# Nome do executável final (agora fica guardado na pasta build)
TARGET = main.exe

# Compilador
CC = gcc

# Flags de compilação (-I inc diz ao compilador onde estão os ficheiros .h)
CFLAGS = -Wall -Wextra -g -I inc

# Localização dos ficheiros fonte
SRC = src/main.c \
      src/app/nomimalMode.c \
      src/app/stateCheck.c \
      src/app/states.c

# Mapeamento dos objetos (todos vão diretamente para a pasta build/)
OBJ = build/main.o \
      build/nomimalMode.o \
      build/stateCheck.o \
      build/states.o

# Regra principal: Garante que a pasta build existe antes de compilar
all: create_dir $(TARGET)

# Cria a pasta build se ela não existir (comando de Windows)
create_dir:
	@if not exist build mkdir build

# Linkagem do executável final
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

# Regra para compilar o ficheiro na raiz da src (main.c)
build/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Regra para compilar os ficheiros dentro de src/app/
build/%.o: src/app/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpeza: Agora basta apagar a pasta build inteira!
clean:
	@if exist build rmdir /s /q build