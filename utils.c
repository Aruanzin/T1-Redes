// utils.c

#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "client.h"

void enviar_mensagem(int socket, const char* mensagem) {
    if (send(socket, mensagem, strlen(mensagem), 0) < 0) {
        perror("send");
    }
}

void enviar_mensagem_para_todos(const char* mensagem, Cliente* remetente) {
    extern Cliente* clientes[]; // Declarar externamente a lista de clientes
    extern int num_clientes;     // Número atual de clientes
    for (int i = 0; i < num_clientes; i++) {
        if (clientes[i]->socket != remetente->socket) {
            enviar_mensagem(clientes[i]->socket, mensagem);
        }
    }
}

void notificar_conexao(const char* mensagem, Cliente* remetente) {
    enviar_mensagem_para_todos(mensagem, remetente);
}

void notificar_desconexao(const char* mensagem, Cliente* remetente) {
    enviar_mensagem_para_todos(mensagem, remetente);
}
