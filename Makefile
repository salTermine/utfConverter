CC = gcc
SRC = src/*.c
INCLUDE = include/
CFLAGS = -Wall -Werror -pedantic -Wextra 
REQ = $(SRC) include/*.h
BINS = bin/utf

all: folders $(BINS)

folders:
	mkdir -p build
	mkdir -p bin

bin/utf:
	$(CC) $(CFLAGS) -I $(INCLUDE) $(SRC) $^ -o $@ -lm

bin/debug: $(REQ)
	$(CC) $(CFLAGS) -g -DDEBUG -I $(INCLUDE) $(SRC) $^ -o $@ -lm

clean:
	rm -f *.o $(BIN)

