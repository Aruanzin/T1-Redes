#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORTA 2000
#define LEN 1024
#define MAX_CLIENTS 100

typedef struct {
    int socket;
    char name[50];
} Cliente;

Cliente clientes[MAX_CLIENTS];
int num_clientes = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* gerenciar_cliente(void* arg) {
    int cliente_socket = *((int*)arg);
    char buffer[LEN];
    char nome[50];

    // Receber o nome do cliente
    memset(buffer, 0x0, LEN);
    recv(cliente_socket, buffer, LEN, 0);
    strcpy(nome, buffer);

    printf("Cliente conectado: %s\n", nome);

    // Adicionar o cliente à lista
    pthread_mutex_lock(&mutex);
    if (num_clientes < MAX_CLIENTS) {
        clientes[num_clientes].socket = cliente_socket;
        strcpy(clientes[num_clientes].name, nome);
        num_clientes++;
    } else {
        printf("Número máximo de clientes alcançado.\n");
        close(cliente_socket);
        pthread_mutex_unlock(&mutex);
        return NULL;
    }
    pthread_mutex_unlock(&mutex);

    while (1) {
        memset(buffer, 0x0, LEN);
        int bytes = recv(cliente_socket, buffer, LEN, 0);
        if (bytes <= 0) {
            printf("Cliente desconectado: %s\n", nome);
            close(cliente_socket);
            pthread_mutex_lock(&mutex);
            // Remover cliente da lista
            for (int i = 0; i < num_clientes; i++) {
                if (clientes[i].socket == cliente_socket) {
                    for (int j = i; j < num_clientes - 1; j++) {
                        clientes[j] = clientes[j + 1];
                    }
                    num_clientes--;
                    break;
                }
            }
            pthread_mutex_unlock(&mutex);
            return NULL;
        }

        buffer[bytes] = '\0';
        printf("Mensagem recebida de %s: %s\n", nome, buffer);

        // Processar comandos
        char comando[LEN];
        sscanf(buffer, "%s", comando);

        // Novo comando /online
        if (strcmp(comando, "/online") == 0) {
            char lista_clientes[LEN] = "Clientes online: ";
            pthread_mutex_lock(&mutex);
            for (int i = 0; i < num_clientes; i++) {
                strcat(lista_clientes, clientes[i].name);
                if (i < num_clientes - 1) {
                    strcat(lista_clientes, ", ");
                }
            }
            pthread_mutex_unlock(&mutex);
            send(cliente_socket, lista_clientes, strlen(lista_clientes), 0);
        }
        // Comando de envio de mensagem /msg
        else if (strcmp(comando, "/msg") == 0) {
            char destinatario[50];
            char mensagem[LEN];
            sscanf(buffer, "%s %s %[^\n]", comando, destinatario, mensagem);

            pthread_mutex_lock(&mutex);
            int destinatario_socket = -1;
            for (int i = 0; i < num_clientes; i++) {
                if (strcmp(clientes[i].name, destinatario) == 0) {
                    destinatario_socket = clientes[i].socket;
                    break;
                }
            }
            pthread_mutex_unlock(&mutex);

            if (destinatario_socket != -1) {
                // Concatenar nome do remetente com a mensagem
                char mensagem_completa[LEN];
                snprintf(mensagem_completa, LEN - 1, "%s: %s", nome, mensagem);

                // Enviar a mensagem formatada para o destinatário
                send(destinatario_socket, mensagem_completa, strlen(mensagem_completa), 0);
            } else {
                char erro[] = "Cliente não encontrado.\n";
                send(cliente_socket, erro, strlen(erro), 0);
            }
        }
    }
}


void* gerenciar_cliente(void* arg) {
    int cliente_socket = *((int*)arg);
    char buffer[LEN];
    char nome[50];

    // Receber o nome do cliente
    memset(buffer, 0x0, LEN);
    recv(cliente_socket, buffer, LEN, 0);
    strcpy(nome, buffer);

    printf("Cliente conectado: %s\n", nome);

    // Adicionar o cliente à lista
    pthread_mutex_lock(&mutex);
    if (num_clientes < MAX_CLIENTS) {
        clientes[num_clientes].socket = cliente_socket;
        strcpy(clientes[num_clientes].name, nome);
        num_clientes++;
    } else {
        printf("Número máximo de clientes alcançado.\n");
        close(cliente_socket);
        pthread_mutex_unlock(&mutex);
        return NULL;
    }
    pthread_mutex_unlock(&mutex);

    while (1) {
        memset(buffer, 0x0, LEN);
        int bytes = recv(cliente_socket, buffer, LEN, 0);
        if (bytes <= 0) {
            printf("Cliente desconectado: %s\n", nome);
            close(cliente_socket);
            pthread_mutex_lock(&mutex);
            // Remover cliente da lista
            for (int i = 0; i < num_clientes; i++) {
                if (clientes[i].socket == cliente_socket) {
                    for (int j = i; j < num_clientes - 1; j++) {
                        clientes[j] = clientes[j + 1];
                    }
                    num_clientes--;
                    break;
                }
            }
            pthread_mutex_unlock(&mutex);
            return NULL;
        }

        buffer[bytes] = '\0';
        printf("Mensagem recebida de %s: %s\n", nome, buffer);

        // Enviar mensagem para o destinatário
        char comando[LEN];
        sscanf(buffer, "%s", comando);

        if (strcmp(comando, "/msg") == 0) {
            char destinatario[50];
            char mensagem[LEN];
            sscanf(buffer, "%s %s %[^\n]", comando, destinatario, mensagem);

            pthread_mutex_lock(&mutex);
            int destinatario_socket = -1;
            for (int i = 0; i < num_clientes; i++) {
                if (strcmp(clientes[i].name, destinatario) == 0) {
                    destinatario_socket = clientes[i].socket;
                    break;
                }
            }
            pthread_mutex_unlock(&mutex);

            if (destinatario_socket != -1) {
                // Concatenar nome do remetente com a mensagem, limitando o tamanho
                char mensagem_completa[LEN];
                snprintf(mensagem_completa, LEN - 1, "%s: %.*s", nome, LEN - (int)strlen(nome) - 4, mensagem);

                // Enviar a mensagem formatada para o destinatário
                send(destinatario_socket, mensagem_completa, strlen(mensagem_completa), 0);
            } else {
                char erro[] = "Cliente não encontrado.\n";
                send(cliente_socket, erro, strlen(erro), 0);
            }
        }
    }
}


int main() {
    int sockfd, cliente_socket;
    struct sockaddr_in local, remoto;
    int len = sizeof(remoto);
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
    local.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(local.sin_zero, 0x0, 8);

    // Vincular o socket ao endereço local
    if (bind(sockfd, (struct sockaddr*)&local, sizeof(local)) == -1) {
        perror("bind ");
        exit(1);
    }

    // Colocar o socket em modo de escuta
    listen(sockfd, MAX_CLIENTS);

    while (1) {
        cliente_socket = accept(sockfd, (struct sockaddr*)&remoto, &len);
        if (cliente_socket == -1) {
            perror("accept ");
            continue;
        }

        // Criar uma nova thread para gerenciar o cliente
        if (pthread_create(&thread_id, NULL, gerenciar_cliente, (void*)&cliente_socket) != 0) {
            perror("pthread_create ");
        }
        pthread_detach(thread_id);
    }

    close(sockfd);
    return 0;
}
