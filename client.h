// client.h

#ifndef CLIENT_H
#define CLIENT_H

#define NAME_LEN 50
#define SALA_LEN 50

typedef struct Cliente {
    int socket;
    char name[NAME_LEN];
    char sala_atual[SALA_LEN];
} Cliente;

// Funções relacionadas aos clientes
Cliente* criar_cliente(int socket);
void adicionar_cliente(Cliente* cliente);
void remover_cliente(int socket);
Cliente* buscar_cliente_por_nome(const char* nome);
void listar_clientes_online(char* buffer, size_t size);

#endif // CLIENT_H
