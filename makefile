# Makefile

CC = gcc
CFLAGS = -Wall -pthread
DEPS = client.h room.h utils.h server.h
OBJ = main.o server.o client.o room.o utils.o

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

chat_server: $(OBJ)
	$(CC) $(CFLAGS) -o chat_server $(OBJ)

.PHONY: clean

clean:
	rm -f *.o chat_server
