// main.c

#include "server.h"
#include "client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORTA 2000
#define MAX_CLIENTS 100
#define LEN 1024

int main() {
    int sockfd, cliente_socket;
    struct sockaddr_in local, remoto;
    socklen_t len = sizeof(remoto);
    pthread_t thread_id;

    // Criar o socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket ");
        exit(1);
    }
    printf("Socket Criado com Sucesso\n");

    // Configurar o endereço local
    local.sin_family = AF_INET;
    local.sin_port = htons(PORTA);
    local.sin_addr.s_addr = INADDR_ANY; // Para aceitar conexões de qualquer IP
    memset(local.sin_zero, 0x0, 8);

    // Vincular o socket ao endereço local
    if (bind(sockfd, (struct sockaddr*)&local, sizeof(local)) == -1) {
        perror("bind ");
        close(sockfd);
        exit(1);
    }
    printf("Bind realizado com sucesso na porta %d\n", PORTA);

    // Colocar o socket em modo de escuta
    if (listen(sockfd, MAX_CLIENTS) == -1) {
        perror("listen ");
        close(sockfd);
        exit(1);
    }
    printf("Servidor escutando na porta %d...\n", PORTA);

    while (1) {
        // Aceitar uma nova conexão
        cliente_socket = accept(sockfd, (struct sockaddr*)&remoto, &len);
        if (cliente_socket == -1) {
            perror("accept ");
            continue;
        }

        // Criar uma nova thread para gerenciar o cliente
        Cliente* novo_cliente = criar_cliente(cliente_socket);
        if (!novo_cliente) {
            enviar_mensagem(cliente_socket, "Erro interno. Tente novamente.\n");
            close(cliente_socket);
            continue;
        }

        if (pthread_create(&thread_id, NULL, gerenciar_cliente, (void*)novo_cliente) != 0) {
            perror("pthread_create ");
            enviar_mensagem(cliente_socket, "Erro interno. Tente novamente.\n");
            close(cliente_socket);
            free(novo_cliente);
            continue;
        }

        // Desanexar a thread para liberar recursos automaticamente
        pthread_detach(thread_id);
    }

    close(sockfd);
    return 0;
}
