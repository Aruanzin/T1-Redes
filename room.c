// room.c

#include "room.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

static Sala salas[MAX_SALAS];
static int num_salas = 0;
static pthread_mutex_t mutex_salas = PTHREAD_MUTEX_INITIALIZER;

int criar_sala(const char* nome) {
    pthread_mutex_lock(&mutex_salas);
    // Verificar se a sala já existe
    for (int i = 0; i < num_salas; i++) {
        if (strcmp(salas[i].nome, nome) == 0) {
            pthread_mutex_unlock(&mutex_salas);
            return -1; // Sala já existe
        }
    }
    if (num_salas >= MAX_SALAS) {
        pthread_mutex_unlock(&mutex_salas);
        return -2; // Limite de salas alcançado
    }
    // Criar nova sala
    strncpy(salas[num_salas].nome, nome, SALA_NAME_LEN);
    salas[num_salas].num_clientes = 0;
    num_salas++;
    pthread_mutex_unlock(&mutex_salas);
    return 0; // Sucesso
}

int entrar_sala(const char* nome, Cliente* cliente) {
    pthread_mutex_lock(&mutex_salas);
    // Encontrar a sala
    int sala_index = -1;
    for (int i = 0; i < num_salas; i++) {
        if (strcmp(salas[i].nome, nome) == 0) {
            sala_index = i;
            break;
        }
    }
    if (sala_index == -1) {
        pthread_mutex_unlock(&mutex_salas);
        return -1; // Sala não encontrada
    }
    // Verificar se a sala está cheia
    if (salas[sala_index].num_clientes >= MAX_CLIENTS_POR_SALA) {
        pthread_mutex_unlock(&mutex_salas);
        return -2; // Sala cheia
    }
    // Adicionar cliente à sala
    salas[sala_index].clientes[salas[sala_index].num_clientes++] = cliente;
    strncpy(cliente->sala_atual, nome, SALA_LEN);
    pthread_mutex_unlock(&mutex_salas);
    return 0; // Sucesso
}

int sair_sala(Cliente* cliente) {
    pthread_mutex_lock(&mutex_salas);
    if (strlen(cliente->sala_atual) == 0) {
        pthread_mutex_unlock(&mutex_salas);
        return -1; // Cliente não está em nenhuma sala
    }
    // Encontrar a sala atual
    int sala_index = -1;
    for (int i = 0; i < num_salas; i++) {
        if (strcmp(salas[i].nome, cliente->sala_atual) == 0) {
            sala_index = i;
            break;
        }
    }
    if (sala_index == -1) {
        pthread_mutex_unlock(&mutex_salas);
        return -2; // Sala não encontrada
    }
    // Remover cliente da sala
    int pos = -1;
    for (int j = 0; j < salas[sala_index].num_clientes; j++) {
        if (salas[sala_index].clientes[j]->socket == cliente->socket) {
            pos = j;
            break;
        }
    }
    if (pos != -1) {
        // Shift para remover
        for (int k = pos; k < salas[sala_index].num_clientes - 1; k++) {
            salas[sala_index].clientes[k] = salas[sala_index].clientes[k + 1];
        }
        salas[sala_index].num_clientes--;
    }
    // Notificar os membros da sala
    char msg[LEN];
    snprintf(msg, LEN, "%s saiu da sala.\n", cliente->name);
    enviar_mensagem_sala(salas[sala_index].nome, msg, cliente);
    // Limpar a sala atual do cliente
    memset(cliente->sala_atual, 0, SALA_LEN);
    pthread_mutex_unlock(&mutex_salas);
    return 0; // Sucesso
}

void listar_salas(char* buffer, size_t size) {
    pthread_mutex_lock(&mutex_salas);
    if (num_salas == 0) {
        snprintf(buffer, size, "Nenhuma sala disponível.\n");
    } else {
        strcpy(buffer, "Salas disponíveis: ");
        for (int i = 0; i < num_salas; i++) {
            strcat(buffer, salas[i].nome);
            if (i < num_salas - 1) {
                strcat(buffer, ", ");
            }
        }
        strcat(buffer, "\n");
    }
    pthread_mutex_unlock(&mutex_salas);
}

void enviar_mensagem_sala(const char* sala_nome, const char* mensagem, Cliente* remetente) {
    pthread_mutex_lock(&mutex_salas);
    for (int i = 0; i < num_salas; i++) {
        if (strcmp(salas[i].nome, sala_nome) == 0) {
            for (int j = 0; j < salas[i].num_clientes; j++) {
                if (salas[i].clientes[j]->socket != remetente->socket) { // Opcional: não enviar ao remetente
                    enviar_mensagem(salas[i].clientes[j]->socket, mensagem);
                }
            }
            break;
        }
    }
    pthread_mutex_unlock(&mutex_salas);
}
