# Variáveis
CC = gcc
CFLAGS = -lpthread

# Alvos
all: server client

# Compila o server
server: server.c
	$(CC) -o server server.c $(CFLAGS)

# Compila o client
client: client.c
	$(CC) client.c -o client

# Limpa os arquivos executáveis
clean:
	rm -f server client
