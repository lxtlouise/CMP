CC=gcc
LD=gcc
CFLAGS=-c -Lm

SRC := .
OBJ := obj

SOURCES := $(wildcard $(SRC)/*.c)
OBJECTS := $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SOURCES))

all: $(OBJECTS)
	$(CC) $^ -o cmp -lm

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) -I$(SRC) -c $< -o $@

clean:
	rm -f $(OBJ)/*.o cmp
