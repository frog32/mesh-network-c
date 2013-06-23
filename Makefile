
CC = /usr/bin/gcc
CFLAGS = -Wall -g
LDFLAGS = -lm -lpthread

BIN = meshnode
OBJ = connection.o conn_server.o conn_io.o check.o parse_options.o

all: $(OBJ) main.c
	$(CC) $(CLAGS) main.c -o $(BIN) $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	rm -rf $(BIN) $(OBJ)
