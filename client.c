// client.c

#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// Array global para armazenar clientes
static Cliente* clientes[MAX_CLIENTS];
static int num_clientes = 0;
static pthread_mutex_t mutex_clientes = PTHREAD_MUTEX_INITIALIZER;

Cliente* criar_cliente(int socket) {
    Cliente* cliente = (Cliente*)malloc(sizeof(Cliente));
    if (!cliente) {
        perror("malloc");
        return NULL;
    }
    cliente->socket = socket;
    memset(cliente->name, 0, NAME_LEN);
    memset(cliente->sala_atual, 0, SALA_LEN);
    return cliente;
}

void adicionar_cliente(Cliente* cliente) {
    pthread_mutex_lock(&mutex_clientes);
    if (num_clientes < MAX_CLIENTS) {
        clientes[num_clientes++] = cliente;
    }
    pthread_mutex_unlock(&mutex_clientes);
}

void remover_cliente(int socket) {
    pthread_mutex_lock(&mutex_clientes);
    int pos = -1;
    for (int i = 0; i < num_clientes; i++) {
        if (clientes[i]->socket == socket) {
            pos = i;
            break;
        }
    }
    if (pos != -1) {
        free(clientes[pos]);
        for (int i = pos; i < num_clientes - 1; i++) {
            clientes[i] = clientes[i + 1];
        }
        num_clientes--;
    }
    pthread_mutex_unlock(&mutex_clientes);
}

Cliente* buscar_cliente_por_nome(const char* nome) {
    Cliente* resultado = NULL;
    pthread_mutex_lock(&mutex_clientes);
    for (int i = 0; i < num_clientes; i++) {
        if (strcmp(clientes[i]->name, nome) == 0) {
            resultado = clientes[i];
            break;
        }
    }
    pthread_mutex_unlock(&mutex_clientes);
    return resultado;
}

void listar_clientes_online(char* buffer, size_t size) {
    pthread_mutex_lock(&mutex_clientes);
    if (num_clientes == 0) {
        snprintf(buffer, size, "Nenhum cliente online.\n");
    } else {
        strcpy(buffer, "Clientes online: ");
        for (int i = 0; i < num_clientes; i++) {
            strcat(buffer, clientes[i]->name);
            if (i < num_clientes - 1) {
                strcat(buffer, ", ");
            }
        }
        strcat(buffer, "\n");
    }
    pthread_mutex_unlock(&mutex_clientes);
}
