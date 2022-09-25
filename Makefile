CC ?= gcc
CFLAGS := -g

SRC = src/jit.c
OBJ=$(SRC:.c=.o)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

all: $(OBJ)
	$(CC) -o jit-c $(OBJ)

clean:
	rm -f src/*.o
	rm -f jit-c
