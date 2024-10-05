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
#define MAX_SALAS 10
#define MAX_MEMBROS_SALA 10

typedef struct {
    int socket;
    char name[50];
} Cliente;

typedef struct {
    char nome[50];
    Cliente membros[MAX_MEMBROS_SALA];
    int num_membros;
} Sala;

Cliente clientes[MAX_CLIENTS];
int num_clientes = 0;
Sala salas[MAX_SALAS];
int num_salas = 0;
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
                int espaco_disponivel = LEN - strlen(nome) - 3;  // 3 espaços para ": " e '\0'
                snprintf(mensagem_completa, LEN - 1, "%s: %.*s", nome, espaco_disponivel, mensagem);

                // Enviar a mensagem formatada para o destinatário
                send(destinatario_socket, mensagem_completa, strlen(mensagem_completa), 0);
            } else {
                char erro[] = "Cliente não encontrado.\n";
                send(cliente_socket, erro, strlen(erro), 0);
            }
        }
        // Comando para criar uma sala /create
        else if (strcmp(comando, "/create") == 0) {
            char nome_sala[50];
            sscanf(buffer, "%s %s", comando, nome_sala);

            pthread_mutex_lock(&mutex);
            if (num_salas < MAX_SALAS) {
                strcpy(salas[num_salas].nome, nome_sala);
                salas[num_salas].num_membros = 0;
                num_salas++;
                char resposta[] = "Sala criada com sucesso.\n";
                send(cliente_socket, resposta, strlen(resposta), 0);
            } else {
                char erro[] = "Número máximo de salas alcançado.\n";
                send(cliente_socket, erro, strlen(erro), 0);
            }
            pthread_mutex_unlock(&mutex);
        }
        // Comando para entrar em uma sala /join
        else if (strcmp(comando, "/join") == 0) {
            char nome_sala[50];
            sscanf(buffer, "%s %s", comando, nome_sala);

            pthread_mutex_lock(&mutex);
            int sala_index = -1;
            for (int i = 0; i < num_salas; i++) {
                if (strcmp(salas[i].nome, nome_sala) == 0) {
                    sala_index = i;
                    break;
                }
            }

            if (sala_index != -1 && salas[sala_index].num_membros < MAX_MEMBROS_SALA) {
                salas[sala_index].membros[salas[sala_index].num_membros].socket = cliente_socket;
                strcpy(salas[sala_index].membros[salas[sala_index].num_membros].name, nome);
                salas[sala_index].num_membros++;
                char resposta[] = "Entrou na sala com sucesso.\n";
                send(cliente_socket, resposta, strlen(resposta), 0);
            } else {
                char erro[] = "Sala não encontrada ou sala cheia.\n";
                send(cliente_socket, erro, strlen(erro), 0);
            }
            pthread_mutex_unlock(&mutex);
        }
        // Comando para listar salas /list
        else if (strcmp(comando, "/list") == 0) {
            char lista_salas[LEN] = "Salas disponíveis: ";
            pthread_mutex_lock(&mutex);
            for (int i = 0; i < num_salas; i++) {
                strcat(lista_salas, salas[i].nome);
                if (i < num_salas - 1) {
                    strcat(lista_salas, ", ");
                }
            }
            pthread_mutex_unlock(&mutex);
            send(cliente_socket, lista_salas, strlen(lista_salas), 0);
        }
        // Comando para sair de uma sala /leave
        else if (strcmp(comando, "/leave") == 0) {
            char nome_sala[50];
            sscanf(buffer, "%s %s", comando, nome_sala);

            pthread_mutex_lock(&mutex);
            int sala_index = -1;
            for (int i = 0; i < num_salas; i++) {
                if (strcmp(salas[i].nome, nome_sala) == 0) {
                    sala_index = i;
                    break;
                }
            }

            if (sala_index != -1) {
                int membro_index = -1;
                for (int i = 0; i < salas[sala_index].num_membros; i++) {
                    if (salas[sala_index].membros[i].socket == cliente_socket) {
                        membro_index = i;
                        break;
                    }
                }

                if (membro_index != -1) {
                    for (int i = membro_index; i < salas[sala_index].num_membros - 1; i++) {
                        salas[sala_index].membros[i] = salas[sala_index].membros[i + 1];
                    }
                    salas[sala_index].num_membros--;
                    char resposta[] = "Saiu da sala com sucesso.\n";
                    send(cliente_socket, resposta, strlen(resposta), 0);
                } else {
                    char erro[] = "Você não está nesta sala.\n";
                    send(cliente_socket, erro, strlen(erro), 0);
                }
            } else {
                char erro[] = "Sala não encontrada.\n";
                send(cliente_socket, erro, strlen(erro), 0);
            }
            pthread_mutex_unlock(&mutex);
        }
        // Comando para enviar mensagem para uma sala /sala
        else if (strcmp(comando, "/sala") == 0) {
            char nome_sala[50];
            char mensagem[LEN];
            sscanf(buffer, "%s %s %[^\n]", comando, nome_sala, mensagem);

            pthread_mutex_lock(&mutex);
            int sala_index = -1;
            for (int i = 0; i < num_salas; i++) {
                if (strcmp(salas[i].nome, nome_sala) == 0) {
                    sala_index = i;
                    break;
                }
            }

            if (sala_index != -1) {
                char mensagem_completa[LEN];
                int espaco_disponivel = LEN - strlen(nome) - 3;  // 3 espaços para ": " e '\0'
                snprintf(mensagem_completa, LEN - 1, "%s (sala %s): %.*s", nome, nome_sala, espaco_disponivel, mensagem);

                for (int i = 0; i < salas[sala_index].num_membros; i++) {
                    send(salas[sala_index].membros[i].socket, mensagem_completa, strlen(mensagem_completa), 0);
                }
            } else {
                char erro[] = "Sala não encontrada.\n";
                send(cliente_socket, erro, strlen(erro), 0);
            }
            pthread_mutex_unlock(&mutex);
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