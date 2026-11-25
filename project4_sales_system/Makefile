CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -Wpedantic -g -Iinclude
LDFLAGS = -lm
TARGET  = sales_system

SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c,build/%.o,$(SRC))

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build:
	mkdir -p build

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf build $(TARGET) $(TARGET).exe

.PHONY: all run clean
