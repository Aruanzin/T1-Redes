// room.h

#ifndef ROOM_H
#define ROOM_H

#include "client.h"

#define MAX_SALAS 50
#define MAX_CLIENTS_POR_SALA 100
#define SALA_NAME_LEN 50

typedef struct Sala {
    char nome[SALA_NAME_LEN];
    Cliente* clientes[MAX_CLIENTS_POR_SALA];
    int num_clientes;
} Sala;

// Funções relacionadas às salas
int criar_sala(const char* nome);
int entrar_sala(const char* nome, Cliente* cliente);
int sair_sala(Cliente* cliente);
void listar_salas(char* buffer, size_t size);
void enviar_mensagem_sala(const char* sala_nome, const char* mensagem, Cliente* remetente);

#endif // ROOM_H
