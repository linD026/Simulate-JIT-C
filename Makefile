CC ?= gcc
CFLAGS := -g

SRC = src/jit.c
OBJ=$(SRC:.c=.o)

ifeq (debug,1)
CFLAGS+=-D'PRINT_OUT_CMD'
endif

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

all: $(OBJ)
	$(CC) -o jit-c $(OBJ)

clean:
	rm -f src/*.o
	rm -f jit-c

indent:
	clang-format -i src/*.[ch]
